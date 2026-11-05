/*
 * Módulo 24 - GDB: programa com BUGS DE PROPÓSITO para caçar no depurador
 *
 * Menu pela serial (115200, /dev/ttyACM0), uma letra por cenário:
 *   m  media()       resultado errado         -> step / next / print / display
 *   o  overflow      variável muda "sozinha"  -> watch
 *   p  processa()    falha numa iteração      -> breakpoint condicional / dprintf
 *   e  máquina de estados                     -> print de enum, commands em breakpoint
 *   c  leitura de endereço inválido -> BusFault/HardFault  -> comando "fault"
 *   n  chamada por ponteiro nulo    -> UsageFault/HardFault
 *   z  divisão por zero (M4 com DIV_0_TRP)    -> UsageFault
 *   u  acesso desalinhado (falha no M0; no M4 com UNALIGN_TRP)
 *
 * Compile com OPT=-Og (Makefile do módulo) para o GDB seguir o código fonte
 * linha a linha sem as reordenações do -O2.
 */
#include <stdint.h>
#include "system.h"
#include "gpio.h"

/* ---------------- dados globais (bons para "print" e "x") ---------------- */
typedef enum { ESTADO_PARADO, ESTADO_ACELERANDO, ESTADO_CRUZEIRO, ESTADO_FREANDO } estado_t;

typedef struct {
    char     nome[8];
    uint32_t limite;          /* fica logo DEPOIS de nome[] na memória */
    uint16_t versao;
} config_t;

volatile uint32_t g_ticks_loop;
config_t          cfg = { "motor", 100, 3 };
estado_t          estado = ESTADO_PARADO;
int32_t           velocidade;

static const int32_t amostras[5] = { 10, 20, 30, 40, 50 };   /* média correta = 30 */
static const int32_t sentinela   = 1000;                      /* vizinho na memória */

/* ---------------- BUG 1: off-by-one ---------------- */
__attribute__((noinline)) int32_t media(const int32_t *v, uint32_t n)
{
    int32_t soma = 0;
    for (uint32_t i = 0; i <= n; i++) {     /* <= : lê v[n], que não existe */
        soma += v[i];
    }
    return soma / (int32_t)n;
}

/* ---------------- BUG 2: estouro de buffer ---------------- */
__attribute__((noinline)) static void copia_nome(char *dst, const char *src)
{
    while ((*dst++ = *src++) != '\0') { }   /* não sabe o tamanho do destino */
}

/* ---------------- BUG 3: falha numa entrada específica ---------------- */
__attribute__((noinline)) uint32_t processa(uint32_t i)
{
    uint32_t r = i * 3U + 1U;
    if ((i & 0x2E1U) == 0x2E1U) {           /* 737 = 0x2E1 (e outros) */
        r = 0;                              /* resultado errado escondido */
    }
    return r;
}

/* ---------------- máquina de estados ---------------- */
__attribute__((noinline)) void passo_estado(void)
{
    switch (estado) {
    case ESTADO_PARADO:     velocidade = 0;  estado = ESTADO_ACELERANDO; break;
    case ESTADO_ACELERANDO: velocidade += 10; if (velocidade >= 50) estado = ESTADO_CRUZEIRO; break;
    case ESTADO_CRUZEIRO:   if (++g_ticks_loop % 3 == 0) estado = ESTADO_FREANDO; break;
    case ESTADO_FREANDO:    velocidade -= 25; if (velocidade <= 0) estado = ESTADO_PARADO; break;
    }
}

/* ---------------- falhas provocadas ---------------- */
static void provoca_busfault(void)
{
    volatile uint32_t *p = (volatile uint32_t *)0x30000000UL;   /* nada mapeado aqui */
    (void)*p;
}

static void provoca_ponteiro_nulo(void)
{
    void (*volatile f)(void) = 0;          /* volatile: o compilador não "sabe" que é nulo */
    f();
}

static void provoca_div_zero(void)
{
#if defined(STM32F4)
    SCB->CCR |= (1U << 4);                 /* DIV_0_TRP: divisão por 0 vira UsageFault */
#endif
    volatile int32_t a = 10, b = 0;
    kprintf("10 / 0 = %d (no Cortex-M0 a libgcc devolve 0, sem falha)\n", a / b);
}

static void provoca_desalinhado(void)
{
#if defined(STM32F4)
    SCB->CCR |= (1U << 3);                 /* UNALIGN_TRP: M4 passa a reclamar também */
#endif
    static uint8_t buf[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    volatile uint32_t *p = (volatile uint32_t *)(void *)&buf[1];   /* endereço ímpar */
    kprintf("word desalinhada = 0x%08X\n", *p);
}

static void led_init(void)
{
#if defined(STM32F0)
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
#else
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
#endif
    gpio_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT);
}

static void menu(void)
{
    kprintf("\nm media | o overflow | p processa | e estados | c busfault | n nulo | z div0 | u desalinhado\n> ");
}

int main(void)
{
    system_init();
    uart_init(115200);
    led_init();
    kprintf("\n=== Modulo 24: GDB (bugs de proposito) ===\n");
    menu();

    uint32_t t = 0;
    for (;;) {
        int c = uart_getc();
        switch (c) {
        case 'm':
            kprintf("media = %d (esperado 30)\n", media(amostras, 5));
            (void)sentinela;
            break;
        case 'o':
            kprintf("antes: nome='%s' limite=%u\n", cfg.nome, cfg.limite);
            copia_nome(cfg.nome, "motor-principal");
            kprintf("depois: limite=%u  (quem mudou isso?)\n", cfg.limite);
            break;
        case 'p': {
            uint32_t soma = 0;
            for (uint32_t i = 0; i < 1000; i++) soma += processa(i);
            kprintf("soma = %u (esperado %u)\n", soma, 3U * (999U * 1000U / 2U) + 1000U);
            break;
        }
        case 'e':
            for (int k = 0; k < 12; k++) passo_estado();
            kprintf("estado = %d, velocidade = %d\n", (int)estado, velocidade);
            break;
        case 'c': provoca_busfault();      break;
        case 'n': provoca_ponteiro_nulo(); break;
        case 'z': provoca_div_zero();      break;
        case 'u': provoca_desalinhado();   break;
        default:  break;
        }
        if (c > 0) menu();

        if (millis() - t >= 500) { t = millis(); gpio_toggle(LED_PORT, LED_PIN); }
    }
}
