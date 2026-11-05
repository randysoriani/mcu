/*
 * spi.c (F4) - SPI1 mestre por polling.
 *
 * O SPI do F4 é mais simples que o do F0: sem FIFO, só um buffer de TX e
 * um de RX, e o tamanho do dado é 8 ou 16 bits (bit DFF).
 */
#include "device.h"
#include "gpio.h"
#include "spi.h"

void spi_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;       /* SPI1 está no APB2 (90 MHz) */
    (void)RCC->APB2ENR;

    gpio_af(GPIOB, 3, 5);                     /* SCK  (AF5) */
    gpio_af(GPIOB, 4, 5);                     /* MISO (AF5) */
    gpio_af(GPIOB, 5, 5);                     /* MOSI (AF5) */
    gpio_speed(GPIOB, 3, GPIO_SPEED_HIGH);
    gpio_speed(GPIOB, 5, GPIO_SPEED_HIGH);

    gpio_set(GPIOB, 6);
    gpio_mode(GPIOB, 6, GPIO_MODE_OUTPUT);

    SPI1->CR1 = 0;
    SPI1->CR1 = SPI_CR1_MSTR
              | (3U << SPI_CR1_BR_Pos)        /* BR = 011: 90 MHz / 16 = 5,625 MHz */
              | SPI_CR1_SSM | SPI_CR1_SSI;    /* DFF = 0: 8 bits; modo 0          */
    SPI1->CR2 = 0;
    SPI1->CR1 |= SPI_CR1_SPE;
}

uint8_t spi_xfer(uint8_t out)
{
    while (!(SPI1->SR & SPI_SR_TXE)) { }
    SPI1->DR = out;
    while (!(SPI1->SR & SPI_SR_RXNE)) { }
    return (uint8_t)SPI1->DR;
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
    while (!(SPI1->SR & SPI_SR_TXE)) { }
    while (SPI1->SR & SPI_SR_BSY) { }
}

void spi_cs(int ativo)
{
    if (ativo) {
        gpio_clear(GPIOB, 6);
    } else {
        spi_wait_idle();
        gpio_set(GPIOB, 6);
    }
}
