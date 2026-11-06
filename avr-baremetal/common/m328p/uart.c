/*
 * uart.c (ATmega328P) - USART0 por polling (PD0 = RXD, PD1 = TXD).
 * No Arduino Uno esses pinos vão ao conversor USB-serial -> /dev/ttyACM0.
 * Explicado no módulo 07.
 */
#include "system.h"

void uart_init(uint32_t baud)
{
    /* Modo de velocidade dupla (U2X0): divisor por 8 em vez de 16, menor erro.
     * UBRR = F_CPU / (8 * baud) - 1   -> 115200 @ 16 MHz: 16 (erro 2,1%) */
    UCSR0A = (1U << U2X0);
    UBRR0  = (uint16_t)((F_CPU / 8UL + baud / 2UL) / baud - 1UL);
    UCSR0C = (1U << UCSZ01) | (1U << UCSZ00);    /* 8 bits, sem paridade, 1 stop */
    UCSR0B = (1U << RXEN0) | (1U << TXEN0);      /* liga receptor e transmissor */
}

void uart_putc(char c)
{
    while (!(UCSR0A & (1U << UDRE0))) { }        /* espera o buffer de TX esvaziar */
    UDR0 = (uint8_t)c;
}

void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

int uart_getc(void)
{
    if (!(UCSR0A & (1U << RXC0))) return -1;      /* nada recebido */
    return UDR0;
}
