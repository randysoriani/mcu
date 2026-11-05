/*
 * uart.c (F4) - console serial na USART2 (PA2 = TX, PA3 = RX, AF7).
 * Na Nucleo-F446RE esses pinos vão para o ST-LINK -> /dev/ttyACM0 no Linux.
 * Explicado no módulo 08.
 */
#include "system.h"
#include "gpio.h"

void uart_init(uint32_t baud)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    gpio_af(GPIOA, 2, 7);          /* PA2 -> USART2_TX */
    gpio_af(GPIOA, 3, 7);          /* PA3 -> USART2_RX */
    gpio_pull(GPIOA, 3, GPIO_PULLUP);

    USART2->CR1 = 0;
    /* BRR = USARTDIV em ponto fixo 12.4 = fPCLK / baud (com OVER8 = 0) */
    USART2->BRR = (APB1Clock + baud / 2U) / baud;
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void uart_putc(char c)
{
    while (!(USART2->SR & USART_SR_TXE)) { }
    USART2->DR = (uint8_t)c;
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
    uint32_t sr = USART2->SR;
    if (sr & (USART_SR_RXNE | USART_SR_ORE)) {
        /* ler SR e depois DR limpa RXNE e também ORE/FE/NE */
        return (int)(USART2->DR & 0xFFU);
    }
    return -1;
}
