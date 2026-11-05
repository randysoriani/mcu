/*
 * Módulo 02 - Startup
 *
 * Este programa é linkado com o startup em ASSEMBLY deste módulo
 * (f0/startup_f0.s ou f4/startup_f4.s) e prova que ele funciona:
 *
 *   1. Confere se .data foi copiada da Flash (valores iniciais corretos)
 *   2. Confere se .bss foi zerada
 *   3. Define SysTick_Handler -> o símbolo FORTE substitui o alias fraco
 *      que aponta para Default_Handler
 *
 * Resultado: LED pisca a 1 Hz se tudo deu certo; pisca rápido (10 Hz) se
 * .data ou .bss estiverem errados.
 */
#include "device.h"

uint32_t valor_data[4] = { 0x11111111, 0x22222222, 0x33333333, 0x44444444 }; /* .data */
uint32_t buffer_bss[16];                                                     /* .bss  */

static volatile uint32_t periodo_ms = 500;
static volatile uint32_t ticks;

/* Sobrescreve o handler fraco do startup */
void SysTick_Handler(void)
{
    if (++ticks >= periodo_ms) {
        ticks = 0;
        GPIOA->ODR ^= (1U << 5);
    }
}

static int startup_ok(void)
{
    static const uint32_t esperado[4] = { 0x11111111, 0x22222222, 0x33333333, 0x44444444 };
    for (int i = 0; i < 4; i++) {
        if (valor_data[i] != esperado[i]) return 0;
    }
    for (int i = 0; i < 16; i++) {
        if (buffer_bss[i] != 0) return 0;
    }
    return 1;
}

int main(void)
{
#if defined(STM32F0)
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    const uint32_t hclk = 8000000U;     /* HSI após reset */
#else
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    const uint32_t hclk = 16000000U;    /* HSI após reset */
#endif
    GPIOA->MODER = (GPIOA->MODER & ~(3U << 10)) | (1U << 10);   /* PA5 saída */

    if (!startup_ok()) {
        periodo_ms = 50;                /* erro: pisca rápido */
    }

    /* SysTick: interrupção a cada 1 ms */
    SysTick->LOAD = hclk / 1000U - 1U;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE | SysTick_CTRL_TICKINT | SysTick_CTRL_ENABLE;

    for (;;) {
        __WFI();      /* dorme entre interrupções */
    }
}
