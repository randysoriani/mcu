/*
 * Módulo 07 - UART: shell interativo sobre a UART0 (ICDI)
 * No PC: picocom -b 115200 /dev/ttyACM0
 */
#include <string.h>
#include "system.h"
#include "gpio.h"

extern volatile uint32_t uart_rx_overflow;
#define LINE_MAX 64

static int readline(char *buf, uint32_t *len)
{
    int c;
    while ((c = uart_getc()) >= 0) {
        if (c == '\r' || c == '\n') {
            if (*len == 0) continue;
            buf[*len] = '\0';
            uart_puts("\n");
            return 1;
        } else if (c == 0x08 || c == 0x7F) {
            if (*len > 0) { (*len)--; uart_puts("\b \b"); }
        } else if (c >= ' ' && *len < LINE_MAX - 1) {
            buf[(*len)++] = (char)c;
            uart_putc((char)c);
        }
    }
    return 0;
}

static void executa(char *cmd)
{
    if (strcmp(cmd, "help") == 0) {
        kprintf("Comandos: led on|off|toggle, uptime, clock, echo <texto>, reset\n");
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
        kprintf("SYSCLK %u Hz, IBRD %u FBRD %u\n", SystemCoreClock, UART0->IBRD, UART0->FBRD);
    } else if (strncmp(cmd, "echo ", 5) == 0) {
        kprintf("%s\n", cmd + 5);
    } else if (strcmp(cmd, "reset") == 0) {
        kprintf("reiniciando...\n");
        delay_ms(20);
        NVIC_SystemReset();
    } else {
        kprintf("comando desconhecido: '%s' (digite help)\n", cmd);
    }
}

int main(void)
{
    system_init();
    uart_init(115200);
    gpio_port_enable(LED_PORT_NUM);
    gpio_output(LED_PORT, LED_PIN);

    kprintf("\n=== Modulo 07: UART (interrupcao + buffer circular) ===\ndigite 'help'\n> ");
    char linha[LINE_MAX];
    uint32_t len = 0;
    for (;;) {
        if (readline(linha, &len)) { executa(linha); len = 0; kprintf("> "); }
        __WFI();
    }
}
