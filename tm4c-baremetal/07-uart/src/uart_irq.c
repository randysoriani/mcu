/*
 * uart_irq.c - UART0 com interrupção e buffers circulares (igual nos dois TM4C).
 *
 * O PL011 tem FIFOs de 16 bytes. A interrupção de TX dispara quando a FIFO
 * CAI abaixo do nível programado em IFLS (borda, não nível!). Por isso:
 *   - uart_putc escreve DIRETO na FIFO se ela tem espaço e o buffer está vazio
 *   - senão enfileira e liga TXIM; a ISR mantém a FIFO cheia
 * A interrupção de RX usa RXIM (FIFO >= nível) + RTIM (timeout: byte parado
 * na FIFO por 32 bits de tempo), para não esperar a FIFO encher.
 */
#include "system.h"
#include "gpio.h"

#define RX_SIZE 128U
#define TX_SIZE 256U

static volatile uint8_t  rx_buf[RX_SIZE], tx_buf[TX_SIZE];
static volatile uint32_t rx_head, rx_tail, tx_head, tx_tail;
volatile uint32_t uart_rx_overflow;

void uart_init(uint32_t baud)
{
    periph_enable(&SYSCTL_RCGCUART, &SYSCTL_PRUART, 0);
    gpio_port_enable(PORTA);
    gpio_af(GPIOA, 0, 1);
    gpio_af(GPIOA, 1, 1);

    UART0->CTL  = 0;
    uint32_t div = (SystemCoreClock * 4U + baud / 2U) / baud;
    UART0->IBRD = div >> 6;
    UART0->FBRD = div & 0x3FU;
    UART0->LCRH = UART_LCRH_WLEN_8 | UART_LCRH_FEN;
    UART0->IFLS = (2U << 3) | (2U << 0);        /* RX e TX: metade da FIFO (8 bytes) */
    UART0->ICR  = 0x7FFU;
    UART0->IM   = UART_INT_RX | UART_INT_RT | UART_INT_OE;
    UART0->CTL  = UART_CTL_UARTEN | UART_CTL_TXE | UART_CTL_RXE;

    NVIC_SetPriority(UART0_IRQn, 3);
    NVIC_EnableIRQ(UART0_IRQn);
}

void UART0_Handler(void)
{
    uint32_t mis = UART0->MIS;
    UART0->ICR = mis;                           /* limpa o que vai ser tratado */

    if (mis & UART_INT_OE) uart_rx_overflow++;

    /* RX: esvazia a FIFO inteira */
    while (!(UART0->FR & UART_FR_RXFE)) {
        uint8_t  c    = (uint8_t)UART0->DR;
        uint32_t next = (rx_head + 1U) & (RX_SIZE - 1U);
        if (next != rx_tail) { rx_buf[rx_head] = c; rx_head = next; }
        else uart_rx_overflow++;
    }

    /* TX: completa a FIFO com o buffer */
    if (mis & UART_INT_TX) {
        while (tx_tail != tx_head && !(UART0->FR & UART_FR_TXFF)) {
            UART0->DR = tx_buf[tx_tail];
            tx_tail = (tx_tail + 1U) & (TX_SIZE - 1U);
        }
        if (tx_tail == tx_head) UART0->IM &= ~UART_INT_TX;
    }
}

void uart_putc(char c)
{
    for (;;) {
        __disable_irq();
        if (tx_head == tx_tail && !(UART0->FR & UART_FR_TXFF)) {
            UART0->DR = (uint8_t)c;              /* caminho rápido: direto na FIFO */
            __enable_irq();
            return;
        }
        uint32_t next = (tx_head + 1U) & (TX_SIZE - 1U);
        if (next != tx_tail) {
            tx_buf[tx_head] = (uint8_t)c;
            tx_head = next;
            UART0->IM |= UART_INT_TX;
            __enable_irq();
            return;
        }
        __enable_irq();                          /* buffer cheio: deixa a ISR esvaziar */
    }
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
    if (rx_tail == rx_head) return -1;
    uint8_t c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1U) & (RX_SIZE - 1U);
    return c;
}
