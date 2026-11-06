/*
 * Módulo 07 - USART: shell interativo com interrupções e buffer circular
 * No PC: picocom -b 115200 /dev/ttyACM0   (EVK1100: a porta do adaptador RS-232)
 * Comandos: help, led on|off|toggle, uptime, echo <texto>
 */
#include <string.h>
#include "system.h"

extern volatile uint16_t uart_rx_overflow;
#define LINE_MAX 48

static int readline(char *buf, uint8_t *len)
{
    int c;
    while ((c = uart_getc()) >= 0) {
        if (c == '\r' || c == '\n') {
            if (*len == 0) continue;
            buf[*len] = '\0';
            uart_puts("\n");
            return 1;
        } else if ((c == 0x08 || c == 0x7F) && *len > 0) {
            (*len)--; uart_puts("\b \b");
        } else if (c >= ' ' && *len < LINE_MAX - 1) {
            buf[(*len)++] = (char)c;
            uart_putc((char)c);
        }
    }
    return 0;
}

static void executa(const char *cmd)
{
    if      (strcmp(cmd, "help") == 0)       kprintf("comandos: led on|off|toggle, uptime, echo <texto>\n");
    else if (strcmp(cmd, "led on") == 0)     led_on();
    else if (strcmp(cmd, "led off") == 0)    led_off();
    else if (strcmp(cmd, "led toggle") == 0) led_toggle();
    else if (strcmp(cmd, "uptime") == 0) {
        uint32_t ms = millis();
        kprintf("%lu.%03lu s (erros/perdas de RX: %u)\n", (unsigned long)(ms / 1000UL),
                (unsigned long)(ms % 1000UL), uart_rx_overflow);
    }
    else if (strncmp(cmd, "echo ", 5) == 0)  kprintf("%s\n", cmd + 5);
    else kprintf("desconhecido: '%s' (digite help)\n", cmd);
}

int main(void)
{
    system_init();
    uart_init(115200);
    led_init();
    kprintf("\n=== Modulo 07: USART com interrupcoes ===\ndigite 'help'\n> ");

    char linha[LINE_MAX];
    uint8_t len = 0;
    for (;;) {
        if (readline(linha, &len)) { executa(linha); len = 0; kprintf("> "); }
    }
}
