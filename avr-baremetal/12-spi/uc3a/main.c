/*
 * Módulo 12 - SPI (AT32UC3A): SPI0 em modo mestre com LOOPBACK INTERNO
 * (bit LLB do MR): o MOSI é ligado ao MISO dentro do chip, sem fios.
 * Para um dispositivo real: SPI0 SCK = PA13, MISO = PA11, MOSI = PA12,
 * NPCS0 = PA10 (função A) — confira na tabela de multiplexação.
 */
#include "system.h"

static void spi_init(void)
{
    SPI0->CR = (1UL << 7);                         /* SWRST */
    SPI0->MR = (1UL << 0)                          /* MSTR: mestre */
             | (1UL << 4)                          /* MODFDIS: sem detecção de falha de modo */
             | (1UL << 7)                          /* LLB: loopback interno */
             | (0xEUL << 16);                      /* PCS = 1110: seleciona o chip-select 0 */
    SPI0->CSR[0] = (1UL << 1)                      /* NCPHA = 1 + CPOL = 0: modo 0 */
                 | (0UL << 4)                      /* BITS = 0: 8 bits */
                 | (48UL << 8);                    /* SCBR: 48 MHz / 48 = 1 MHz */
    SPI0->CR = (1UL << 0);                         /* SPIEN */
}

static uint8_t spi_xfer(uint8_t out)
{
    while (!(SPI0->SR & (1UL << 1))) { }           /* TDRE: pode escrever */
    SPI0->TDR = out;
    while (!(SPI0->SR & (1UL << 0))) { }           /* RDRF: chegou o byte */
    return (uint8_t)SPI0->RDR;
}

int main(void)
{
    system_init();
    uart_init(115200);
    spi_init();
    kprintf("\n=== Modulo 12: SPI0 com loopback interno ===\n");
    uint32_t ok = 0, erro = 0;
    for (uint32_t rodada = 0;; rodada++) {
        for (uint32_t i = 0; i < 256; i++) {
            uint8_t tx = (uint8_t)(i + rodada);
            if (spi_xfer(tx) == tx) ok++; else erro++;
        }
        kprintf("rodada %lu: ok = %lu, erro = %lu\n", (unsigned long)rodada, (unsigned long)ok, (unsigned long)erro);
        delay_ms(1000);
    }
}
