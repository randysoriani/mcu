/*
 * uart.c (F0) - console serial na USART2 (PA2 = TX, PA3 = RX, AF1).
 * Na Nucleo-F072RB esses pinos vão para o ST-LINK -> /dev/ttyACM0 no Linux.
 * Explicado no módulo 08.
 */
#include "system.h"
#include "gpio.h"

void uart_init(uint32_t baud)
{
    RCC->AHBENR  |= RCC_AHBENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    gpio_af(GPIOA, 2, 1);          /* PA2 -> USART2_TX */
    gpio_af(GPIOA, 3, 1);          /* PA3 -> USART2_RX */
    gpio_pull(GPIOA, 3, GPIO_PULLUP);

    USART2->CR1 = 0;                                   /* desabilita p/ configurar */
    USART2->BRR = (APB1Clock + baud / 2U) / baud;      /* oversampling x16 */
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;  /* 8N1 */
}

void uart_putc(char c)
{
    while (!(USART2->ISR & USART_ISR_TXE)) { }
    USART2->TDR = (uint8_t)c;
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
    if (USART2->ISR & USART_ISR_ORE) {
        USART2->ICR = USART_ICR_ORECF;    /* overrun trava a recepção até limpar */
    }
    if (USART2->ISR & USART_ISR_RXNE) {
        return (int)(USART2->RDR & 0xFFU);
    }
    return -1;
}
