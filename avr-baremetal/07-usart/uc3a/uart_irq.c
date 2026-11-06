/*
 * uart_irq.c (AT32UC3A) - USART0 com interrupções e buffers circulares
 * A USART0 é o grupo 6 do INTC. Uma única rotina trata RX e TX.
 */
#include "system.h"

#define RX_SIZE 128U
#define TX_SIZE 256U

static volatile uint8_t  rx_buf[RX_SIZE], tx_buf[TX_SIZE];
static volatile uint32_t rx_head, rx_tail, tx_head, tx_tail;
volatile uint16_t uart_rx_overflow;

ISR_ATTR static void usart0_handler(void)
{
    uint32_t csr = USART0->CSR;
    if (csr & USART_CSR_OVRE) { uart_rx_overflow++; USART0->CR = USART_CR_RSTSTA; }
    if (csr & USART_CSR_RXRDY) {
        uint8_t  c = (uint8_t)USART0->RHR;      /* ler RHR limpa o RXRDY */
        uint32_t next = (rx_head + 1U) & (RX_SIZE - 1U);
        if (next != rx_tail) { rx_buf[rx_head] = c; rx_head = next; }
        else uart_rx_overflow++;
    }
    if ((USART0->IMR & USART_CSR_TXRDY) && (csr & USART_CSR_TXRDY)) {
        if (tx_tail != tx_head) {
            USART0->THR = tx_buf[tx_tail];
            tx_tail = (tx_tail + 1U) & (TX_SIZE - 1U);
        } else {
            USART0->IDR = USART_CSR_TXRDY;      /* nada a mandar: desliga TXRDY */
        }
    }
}

void uart_init(uint32_t baud)
{
    gpio_periph(PA(0), 0);
    gpio_periph(PA(1), 0);
    USART0->CR = USART_CR_RSTRX | USART_CR_RSTTX | USART_CR_RXDIS | USART_CR_TXDIS;
    USART0->MR = USART_MR_CHRL_8 | USART_MR_PAR_NONE;
    uint32_t div8 = (SystemCoreClock + baud) / (2UL * baud);
    USART0->BRGR = ((div8 >> 3) & 0xFFFFUL) | ((div8 & 7UL) << 16);
    intc_register(IRQ_GRP_USART0, usart0_handler, 1);
    USART0->IER = USART_CSR_RXRDY;               /* interrupção de recepção */
    USART0->CR  = USART_CR_RXEN | USART_CR_TXEN;
    irq_enable();
}

void uart_putc(char c)
{
    uint32_t next = (tx_head + 1U) & (TX_SIZE - 1U);
    while (next == tx_tail) { }
    tx_buf[tx_head] = (uint8_t)c;
    tx_head = next;
    USART0->IER = USART_CSR_TXRDY;               /* IER/IDR: escrever 1 liga/desliga, sem RMW */
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
