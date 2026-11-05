/*
 * i2c.c (F4) - I2C1 mestre, IP "clássica" (I2C v1, com SR1/SR2/CCR).
 *
 * Nesta IP o SOFTWARE conduz cada fase do protocolo seguindo uma sequência
 * de eventos (EV5, EV6, EV8, EV7... no RM0390). Pontos delicados:
 *   - ADDR só é limpo lendo SR1 e DEPOIS SR2
 *   - ao ler, o ACK/NACK do último byte tem de ser programado ANTES de o
 *     byte terminar de chegar -> sequências especiais para N = 1, 2 e > 2
 */
#include "system.h"
#include "gpio.h"
#include "i2c.h"

#define TIMEOUT_MS 10U

static int wait_sr1(uint32_t flag)
{
    uint32_t t0 = millis();
    while (!(I2C1->SR1 & flag)) {
        if (I2C1->SR1 & I2C_SR1_AF) {                 /* Acknowledge Failure = NACK */
            I2C1->SR1 = ~I2C_SR1_AF;                  /* rc_w0 */
            I2C1->CR1 |= I2C_CR1_STOP;
            return I2C_NACK;
        }
        if (millis() - t0 > TIMEOUT_MS) return I2C_TIMEOUT;
    }
    return I2C_OK;
}

static int recover(int err)
{
    if (err == I2C_TIMEOUT) {
        /* reset por software do periférico e reconfiguração */
        i2c_init();
    }
    return err;
}

static inline void clear_addr(void)
{
    (void)I2C1->SR1;
    (void)I2C1->SR2;
}

/* START + endereço. Deixa ADDR pendente (quem chama decide quando limpar) */
static int start(uint8_t addr_rw)
{
    I2C1->CR1 |= I2C_CR1_START;
    int e = wait_sr1(I2C_SR1_SB);          /* EV5: START enviado */
    if (e) return e;
    I2C1->DR = addr_rw;                    /* ler SR1 + escrever DR limpa SB */
    return wait_sr1(I2C_SR1_ADDR);         /* EV6: endereço reconhecido (ACK) */
}

static int wait_not_busy(void)
{
    uint32_t t0 = millis();
    while (I2C1->SR2 & I2C_SR2_BUSY) {
        if (millis() - t0 > TIMEOUT_MS) return I2C_TIMEOUT;
    }
    return I2C_OK;
}

void i2c_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    (void)RCC->APB1ENR;

    for (uint32_t pin = 8; pin <= 9; pin++) {
        gpio_otype(GPIOB, pin, GPIO_OPENDRAIN);
        gpio_pull(GPIOB, pin, GPIO_PULLUP);
        gpio_speed(GPIOB, pin, GPIO_SPEED_HIGH);
        gpio_af(GPIOB, pin, 4);            /* AF4 = I2C1 */
    }

    I2C1->CR1 = I2C_CR1_SWRST;             /* reset do periférico */
    I2C1->CR1 = 0;

    uint32_t mhz = APB1Clock / 1000000U;   /* 45 */
    I2C1->CR2   = mhz;                     /* FREQ: clock do APB1 em MHz */
    /* Standard mode 100 kHz: T_high = T_low = CCR * T_pclk
     * -> CCR = PCLK1 / (2 * 100 kHz) = 225 */
    I2C1->CCR   = APB1Clock / (2U * 100000U);
    /* TRISE = tempo máximo de subida (1000 ns no SM) / T_pclk + 1 */
    I2C1->TRISE = mhz + 1U;
    I2C1->CR1   = I2C_CR1_PE;
}

int i2c_probe(uint8_t addr7)
{
    if (wait_not_busy()) return recover(I2C_TIMEOUT);
    int e = start((uint8_t)(addr7 << 1));
    if (e) return recover(e);
    clear_addr();
    I2C1->CR1 |= I2C_CR1_STOP;
    return I2C_OK;
}

static int write_bytes(const uint8_t *data, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        int e = wait_sr1(I2C_SR1_TXE);     /* EV8: DR vazio */
        if (e) return e;
        I2C1->DR = data[i];
    }
    return wait_sr1(I2C_SR1_BTF);          /* último byte totalmente enviado */
}

static int read_bytes(uint8_t addr7, uint8_t *data, size_t n)
{
    int e = start((uint8_t)((addr7 << 1) | 1U));
    if (e) return e;

    if (n == 1) {
        I2C1->CR1 &= ~I2C_CR1_ACK;         /* NACK no único byte */
        clear_addr();
        I2C1->CR1 |= I2C_CR1_STOP;
        if ((e = wait_sr1(I2C_SR1_RXNE))) return e;
        data[0] = (uint8_t)I2C1->DR;
    } else if (n == 2) {
        I2C1->CR1 |= I2C_CR1_POS | I2C_CR1_ACK;   /* POS: ACK vale p/ o PRÓXIMO byte */
        clear_addr();
        I2C1->CR1 &= ~I2C_CR1_ACK;
        if ((e = wait_sr1(I2C_SR1_BTF))) return e;  /* byte1 em DR, byte2 no shift */
        I2C1->CR1 |= I2C_CR1_STOP;
        data[0] = (uint8_t)I2C1->DR;
        data[1] = (uint8_t)I2C1->DR;
        I2C1->CR1 &= ~I2C_CR1_POS;
    } else {
        I2C1->CR1 |= I2C_CR1_ACK;
        clear_addr();
        size_t i = 0;
        while (n - i > 3) {
            if ((e = wait_sr1(I2C_SR1_RXNE))) return e;
            data[i++] = (uint8_t)I2C1->DR;
        }
        /* faltam 3: N-2 em DR e N-1 no shift register */
        if ((e = wait_sr1(I2C_SR1_BTF))) return e;
        I2C1->CR1 &= ~I2C_CR1_ACK;          /* o byte N receberá NACK */
        data[i++] = (uint8_t)I2C1->DR;      /* N-2 */
        if ((e = wait_sr1(I2C_SR1_BTF))) return e;
        I2C1->CR1 |= I2C_CR1_STOP;
        data[i++] = (uint8_t)I2C1->DR;      /* N-1 */
        if ((e = wait_sr1(I2C_SR1_RXNE))) return e;
        data[i++] = (uint8_t)I2C1->DR;      /* N   */
    }
    return I2C_OK;
}

int i2c_write(uint8_t addr7, const uint8_t *data, size_t n)
{
    if (wait_not_busy()) return recover(I2C_TIMEOUT);
    int e = start((uint8_t)(addr7 << 1));
    if (e) return recover(e);
    clear_addr();
    e = write_bytes(data, n);
    if (e) return recover(e);
    I2C1->CR1 |= I2C_CR1_STOP;
    return I2C_OK;
}

int i2c_read(uint8_t addr7, uint8_t *data, size_t n)
{
    if (n == 0) return I2C_OK;
    if (wait_not_busy()) return recover(I2C_TIMEOUT);
    return recover(read_bytes(addr7, data, n));
}

int i2c_write_read(uint8_t addr7, const uint8_t *w, size_t wn, uint8_t *r, size_t rn)
{
    if (wait_not_busy()) return recover(I2C_TIMEOUT);
    int e = start((uint8_t)(addr7 << 1));
    if (e) return recover(e);
    clear_addr();
    e = write_bytes(w, wn);
    if (e) return recover(e);
    /* sem STOP: read_bytes() gera o REPEATED START */
    return recover(read_bytes(addr7, r, rn));
}
