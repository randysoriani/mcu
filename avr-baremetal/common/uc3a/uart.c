/*
 * uart.c (UC3A) - USART0 por polling: PA00 = RXD0, PA01 = TXD0 (função A).
 * Na EVK1100 a USART0 vai ao conector DB9 "UART_0" (nível RS-232); use um
 * cabo serial ou um adaptador USB-RS232. Explicado no módulo 07.
 */
#include "system.h"

void uart_init(uint32_t baud)
{
    gpio_periph(PA(0), 0);                       /* RXD0 = função A */
    gpio_periph(PA(1), 0);                       /* TXD0 = função A */

    USART0->CR = USART_CR_RSTRX | USART_CR_RSTTX | USART_CR_RXDIS | USART_CR_TXDIS;
    USART0->MR = USART_MR_CHRL_8 | USART_MR_PAR_NONE;      /* 8N1, clock = PBA */
    /* baud = PBA / (16 * (CD + FP/8)) -> 48 MHz, 115200: CD = 26, FP = 0 */
    uint32_t div8 = (SystemCoreClock + baud) / (2UL * baud); /* = PBA*8/(16*baud), arredondado */
    USART0->BRGR = ((div8 >> 3) & 0xFFFFUL) | ((div8 & 7UL) << 16);
    USART0->CR = USART_CR_RXEN | USART_CR_TXEN;
}

void uart_putc(char c)
{
    while (!(USART0->CSR & USART_CSR_TXRDY)) { }
    USART0->THR = (uint8_t)c;
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
    if (USART0->CSR & USART_CSR_OVRE) USART0->CR = USART_CR_RSTSTA;
    if (!(USART0->CSR & USART_CSR_RXRDY)) return -1;
    return (int)(USART0->RHR & 0xFFU);
}
