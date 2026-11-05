/*
 * systick.c - base de tempo de 1 ms com o SysTick (igual no M0 e no M4).
 * Explicado no módulo 06.
 */
#include "system.h"

static volatile uint32_t s_ticks;

void systick_init(void)
{
    SysTick->LOAD = SystemCoreClock / 1000U - 1U;   /* 1 ms */
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE | SysTick_CTRL_TICKINT | SysTick_CTRL_ENABLE;
}

void SysTick_Handler(void)
{
    s_ticks++;
}

uint32_t millis(void)
{
    return s_ticks;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = s_ticks;
    while ((s_ticks - start) < ms) {   /* subtração sem sinal: imune a overflow */
        __WFI();                       /* dorme até a próxima interrupção */
    }
}

/* Mede o tempo contando ciclos do próprio SysTick (precisa estar ligado) */
void delay_us(uint32_t us)
{
    uint32_t load    = SysTick->LOAD + 1U;
    uint32_t total   = us * (SystemCoreClock / 1000000U);
    uint32_t elapsed = 0;
    uint32_t prev    = SysTick->VAL;
    while (elapsed < total) {
        uint32_t now = SysTick->VAL;
        elapsed += (prev >= now) ? (prev - now) : (prev + load - now);
        prev = now;
    }
}

void system_init(void)
{
    clock_init();
    systick_init();
}
