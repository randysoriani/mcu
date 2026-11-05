/*
 * uart_irq.c (F0) - driver da USART2 com interrupção e buffers circulares.
 *
 * Diferença para o F4: a USART "nova" do F0 tem registradores separados
 * para status (ISR), limpeza de flags (ICR), recepção (RDR) e transmissão (TDR).
 */
#include "system.h"
#include "gpio.h"

#define RX_SIZE 128U          /* potências de 2 -> índice com & (N-1) */
#define TX_SIZE 256U

static volatile uint8_t  rx_buf[RX_SIZE];
static volatile uint8_t  tx_buf[TX_SIZE];
static volatile uint32_t rx_head, rx_tail;    /* head: escreve, tail: lê */
static volatile uint32_t tx_head, tx_tail;
volatile uint32_t uart_rx_overflow;           /* bytes perdidos (buffer cheio/ORE) */

void uart_init(uint32_t baud)
{
    RCC->AHBENR  |= RCC_AHBENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    gpio_af(GPIOA, 2, 1);                 /* PA2 = USART2_TX (AF1) */
    gpio_af(GPIOA, 3, 1);                 /* PA3 = USART2_RX (AF1) */
    gpio_pull(GPIOA, 3, GPIO_PULLUP);

    USART2->CR1 = 0;                      /* UE = 0: permite configurar */
    USART2->CR2 = 0;                      /* 1 stop bit                 */
    USART2->CR3 = 0;                      /* sem flow control           */
    USART2->BRR = (APB1Clock + baud / 2U) / baud;
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

    NVIC_SetPriority(USART2_IRQn, 2);
    NVIC_EnableIRQ(USART2_IRQn);
}

void USART2_IRQHandler(void)
{
    uint32_t isr = USART2->ISR;

    /* Erros: precisam ser limpos via ICR, senão a IRQ fica disparando */
    if (isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE | USART_ISR_PE)) {
        if (isr & USART_ISR_ORE) uart_rx_overflow++;
        USART2->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NCF | USART_ICR_PECF;
    }

    /* Recepção: ler RDR limpa RXNE */
    if (isr & USART_ISR_RXNE) {
        uint8_t  c    = (uint8_t)USART2->RDR;
        uint32_t next = (rx_head + 1U) & (RX_SIZE - 1U);
        if (next != rx_tail) {
            rx_buf[rx_head] = c;
            rx_head = next;
        } else {
            uart_rx_overflow++;
        }
    }

    /* Transmissão: TXE = TDR vazio. Se não há mais nada, desliga TXEIE. */
    if ((USART2->CR1 & USART_CR1_TXEIE) && (isr & USART_ISR_TXE)) {
        if (tx_tail != tx_head) {
            USART2->TDR = tx_buf[tx_tail];
            tx_tail = (tx_tail + 1U) & (TX_SIZE - 1U);
        } else {
            USART2->CR1 &= ~USART_CR1_TXEIE;
        }
    }
}

void uart_putc(char c)
{
    uint32_t next = (tx_head + 1U) & (TX_SIZE - 1U);
    while (next == tx_tail) { }           /* buffer cheio: espera a ISR esvaziar */
    tx_buf[tx_head] = (uint8_t)c;
    tx_head = next;
    USART2->CR1 |= USART_CR1_TXEIE;       /* (re)liga a interrupção de TX */
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
