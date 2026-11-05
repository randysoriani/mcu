/*
 * Módulo 12 - SPI
 *
 * DEMO 1 (padrão): loopback. Ligue um jumper MOSI -> MISO (D4/PB5 -> D5/PB4).
 *                  Cada byte enviado deve voltar igual.
 * DEMO 2:          memória Flash SPI (W25Q16/W25Q32/W25Q64/W25Q128...):
 *                  lê o JEDEC ID (cmd 0x9F), o registrador de status (0x05)
 *                  e os primeiros 16 bytes (0x03).
 *                  Ligações: SCK=D3 MISO=D5 MOSI=D4 CS=D10, VCC=3V3, GND.
 *
 *   make clean && make MCU=f0 EXTRA=-DDEMO=2
 */
#include "system.h"
#include "spi.h"

#ifndef DEMO
#define DEMO 1
#endif

#if DEMO == 1
static void demo(void)
{
    kprintf("DEMO 1: loopback (jumper D4 -> D5)\n");
    uint32_t ok = 0, erro = 0;
    for (uint32_t rodada = 0;; rodada++) {
        spi_cs(1);
        for (uint32_t i = 0; i < 256; i++) {
            uint8_t tx = (uint8_t)(i + rodada);
            uint8_t rx = spi_xfer(tx);
            if (rx == tx) ok++; else erro++;
        }
        spi_cs(0);
        kprintf("rodada %u: ok = %u, erro = %u%s\n", rodada, ok, erro,
                erro ? "  <- jumper ligado?" : "");
        delay_ms(1000);
    }
}

#elif DEMO == 2
static void demo(void)
{
    uint8_t id[3], buf[16];

    kprintf("DEMO 2: Flash SPI W25Qxx\n");

    spi_cs(1);
    spi_xfer(0x9F);                     /* JEDEC ID */
    spi_xfer_buf(NULL, id, 3);
    spi_cs(0);
    kprintf("JEDEC: fabricante 0x%02X, tipo 0x%02X, capacidade 0x%02X (%u KB)\n",
            id[0], id[1], id[2], (id[2] >= 0x11 && id[2] <= 0x19) ? (1U << id[2]) / 1024U : 0U);
    if (id[0] == 0xEF) kprintf("  -> Winbond\n");
    if (id[0] == 0x00 || id[0] == 0xFF) kprintf("  -> nada respondeu (fios?)\n");

    spi_cs(1);
    spi_xfer(0x05);                     /* Read Status Register-1 */
    uint8_t sr1 = spi_xfer(0xFF);
    spi_cs(0);
    kprintf("SR1 = 0x%02X (BUSY=%u WEL=%u)\n", sr1, sr1 & 1U, (sr1 >> 1) & 1U);

    spi_cs(1);
    spi_xfer(0x03);                     /* Read Data, endereço de 24 bits */
    spi_xfer(0x00); spi_xfer(0x00); spi_xfer(0x00);
    spi_xfer_buf(NULL, buf, sizeof buf);
    spi_cs(0);
    kprintf("dados @0:");
    for (unsigned i = 0; i < sizeof buf; i++) kprintf(" %02X", buf[i]);
    kprintf("\n");

    for (;;) { __WFI(); }
}
#endif

int main(void)
{
    system_init();
    uart_init(115200);
    spi_init();
    kprintf("\n=== Modulo 12: SPI ===\n");
    demo();
}
