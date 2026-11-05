/*
 * i2c.c (F0) - I2C1 mestre, IP "nova" (I2C v2, com TIMINGR).
 *
 * Nesta IP o hardware cuida do protocolo: você diz endereço, direção e
 * número de bytes em CR2 e dá START; ele gera START, endereço, ACKs e
 * (com AUTOEND) o STOP. O software só alimenta TXDR / lê RXDR.
 */
#include "system.h"
#include "gpio.h"
#include "i2c.h"

#define TIMEOUT_MS 10U

/* Espera um bit em ISR. Retorna 0, I2C_NACK ou I2C_TIMEOUT */
static int wait_flag(uint32_t flag)
{
    uint32_t t0 = millis();
    while (!(I2C1->ISR & flag)) {
        if (I2C1->ISR & I2C_ISR_NACKF) return I2C_NACK;
        if (millis() - t0 > TIMEOUT_MS) return I2C_TIMEOUT;
    }
    return I2C_OK;
}

/* Finaliza uma transação: espera o STOP e limpa as flags */
static int finish(int err)
{
    if (err == I2C_NACK) {
        /* Com NACK o hardware gera STOP sozinho (mesmo com AUTOEND = 0) */
        uint32_t t0 = millis();
        while (!(I2C1->ISR & I2C_ISR_STOPF) && (millis() - t0 <= TIMEOUT_MS)) { }
        I2C1->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
        return I2C_NACK;
    }
    if (err == I2C_TIMEOUT) {
        /* recupera: desliga e religa o periférico (reset das máquinas de estado) */
        I2C1->CR1 &= ~I2C_CR1_PE;
        while (I2C1->CR1 & I2C_CR1_PE) { }
        I2C1->CR1 |= I2C_CR1_PE;
        return I2C_TIMEOUT;
    }
    err = wait_flag(I2C_ISR_STOPF);
    I2C1->ICR = I2C_ICR_STOPCF;
    return err;
}

void i2c_init(void)
{
    RCC->AHBENR  |= RCC_AHBENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    RCC->CFGR3   &= ~(1U << 4);             /* I2C1SW = 0: clock do I2C1 = HSI 8 MHz */

    /* PB8 = SCL, PB9 = SDA: AF1, open-drain, pull-up interno (fraco) */
    for (uint32_t pin = 8; pin <= 9; pin++) {
        gpio_otype(GPIOB, pin, GPIO_OPENDRAIN);
        gpio_pull(GPIOB, pin, GPIO_PULLUP);
        gpio_speed(GPIOB, pin, GPIO_SPEED_HIGH);
        gpio_af(GPIOB, pin, 1);
    }

    I2C1->CR1 = 0;                          /* PE = 0 para configurar o TIMINGR */
    /* TIMINGR para 100 kHz com I2CCLK = 8 MHz (RM0091, tabela "Examples of
     * timings settings"): PRESC=1 SCLDEL=4 SDADEL=2 SCLH=0x0F SCLL=0x13 */
    I2C1->TIMINGR = 0x10420F13;
    I2C1->CR1 = I2C_CR1_PE;                 /* filtro analógico ligado (ANFOFF=0) */
}

/* Barramento ocupado (outro mestre ou escravo travado segurando SDA)? */
static int wait_not_busy(void)
{
    uint32_t t0 = millis();
    while (I2C1->ISR & I2C_ISR_BUSY) {
        if (millis() - t0 > TIMEOUT_MS) return I2C_TIMEOUT;
    }
    return I2C_OK;
}

int i2c_probe(uint8_t addr7)
{
    if (wait_not_busy()) return finish(I2C_TIMEOUT);
    /* NBYTES = 0: só START + endereço + STOP */
    I2C1->CR2 = ((uint32_t)addr7 << 1) | I2C_CR2_AUTOEND | I2C_CR2_START;
    uint32_t t0 = millis();
    while (!(I2C1->ISR & (I2C_ISR_STOPF | I2C_ISR_NACKF))) {
        if (millis() - t0 > TIMEOUT_MS) return finish(I2C_TIMEOUT);
    }
    return finish((I2C1->ISR & I2C_ISR_NACKF) ? I2C_NACK : I2C_OK);
}

int i2c_write(uint8_t addr7, const uint8_t *data, size_t n)
{
    if (n > 255) return I2C_TIMEOUT;        /* >255 exigiria RELOAD */
    if (wait_not_busy()) return finish(I2C_TIMEOUT);
    I2C1->CR2 = ((uint32_t)addr7 << 1) | ((uint32_t)n << I2C_CR2_NBYTES_Pos)
              | I2C_CR2_AUTOEND | I2C_CR2_START;
    for (size_t i = 0; i < n; i++) {
        int e = wait_flag(I2C_ISR_TXIS);    /* TXIS: TXDR vazio e byte anterior com ACK */
        if (e) return finish(e);
        I2C1->TXDR = data[i];
    }
    return finish(I2C_OK);
}

int i2c_read(uint8_t addr7, uint8_t *data, size_t n)
{
    if (n == 0 || n > 255) return I2C_TIMEOUT;
    if (wait_not_busy()) return finish(I2C_TIMEOUT);
    I2C1->CR2 = ((uint32_t)addr7 << 1) | I2C_CR2_RD_WRN | ((uint32_t)n << I2C_CR2_NBYTES_Pos)
              | I2C_CR2_AUTOEND | I2C_CR2_START;
    for (size_t i = 0; i < n; i++) {
        int e = wait_flag(I2C_ISR_RXNE);
        if (e) return finish(e);
        data[i] = (uint8_t)I2C1->RXDR;      /* o hardware dá NACK no último byte */
    }
    return finish(I2C_OK);
}

int i2c_write_read(uint8_t addr7, const uint8_t *w, size_t wn, uint8_t *r, size_t rn)
{
    if (wn == 0 || wn > 255 || rn == 0 || rn > 255) return I2C_TIMEOUT;
    if (wait_not_busy()) return finish(I2C_TIMEOUT);

    /* Fase de escrita SEM AUTOEND: ao final, TC = 1 e o barramento fica preso */
    I2C1->CR2 = ((uint32_t)addr7 << 1) | ((uint32_t)wn << I2C_CR2_NBYTES_Pos) | I2C_CR2_START;
    for (size_t i = 0; i < wn; i++) {
        int e = wait_flag(I2C_ISR_TXIS);
        if (e) return finish(e);
        I2C1->TXDR = w[i];
    }
    int e = wait_flag(I2C_ISR_TC);
    if (e) return finish(e);

    /* Novo START (repeated start) já em leitura, agora com AUTOEND */
    I2C1->CR2 = ((uint32_t)addr7 << 1) | I2C_CR2_RD_WRN | ((uint32_t)rn << I2C_CR2_NBYTES_Pos)
              | I2C_CR2_AUTOEND | I2C_CR2_START;
    for (size_t i = 0; i < rn; i++) {
        e = wait_flag(I2C_ISR_RXNE);
        if (e) return finish(e);
        r[i] = (uint8_t)I2C1->RXDR;
    }
    return finish(I2C_OK);
}
