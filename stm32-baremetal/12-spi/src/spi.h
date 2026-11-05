/*
 * spi.h - interface do driver SPI1 (implementado em f0/spi.c e f4/spi.c)
 *
 * Pinos (iguais nas duas Nucleo):
 *   PB3 = SCK (D3)   PB4 = MISO (D5)   PB5 = MOSI (D4)   PB6 = CS (D10, GPIO)
 */
#ifndef SPI_H
#define SPI_H
#include <stdint.h>
#include <stddef.h>

void    spi_init(void);                 /* modo 0, 8 bits, MSB primeiro, ~6 MHz */
uint8_t spi_xfer(uint8_t out);          /* full-duplex: envia 1 byte, recebe 1 */
void    spi_xfer_buf(const uint8_t *tx, uint8_t *rx, size_t n);
void    spi_cs(int ativo);              /* 1 = CS baixo (selecionado)          */
void    spi_wait_idle(void);            /* espera BSY = 0                      */

#endif
