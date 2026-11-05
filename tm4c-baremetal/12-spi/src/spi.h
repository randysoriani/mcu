#ifndef SPI_H
#define SPI_H
#include <stdint.h>
#include <stddef.h>
void    spi_init(int loopback);   /* loopback = 1: SSI conecta TX->RX internamente (LBM) */
uint8_t spi_xfer(uint8_t out);
void    spi_xfer_buf(const uint8_t *tx, uint8_t *rx, size_t n);
void    spi_cs(int ativo);        /* 1 = CS baixo */
#endif
