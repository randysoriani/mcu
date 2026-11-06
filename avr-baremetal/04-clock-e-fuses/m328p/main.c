/*
 * Módulo 04 - Clock e fuses do ATmega328P (programa autocontido)
 *
 * 1. Lê e imprime os FUSES (é o fuse que escolhe a fonte de clock!)
 * 2. Pisca o LED com um laço fixo usando o divisor CLKPR = ÷1, ÷2, ÷4, ÷8:
 *    o mesmo laço fica 2x mais lento a cada passo.
 *
 * A UART é configurada de novo a cada mudança, porque o baud rate depende
 * do clock.
 */
#include "device.h"

static uint32_t f_cpu = F_CPU;

/* Lê um byte de fuse/lock com a sequência BLBSET + SPMEN seguida de LPM
 * (datasheet, "Reading the Fuse and Lock Bits from Software").
 * Z = 0: fuse baixo, 1: lock bits, 2: fuse estendido, 3: fuse alto */
static uint8_t read_fuse(uint16_t z)
{
    uint8_t r;
    __asm__ __volatile__ (
        "sts %1, %2 \n\t"           /* SPMCSR = (1<<BLBSET) | (1<<SPMEN) */
        "lpm %0, Z  \n\t"           /* tem de vir nos 3 ciclos seguintes */
        : "=r"(r)
        : "i"(0x57), "r"((uint8_t)((1U << BLBSET) | (1U << SPMEN))), "z"(z));
    return r;
}

static void uart_tx_init(void)
{
    UCSR0A = (1U << U2X0);
    UBRR0  = (uint16_t)(f_cpu / 8UL / 9600UL - 1UL);   /* 9600 bps: funciona em todos os divisores */
    UCSR0C = (1U << UCSZ01) | (1U << UCSZ00);
    UCSR0B = (1U << TXEN0);
}

static void putc_(char c) { while (!(UCSR0A & (1U << UDRE0))) { } UDR0 = (uint8_t)c; }
static void puts_(const char *s) { while (*s) { if (*s == '\n') putc_('\r'); putc_(*s++); } }
static void puthex(uint8_t v) { const char *h = "0123456789ABCDEF"; putc_(h[v >> 4]); putc_(h[v & 15]); }

static void set_clkpr(uint8_t div_log2)    /* 0 = ÷1, 1 = ÷2, 2 = ÷4, 3 = ÷8 ... */
{
    uint8_t s = SREG;
    cli();
    CLKPR = (1U << CLKPCE);                 /* destrava (vale por 4 ciclos) */
    CLKPR = div_log2;
    SREG = s;
    f_cpu = F_CPU >> div_log2;
}

static void delay_fixo(void) { for (volatile uint32_t i = 0; i < 60000UL; i++) { } }

int main(void)
{
    DDRB |= (1U << 5);
    set_clkpr(0);
    uart_tx_init();

    puts_("\n=== Modulo 04: clock e fuses (9600 bps) ===\n");
    puts_("lfuse=0x"); puthex(read_fuse(0));
    puts_(" hfuse=0x"); puthex(read_fuse(3));
    puts_(" efuse=0x"); puthex(read_fuse(2));
    puts_(" lock=0x");  puthex(read_fuse(1));
    puts_("\nArduino Uno de fabrica: lfuse=FF (cristal externo, sem CKDIV8)\n");

    for (;;) {
        for (uint8_t d = 0; d <= 3; d++) {
            set_clkpr(d);
            uart_tx_init();
            puts_("CLKPR = /"); putc_((char)('0' + (1 << d))); puts_("\n");
            for (uint8_t i = 0; i < 6; i++) { PINB = (1U << 5); delay_fixo(); }
        }
    }
}
