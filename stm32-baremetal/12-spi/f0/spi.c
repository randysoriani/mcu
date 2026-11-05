/*
 * spi.c (F0) - SPI1 mestre por polling.
 *
 * O SPI do F0 tem FIFOs de 32 bits e tamanho de dado programável (DS = 4..16
 * bits). Duas armadilhas clássicas:
 *   1. Para dados de 8 bits, configure FRXTH = 1 (RXNE com 1 byte na FIFO)
 *   2. Acesse o DR com um ponteiro de 8 BITS. Uma escrita de 16/32 bits
 *      coloca DOIS bytes na FIFO (data packing) e envia 16 clocks.
 */
#include "device.h"
#include "gpio.h"
#include "spi.h"

#define SPI_DR8 (*(volatile uint8_t *)&SPI1->DR)

void spi_init(void)
{
    RCC->AHBENR  |= RCC_AHBENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    gpio_af(GPIOB, 3, 0);                     /* SCK  (AF0) */
    gpio_af(GPIOB, 4, 0);                     /* MISO (AF0) */
    gpio_af(GPIOB, 5, 0);                     /* MOSI (AF0) */
    gpio_speed(GPIOB, 3, GPIO_SPEED_HIGH);
    gpio_speed(GPIOB, 5, GPIO_SPEED_HIGH);

    gpio_set(GPIOB, 6);                       /* CS inativo (alto) */
    gpio_mode(GPIOB, 6, GPIO_MODE_OUTPUT);

    SPI1->CR1 = 0;
    SPI1->CR1 = SPI_CR1_MSTR                  /* mestre                         */
              | (2U << SPI_CR1_BR_Pos)        /* BR = 010: fPCLK/8 = 6 MHz      */
              | SPI_CR1_SSM | SPI_CR1_SSI;    /* NSS por software (evita MODF)  */
              /* CPOL = 0, CPHA = 0 -> modo 0 */
    SPI1->CR2 = (7U << SPI_CR2_DS_Pos)        /* DS = 0111: 8 bits              */
              | SPI_CR2_FRXTH;                /* RXNE quando há >= 8 bits       */
    SPI1->CR1 |= SPI_CR1_SPE;
}

uint8_t spi_xfer(uint8_t out)
{
    while (!(SPI1->SR & SPI_SR_TXE)) { }
    SPI_DR8 = out;
    while (!(SPI1->SR & SPI_SR_RXNE)) { }
    return SPI_DR8;
}

void spi_xfer_buf(const uint8_t *tx, uint8_t *rx, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        uint8_t r = spi_xfer(tx ? tx[i] : 0xFF);
        if (rx) rx[i] = r;
    }
}

void spi_wait_idle(void)
{
    while (SPI1->SR & SPI_SR_FTLVL_Msk) { }   /* FIFO de TX vazia */
    while (SPI1->SR & SPI_SR_BSY) { }
}

void spi_cs(int ativo)
{
    if (ativo) {
        gpio_clear(GPIOB, 6);
    } else {
        spi_wait_idle();                      /* não solte o CS no meio de um byte */
        gpio_set(GPIOB, 6);
    }
}
