/*
 * uart_irq.c (F4) - driver da USART2 com interrupção e buffers circulares.
 *
 * Diferença para o F0: a USART "clássica" do F4 tem um único registrador de
 * dados (DR) e um de status (SR). As flags de erro (ORE/FE/NE/PE) são limpas
 * pela sequência "ler SR, depois ler DR".
 */
#include "system.h"
#include "gpio.h"

#define RX_SIZE 128U
#define TX_SIZE 256U

static volatile uint8_t  rx_buf[RX_SIZE];
static volatile uint8_t  tx_buf[TX_SIZE];
static volatile uint32_t rx_head, rx_tail;
static volatile uint32_t tx_head, tx_tail;
volatile uint32_t uart_rx_overflow;

void uart_init(uint32_t baud)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    gpio_af(GPIOA, 2, 7);                 /* PA2 = USART2_TX (AF7) */
    gpio_af(GPIOA, 3, 7);                 /* PA3 = USART2_RX (AF7) */
    gpio_pull(GPIOA, 3, GPIO_PULLUP);

    USART2->CR1 = 0;
    USART2->CR2 = 0;
    USART2->CR3 = 0;
    /* 45 MHz / 115200 = 390,625 -> BRR = 391 = 0x187 (mantissa 24, fração 7/16) */
    USART2->BRR = (APB1Clock + baud / 2U) / baud;
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

    NVIC_SetPriority(USART2_IRQn, 5);
    NVIC_EnableIRQ(USART2_IRQn);
}

void USART2_IRQHandler(void)
{
    uint32_t sr = USART2->SR;

    /* RXNE ou erro: ler DR (após SR) entrega o byte e limpa ORE/FE/NE/PE */
    if (sr & (USART_SR_RXNE | USART_SR_ORE | USART_SR_FE | USART_SR_NE | USART_SR_PE)) {
        uint8_t c = (uint8_t)USART2->DR;
        if (sr & USART_SR_ORE) uart_rx_overflow++;
        if (sr & USART_SR_RXNE) {
            uint32_t next = (rx_head + 1U) & (RX_SIZE - 1U);
            if (next != rx_tail) {
                rx_buf[rx_head] = c;
                rx_head = next;
            } else {
                uart_rx_overflow++;
            }
        }
    }

    if ((USART2->CR1 & USART_CR1_TXEIE) && (sr & USART_SR_TXE)) {
        if (tx_tail != tx_head) {
            USART2->DR = tx_buf[tx_tail];
            tx_tail = (tx_tail + 1U) & (TX_SIZE - 1U);
        } else {
            USART2->CR1 &= ~USART_CR1_TXEIE;
        }
    }
}

void uart_putc(char c)
{
    uint32_t next = (tx_head + 1U) & (TX_SIZE - 1U);
    while (next == tx_tail) { }
    tx_buf[tx_head] = (uint8_t)c;
    tx_head = next;
    USART2->CR1 |= USART_CR1_TXEIE;
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
