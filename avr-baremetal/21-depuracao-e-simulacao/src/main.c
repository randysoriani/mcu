/*
 * Módulo 21 - Depuração e simulação: programa com bugs de propósito
 * (o mesmo código nos dois chips)
 *
 *   m  media()      resultado errado          -> break, step, print, finish
 *   o  overflow     variável muda "sozinha"   -> watch
 *   p  processa()   falha numa iteração       -> break condicional
 *
 * ATmega328P: rode no QEMU com o GDB (veja o README):
 *   make MCU=m328p EXTRA="-DQEMU -O0" && make MCU=m328p qemu-gdb
 */
#include <string.h>
#include "system.h"

typedef struct {
    char     nome[6];
    uint16_t limite;                 /* logo depois de nome[] na memória */
} config_t;

config_t cfg = { "motor", 100 };
static const int16_t amostras[5] = { 10, 20, 30, 40, 50 };    /* média correta = 30 */

__attribute__((noinline)) int16_t media(const int16_t *v, uint8_t n)
{
    int16_t soma = 0;
    for (uint8_t i = 0; i <= n; i++) soma += v[i];             /* BUG: <= */
    return soma / n;
}

__attribute__((noinline)) static void copia_nome(char *dst, const char *src)
{
    while ((*dst++ = *src++) != '\0') { }                     /* BUG: não conhece o tamanho */
}

__attribute__((noinline)) uint16_t processa(uint16_t i)
{
    uint16_t r = (uint16_t)(i * 3U + 1U);
    if ((i & 0x2E1U) == 0x2E1U) r = 0;                        /* BUG escondido: 737 = 0x2E1 */
    return r;
}

int main(void)
{
    system_init();
    uart_init(115200);
    kprintf("\n=== Modulo 21: depuracao ===\nm media | o overflow | p processa\n> ");
    for (;;) {
        int c = uart_getc();
        if (c == 'm') kprintf("media = %d (esperado 30)\n> ", media(amostras, 5));
        if (c == 'o') {
            copia_nome(cfg.nome, "motor-principal");
            kprintf("limite = %u (era 100)\n> ", cfg.limite);
        }
        if (c == 'p') {
            uint32_t soma = 0;
            for (uint16_t i = 0; i < 1000; i++) soma += processa(i);
            kprintf("soma = %lu (esperado 1499500)\n> ", (unsigned long)soma);
        }
    }
}
