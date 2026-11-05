/*
 * uart.c - console serial na UART0 (PA0 = RX, PA1 = TX, PCTL = 1), por polling.
 * Igual nas duas LaunchPads: a UART0 vai para o ICDI -> /dev/ttyACM0.
 * Explicado no módulo 07.
 */
#include "system.h"
#include "gpio.h"

void uart_init(uint32_t baud)
{
    periph_enable(&SYSCTL_RCGCUART, &SYSCTL_PRUART, 0);
    gpio_port_enable(PORTA);
    gpio_af(GPIOA, 0, 1);                 /* PA0 = U0RX */
    gpio_af(GPIOA, 1, 1);                 /* PA1 = U0TX */

    UART0->CTL = 0;                        /* desliga para configurar */
    /* BRD = clk / (16 * baud) em ponto fixo 16.6:
     * div = 64 * clk / (16 * baud) = 4 * clk / baud
     * IBRD = parte inteira, FBRD = parte fracionária em 1/64 */
    uint32_t div = (SystemCoreClock * 4U + baud / 2U) / baud;
    UART0->IBRD = div >> 6;
    UART0->FBRD = div & 0x3FU;
    UART0->LCRH = UART_LCRH_WLEN_8 | UART_LCRH_FEN;   /* 8N1, FIFOs de 16 bytes.
                                                       * Escrever LCRH também "trava" IBRD/FBRD */
    UART0->CC   = 0;                       /* clock = sysclk */
    UART0->CTL  = UART_CTL_UARTEN | UART_CTL_TXE | UART_CTL_RXE;
}

void uart_putc(char c)
{
    while (UART0->FR & UART_FR_TXFF) { }   /* espera espaço na FIFO de TX */
    UART0->DR = (uint8_t)c;
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
    if (UART0->FR & UART_FR_RXFE) return -1;
    uint32_t d = UART0->DR;                /* bits [11:8] = OE, BE, PE, FE deste byte */
    if (d & 0xF00U) UART0->RSR = 0;        /* escrever em RSR (ECR) limpa os erros */
    return (int)(d & 0xFFU);
}
