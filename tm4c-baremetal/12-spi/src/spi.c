/*
 * spi.c - SSI2 mestre, Freescale SPI modo 0, 8 bits, ~5 MHz, por polling.
 *
 *          CLK      MOSI(TX)     MISO(RX)     CS (GPIO)   PCTL
 *   123:   PB4      PB7          PB6          PB5         2
 *   129:   PD3      PD1(XDAT0)   PD0(XDAT1)   PD2         15
 *
 * Obs. 123: na LaunchPad, PB6/PB7 estão ligados a PD0/PD1 por resistores de
 * 0 ohm (R9/R10). Não use PD0/PD1 como saída junto com o SSI2.
 */
#include "system.h"
#include "gpio.h"
#include "spi.h"

#if defined(TM4C123)
#  define SPI_PORT GPIOB
#  define SPI_PORTN PORTB
#  define PIN_CLK 4
#  define PIN_TX  7
#  define PIN_RX  6
#  define PIN_CS  5
#  define SPI_PCTL 2
#else
#  define SPI_PORT GPIOD
#  define SPI_PORTN PORTD
#  define PIN_CLK 3
#  define PIN_TX  1
#  define PIN_RX  0
#  define PIN_CS  2
#  define SPI_PCTL 15
#endif

void spi_init(int loopback)
{
    periph_enable(&SYSCTL_RCGCSSI, &SYSCTL_PRSSI, 2);
    gpio_port_enable(SPI_PORTN);
    gpio_af(SPI_PORT, PIN_CLK, SPI_PCTL);
    gpio_af(SPI_PORT, PIN_TX,  SPI_PCTL);
    gpio_af(SPI_PORT, PIN_RX,  SPI_PCTL);
    gpio_set(SPI_PORT, PIN_CS);
    gpio_output(SPI_PORT, PIN_CS);

    SSI2->CR1  = 0;                                  /* SSE = 0, mestre       */
    SSI2->CC   = 0;                                  /* clock = sysclk        */
    /* bit rate = SYSCLK / (CPSDVSR * (1 + SCR))
     *   123: 80 MHz / (8 * 2) = 5 MHz     129: 120 MHz / (12 * 2) = 5 MHz */
    SSI2->CPSR = SystemCoreClock / 10000000U;        /* 8 ou 12 (tem que ser par) */
    SSI2->CR0  = (1U << SSI_CR0_SCR_Pos)             /* SCR = 1               */
               | SSI_CR0_FRF_MOTO                    /* Freescale SPI         */
               | SSI_CR0_DSS_8;                      /* SPO = SPH = 0: modo 0 */
    SSI2->CR1  = (loopback ? SSI_CR1_LBM : 0U) | SSI_CR1_SSE;
}

uint8_t spi_xfer(uint8_t out)
{
    while (!(SSI2->SR & SSI_SR_TNF)) { }             /* FIFO de TX não cheia  */
    SSI2->DR = out;
    while (!(SSI2->SR & SSI_SR_RNE)) { }             /* FIFO de RX não vazia  */
    return (uint8_t)SSI2->DR;
}

void spi_xfer_buf(const uint8_t *tx, uint8_t *rx, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        uint8_t r = spi_xfer(tx ? tx[i] : 0xFF);
        if (rx) rx[i] = r;
    }
}

void spi_cs(int ativo)
{
    if (ativo) {
        gpio_clear(SPI_PORT, PIN_CS);
    } else {
        while (SSI2->SR & SSI_SR_BSY) { }            /* espera o último bit sair */
        gpio_set(SPI_PORT, PIN_CS);
    }
}
