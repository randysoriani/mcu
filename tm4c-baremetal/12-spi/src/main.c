/*
 * Módulo 12 - SPI (SSI)
 *   DEMO 1 (padrão): loopback INTERNO do SSI (bit LBM) - não precisa de fios
 *   DEMO 2: memória W25Qxx (JEDEC ID, status, leitura)
 *   make clean && make MCU=123 EXTRA=-DDEMO=2
 */
#include "system.h"
#include "spi.h"

#ifndef DEMO
#define DEMO 1
#endif

int main(void)
{
    system_init();
    uart_init(115200);
    spi_init(DEMO == 1);
    kprintf("\n=== Modulo 12: SPI (SSI2) ===\n");

#if DEMO == 1
    kprintf("DEMO 1: loopback interno (LBM)\n");
    uint32_t ok = 0, erro = 0;
    for (uint32_t rodada = 0;; rodada++) {
        spi_cs(1);
        for (uint32_t i = 0; i < 256; i++) {
            uint8_t tx = (uint8_t)(i + rodada);
            if (spi_xfer(tx) == tx) ok++; else erro++;
        }
        spi_cs(0);
        kprintf("rodada %u: ok = %u, erro = %u\n", rodada, ok, erro);
        delay_ms(1000);
    }
#else
    uint8_t id[3], buf[16];
    spi_cs(1); spi_xfer(0x9F); spi_xfer_buf(NULL, id, 3); spi_cs(0);
    kprintf("JEDEC: fabricante 0x%02X, tipo 0x%02X, capacidade 0x%02X\n", id[0], id[1], id[2]);
    if (id[0] == 0x00 || id[0] == 0xFF) kprintf("  -> nada respondeu (fios?)\n");
    spi_cs(1); spi_xfer(0x05); uint8_t sr1 = spi_xfer(0xFF); spi_cs(0);
    kprintf("SR1 = 0x%02X (BUSY=%u WEL=%u)\n", sr1, sr1 & 1U, (sr1 >> 1) & 1U);
    spi_cs(1); spi_xfer(0x03); spi_xfer(0); spi_xfer(0); spi_xfer(0);
    spi_xfer_buf(NULL, buf, sizeof buf); spi_cs(0);
    kprintf("dados @0:");
    for (unsigned i = 0; i < sizeof buf; i++) kprintf(" %02X", buf[i]);
    kprintf("\n");
    for (;;) __WFI();
#endif
}
