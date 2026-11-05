/*
 * Módulo 07 - UART: console interativo (shell) sobre a USART2.
 *
 * O driver (f0/uart_irq.c ou f4/uart_irq.c) usa interrupções e buffers
 * circulares; este arquivo é igual para os dois MCUs.
 *
 * No PC:  picocom -b 115200 /dev/ttyACM0
 * Comandos: help, led on|off|toggle, uptime, clock, echo <texto>, reset
 */
#include <string.h>
#include "system.h"
#include "gpio.h"

extern volatile uint32_t uart_rx_overflow;

#define LINE_MAX 64

/* Lê uma linha sem bloquear. Retorna 1 quando ENTER é recebido. */
static int readline(char *buf, uint32_t *len)
{
    int c;
    while ((c = uart_getc()) >= 0) {
        if (c == '\r' || c == '\n') {
            if (*len == 0) continue;           /* ignora linhas vazias / LF após CR */
            buf[*len] = '\0';
            uart_puts("\n");
            return 1;
        } else if (c == 0x08 || c == 0x7F) {    /* backspace / DEL */
            if (*len > 0) { (*len)--; uart_puts("\b \b"); }
        } else if (c >= ' ' && *len < LINE_MAX - 1) {
            buf[(*len)++] = (char)c;
            uart_putc((char)c);                /* eco */
        }
    }
    return 0;
}

static void executa(char *cmd)
{
    if (strcmp(cmd, "help") == 0) {
        kprintf("Comandos:\n"
                "  led on|off|toggle\n"
                "  uptime\n"
                "  clock\n"
                "  echo <texto>\n"
                "  reset\n");
    } else if (strcmp(cmd, "led on") == 0) {
        gpio_set(LED_PORT, LED_PIN);
    } else if (strcmp(cmd, "led off") == 0) {
        gpio_clear(LED_PORT, LED_PIN);
    } else if (strcmp(cmd, "led toggle") == 0) {
        gpio_toggle(LED_PORT, LED_PIN);
    } else if (strcmp(cmd, "uptime") == 0) {
        uint32_t ms = millis();
        kprintf("%u.%03u s  (overflows RX: %u)\n", ms / 1000U, ms % 1000U, uart_rx_overflow);
    } else if (strcmp(cmd, "clock") == 0) {
        kprintf("HCLK %u Hz, PCLK1 %u Hz, PCLK2 %u Hz\n", SystemCoreClock, APB1Clock, APB2Clock);
    } else if (strncmp(cmd, "echo ", 5) == 0) {
        kprintf("%s\n", cmd + 5);
    } else if (strcmp(cmd, "reset") == 0) {
        kprintf("reiniciando...\n");
        delay_ms(20);                          /* deixa o buffer de TX esvaziar */
        NVIC_SystemReset();
    } else {
        kprintf("comando desconhecido: '%s' (digite help)\n", cmd);
    }
}

int main(void)
{
    system_init();
    uart_init(115200);
    __enable_irq();

#if defined(STM32F0)
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
#else
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
#endif
    gpio_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT);

    kprintf("\n=== Modulo 07: UART (interrupcao + buffer circular) ===\n");
    kprintf("digite 'help'\n> ");

    char     linha[LINE_MAX];
    uint32_t len = 0;
    for (;;) {
        if (readline(linha, &len)) {
            executa(linha);
            len = 0;
            kprintf("> ");
        }
        __WFI();      /* acorda a cada byte recebido (IRQ) ou tick do SysTick */
    }
}
