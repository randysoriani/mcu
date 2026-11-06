/*
 * tick.c (UC3A) - base de tempo de 1 ms com os registradores COUNT/COMPARE
 * do núcleo AVR32 (o equivalente ao SysTick do ARM). Explicado no módulo 06.
 *
 * COUNT conta ciclos da CPU sozinho. Quando COUNT == COMPARE, a CPU gera a
 * interrupção do grupo 0. Escrever em COMPARE limpa a interrupção.
 */
#include "system.h"

static volatile uint32_t s_ms;
static uint32_t s_cycles_per_ms;

ISR_ATTR static void compare_handler(void)
{
    mtsr(SYSREG_COUNT, 0);                       /* recomeça a contagem */
    mtsr(SYSREG_COMPARE, s_cycles_per_ms);       /* rearma (e limpa o pedido) */
    s_ms++;
}

void tick_init(void)
{
    s_cycles_per_ms = SystemCoreClock / 1000UL;
    intc_init();
    intc_register(IRQ_GRP_COMPARE, compare_handler, 0);
    mtsr(SYSREG_COUNT, 0);
    mtsr(SYSREG_COMPARE, s_cycles_per_ms);
    irq_enable();
}

uint32_t millis(void)
{
    return s_ms;                                 /* 32 bits: leitura atômica nesta CPU */
}

void delay_ms(uint32_t ms)
{
    uint32_t t0 = s_ms;
    while ((s_ms - t0) < ms) { }
}

void delay_us(uint32_t us)
{
    /* laço calibrado de forma aproximada: ~4 ciclos por volta */
    volatile uint32_t n = us * (SystemCoreClock / 4000000UL);
    while (n--) { }
}

void system_init(void)
{
    clock_init();
    tick_init();
}
