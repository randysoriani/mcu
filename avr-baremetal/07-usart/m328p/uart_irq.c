/*
 * uart_irq.c (ATmega328P) - USART0 com interrupções e buffers circulares
 *
 *   USART_RX_vect   : chegou um byte (RXC0)          -> guarda no buffer de RX
 *   USART_UDRE_vect : registrador de dados vazio     -> manda o próximo byte do buffer de TX
 */
#include "system.h"

#define RX_SIZE 64U            /* potências de 2 -> índice com & (N-1) */
#define TX_SIZE 128U

static volatile uint8_t rx_buf[RX_SIZE], tx_buf[TX_SIZE];
static volatile uint8_t rx_head, rx_tail, tx_head, tx_tail;   /* 8 bits: leitura atômica no AVR */
volatile uint16_t uart_rx_overflow;

void uart_init(uint32_t baud)
{
    UCSR0A = (1U << U2X0);
    UBRR0  = (uint16_t)((F_CPU / 8UL + baud / 2UL) / baud - 1UL);
    UCSR0C = (1U << UCSZ01) | (1U << UCSZ00);
    UCSR0B = (1U << RXEN0) | (1U << TXEN0) | (1U << RXCIE0);  /* interrupção de recepção */
    sei();
}

ISR(USART_RX_vect)
{
    uint8_t status = UCSR0A;               /* ler os erros ANTES do dado */
    uint8_t c = UDR0;                      /* ler UDR0 limpa o RXC0 */
    if (status & ((1U << DOR0) | (1U << FE0))) uart_rx_overflow++;
    uint8_t next = (rx_head + 1U) & (RX_SIZE - 1U);
    if (next != rx_tail) { rx_buf[rx_head] = c; rx_head = next; }
    else uart_rx_overflow++;
}

ISR(USART_UDRE_vect)
{
    if (tx_tail != tx_head) {
        UDR0 = tx_buf[tx_tail];
        tx_tail = (tx_tail + 1U) & (TX_SIZE - 1U);
    } else {
        UCSR0B &= (uint8_t)~(1U << UDRIE0);    /* nada mais a mandar: desliga a interrupção */
    }
}

void uart_putc(char c)
{
    uint8_t next = (tx_head + 1U) & (TX_SIZE - 1U);
    while (next == tx_tail) { }                /* buffer cheio: espera a ISR esvaziar */
    tx_buf[tx_head] = (uint8_t)c;
    tx_head = next;
    UCSR0B |= (1U << UDRIE0);                  /* (re)liga a interrupção "UDR vazio" */
}

void uart_puts(const char *s)
{
    while (*s) { if (*s == '\n') uart_putc('\r'); uart_putc(*s++); }
}

int uart_getc(void)
{
    if (rx_tail == rx_head) return -1;
    uint8_t c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1U) & (RX_SIZE - 1U);
    return c;
}
