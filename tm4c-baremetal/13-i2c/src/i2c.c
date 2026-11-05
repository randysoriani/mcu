/*
 * i2c.c - I2C0 mestre (igual nos dois TM4C, só muda o PCTL)
 *
 *   PB2 = I2C0SCL, PB3 = I2C0SDA     PCTL: 3 (123) / 2 (129)
 *   SDA em open-drain; SCL em push-pull (recomendação do datasheet: o
 *   controlador cuida do clock stretching). Pull-ups externos de 4k7 a 3V3.
 *
 * O mestre da TI é "orientado a comando": escreve-se em MCS uma combinação
 * de START / RUN / STOP / ACK e o hardware executa a fase inteira.
 *
 *   escrever 1 byte:  MSA=addr<<1, MDR=dado, MCS = START|RUN|STOP
 *   escrever N:       1º START|RUN,  meio RUN,  último RUN|STOP
 *   ler 1 byte:       MSA=addr<<1|1, MCS = START|RUN|STOP
 *   ler N:            1º START|RUN|ACK, meio RUN|ACK, último RUN|STOP (NACK)
 */
#include "system.h"
#include "gpio.h"
#include "i2c.h"

#if defined(TM4C123)
#  define I2C_PCTL 3
#else
#  define I2C_PCTL 2
#endif
#define TIMEOUT_MS 10U

/* Executa um comando e espera terminar. Usa MRIS (fim de transação) em vez
 * de MCS.BUSY: no TM4C129 o BUSY demora alguns ciclos para subir. */
static int cmd(uint32_t c)
{
    I2C0->MICR = 1U;
    I2C0->MCS  = c;
    uint32_t t0 = millis();
    while (!(I2C0->MRIS & 1U)) {
        if (millis() - t0 > TIMEOUT_MS) return I2C_TIMEOUT;
    }
    uint32_t s = I2C0->MCS;
    if (s & I2C_MCS_ERROR) {
        /* sem perda de arbitragem e sem STOP já pedido -> gera STOP */
        if (!(s & I2C_MCS_ARBLST) && !(c & I2C_MCS_STOP)) {
            I2C0->MICR = 1U;
            I2C0->MCS  = I2C_MCS_STOP;
            uint32_t t1 = millis();
            while (!(I2C0->MRIS & 1U) && millis() - t1 <= TIMEOUT_MS) { }
        }
        return (s & (I2C_MCS_ADRACK | I2C_MCS_DATACK)) ? I2C_NACK : I2C_TIMEOUT;
    }
    return I2C_OK;
}

static int wait_bus_free(void)
{
    uint32_t t0 = millis();
    while (I2C0->MCS & I2C_MCS_BUSBSY) {
        if (millis() - t0 > TIMEOUT_MS) return I2C_TIMEOUT;
    }
    return I2C_OK;
}

void i2c_init(void)
{
    periph_enable(&SYSCTL_RCGCI2C, &SYSCTL_PRI2C, 0);
    gpio_port_enable(PORTB);
    gpio_af(GPIOB, 2, I2C_PCTL);
    gpio_af(GPIOB, 3, I2C_PCTL);
    GPIOB->ODR |= (1U << 3);                 /* SDA open-drain */
    GPIOB->PUR |= (1U << 3) | (1U << 2);     /* pull-ups fracos (ajudam em testes) */

    I2C0->MCR  = I2C_MCR_MFE;                /* habilita o mestre */
    /* SCL = SYSCLK / (2 * (SCL_LP + SCL_HP) * (TPR + 1)), com LP = 6, HP = 4
     * -> TPR = SYSCLK / (20 * 100 kHz) - 1:  123 -> 39, 129 -> 59 */
    I2C0->MTPR = SystemCoreClock / (20U * 100000U) - 1U;
}

int i2c_probe(uint8_t addr7)
{
    if (wait_bus_free()) return I2C_TIMEOUT;
    I2C0->MSA = ((uint32_t)addr7 << 1) | 1U;  /* leitura de 1 byte (inofensiva) */
    int e = cmd(I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP);
    (void)I2C0->MDR;
    return e;
}

static int write_phase(uint8_t addr7, const uint8_t *d, size_t n, int stop)
{
    I2C0->MSA = (uint32_t)addr7 << 1;
    for (size_t i = 0; i < n; i++) {
        uint32_t c = I2C_MCS_RUN;
        if (i == 0) c |= I2C_MCS_START;
        if (i == n - 1 && stop) c |= I2C_MCS_STOP;
        I2C0->MDR = d[i];
        int e = cmd(c);
        if (e) return e;
    }
    return I2C_OK;
}

static int read_phase(uint8_t addr7, uint8_t *d, size_t n)
{
    I2C0->MSA = ((uint32_t)addr7 << 1) | 1U;
    for (size_t i = 0; i < n; i++) {
        uint32_t c = I2C_MCS_RUN;
        if (i == 0) c |= I2C_MCS_START;           /* (repeated) START */
        if (i == n - 1) c |= I2C_MCS_STOP;        /* último: NACK + STOP */
        else            c |= I2C_MCS_ACK;
        int e = cmd(c);
        if (e) return e;
        d[i] = (uint8_t)I2C0->MDR;
    }
    return I2C_OK;
}

int i2c_write(uint8_t addr7, const uint8_t *data, size_t n)
{
    if (n == 0 || wait_bus_free()) return I2C_TIMEOUT;
    return write_phase(addr7, data, n, 1);
}

int i2c_read(uint8_t addr7, uint8_t *data, size_t n)
{
    if (n == 0 || wait_bus_free()) return I2C_TIMEOUT;
    return read_phase(addr7, data, n);
}

int i2c_write_read(uint8_t addr7, const uint8_t *w, size_t wn, uint8_t *r, size_t rn)
{
    if (wn == 0 || rn == 0 || wait_bus_free()) return I2C_TIMEOUT;
    int e = write_phase(addr7, w, wn, 0);        /* sem STOP */
    if (e) return e;
    return read_phase(addr7, r, rn);             /* começa com repeated START */
}
