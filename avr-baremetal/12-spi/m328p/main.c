/*
 * Módulo 12 - SPI (ATmega328P)
 *
 *   SCK = pino 13 (PB5)  MISO = pino 12 (PB4)  MOSI = pino 11 (PB3)  SS = pino 10 (PB2)
 *   Teste de loopback: >>> jumper do pino 11 (MOSI) para o pino 12 (MISO) <<<
 *   Atenção: o LED "L" está no pino 13 (SCK): ele pisca com o clock.
 */
#include "system.h"

static void spi_init(void)
{
    /* Mestre: SCK, MOSI e SS como saídas. O SS (PB2) PRECISA ser saída (ou
     * entrada em nível alto): se ele virar entrada em nível baixo, o SPI
     * volta sozinho para o modo escravo (MSTR = 0). */
    DDRB |= (1U << 5) | (1U << 3) | (1U << 2);
    DDRB &= (uint8_t)~(1U << 4);
    PORTB |= (1U << 2);                       /* CS inativo (alto) */
    SPCR = (1U << SPE) | (1U << MSTR) | (1U << SPR0);   /* modo 0, F_CPU/16 = 1 MHz */
}

static uint8_t spi_xfer(uint8_t out)
{
    SPDR = out;                               /* escrever inicia a transferência */
    while (!(SPSR & (1U << SPIF))) { }        /* SPIF = 1: os 8 bits foram trocados */
    return SPDR;                              /* ler SPDR (depois de ler SPSR) limpa o SPIF */
}

int main(void)
{
    system_init();
    uart_init(115200);
    spi_init();
    kprintf("\n=== Modulo 12: SPI (jumper pino 11 -> pino 12) ===\n");

    uint16_t ok = 0, erro = 0;
    for (uint8_t rodada = 0;; rodada++) {
        PORTB &= (uint8_t)~(1U << 2);             /* CS ativo */
        for (uint16_t i = 0; i < 256; i++) {
            uint8_t tx = (uint8_t)(i + rodada);
            if (spi_xfer(tx) == tx) ok++; else erro++;
        }
        PORTB |= (1U << 2);
        kprintf("rodada %u: ok = %u, erro = %u%s\n", rodada, ok, erro, erro ? "  <- jumper?" : "");
        delay_ms(1000);
    }
}
