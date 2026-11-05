/*
 * Módulo 02 - Startup
 *
 * Linkado com o startup em ASSEMBLY deste módulo e prova que ele funciona:
 *   1. .data copiada da Flash   2. .bss zerada
 *   3. SysTick_Handler forte substitui o alias fraco (Default_Handler)
 * LED pisca a 1 Hz se tudo OK; 10 Hz se .data/.bss estiverem errados.
 */
#include "device.h"
#include "system.h"   /* só para LED_PORT/LED_PIN */

uint32_t valor_data[4] = { 0x11111111, 0x22222222, 0x33333333, 0x44444444 }; /* .data */
uint32_t buffer_bss[16];                                                     /* .bss  */

static volatile uint32_t periodo_ms = 500;
static volatile uint32_t ticks;

void SysTick_Handler(void)
{
    if (++ticks >= periodo_ms) {
        ticks = 0;
        LED_PORT->DATA_BITS[1U << LED_PIN] ^= 0xFFU;
    }
}

static int startup_ok(void)
{
    static const uint32_t esperado[4] = { 0x11111111, 0x22222222, 0x33333333, 0x44444444 };
    for (int i = 0; i < 4; i++)  if (valor_data[i] != esperado[i]) return 0;
    for (int i = 0; i < 16; i++) if (buffer_bss[i] != 0) return 0;
    return 1;
}

int main(void)
{
    SYSCTL_RCGCGPIO |= (1U << LED_PORT_NUM);
    while (!(SYSCTL_PRGPIO & (1U << LED_PORT_NUM))) { }
    LED_PORT->DIR |= (1U << LED_PIN);
    LED_PORT->DEN |= (1U << LED_PIN);

    if (!startup_ok()) periodo_ms = 50;

    SysTick->LOAD = 16000000U / 1000U - 1U;     /* PIOSC 16 MHz após reset -> 1 ms */
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE | SysTick_CTRL_TICKINT | SysTick_CTRL_ENABLE;

    for (;;) { __WFI(); }
}
