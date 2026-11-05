/*
 * Módulo 09 - GPTM: timers básicos (iguais nos dois TM4C)
 *
 *   TIMER0 (32 bits, periódico) -> interrupção a 2 Hz: pisca o LED
 *   TIMER2 (32 bits, livre, contando PARA CIMA) -> micros()
 *   TIMER1 (32 bits, one-shot)  -> atrasos precisos em microssegundos
 *
 * Clock dos timers = SYSCLK (80 MHz no 123, 120 MHz no 129).
 * No modo 32 bits concatenado NÃO há prescaler: 1 tick = 1 ciclo de clock.
 */
#include "system.h"
#include "gpio.h"

static uint32_t mhz;
static volatile uint32_t tim0_ticks;

void TIMER0A_Handler(void)
{
    TIMER0->ICR = TIMER_INT_TATO;              /* limpa o timeout (escreve 1) */
    tim0_ticks++;
    gpio_toggle(LED_PORT, LED_PIN);
}

static void timer0_periodic_hz(uint32_t hz)
{
    TIMER0->CTL   = 0;                          /* desliga para configurar      */
    TIMER0->CFG   = TIMER_CFG_32;               /* timer A + B = um de 32 bits  */
    TIMER0->TAMR  = TIMER_TAMR_PERIOD;          /* periódico, contando p/ baixo */
    TIMER0->TAILR = SystemCoreClock / hz - 1U;  /* recarga                      */
    TIMER0->ICR   = TIMER_INT_TATO;
    TIMER0->IMR   = TIMER_INT_TATO;             /* interrupção no timeout       */
    TIMER0->CTL   = TIMER_CTL_TAEN | TIMER_CTL_TASTALL;   /* para no breakpoint */
    NVIC_SetPriority(TIMER0A_IRQn, 2);
    NVIC_EnableIRQ(TIMER0A_IRQn);
}

static void timer2_free_running(void)
{
    TIMER2->CTL   = 0;
    TIMER2->CFG   = TIMER_CFG_32;
    TIMER2->TAMR  = TIMER_TAMR_PERIOD | TIMER_TAMR_TACDIR;   /* conta PARA CIMA */
    TIMER2->TAILR = 0xFFFFFFFFU;
    TIMER2->CTL   = TIMER_CTL_TAEN;
}

static inline uint32_t cycles(void) { return TIMER2->TAV; }

static void timer1_delay_us(uint32_t us)
{
    TIMER1->CTL   = 0;
    TIMER1->CFG   = TIMER_CFG_32;
    TIMER1->TAMR  = TIMER_TAMR_ONESHOT;         /* conta até 0 e para sozinho */
    TIMER1->TAILR = us * mhz - 1U;
    TIMER1->ICR   = TIMER_INT_TATO;
    TIMER1->CTL   = TIMER_CTL_TAEN;
    while (!(TIMER1->RIS & TIMER_INT_TATO)) { } /* espera o timeout (sem IRQ) */
}

int main(void)
{
    system_init();
    uart_init(115200);
    mhz = SystemCoreClock / 1000000U;

    gpio_port_enable(LED_PORT_NUM);
    gpio_output(LED_PORT, LED_PIN);

    SYSCTL_RCGCTIMER |= (1U << 0) | (1U << 1) | (1U << 2);
    while ((SYSCTL_PRTIMER & 0x7U) != 0x7U) { }

    timer2_free_running();
    timer0_periodic_hz(2);

    kprintf("\n=== Modulo 09: timers (GPTM) ===\n");
    kprintf("SYSCLK = %u Hz, TIMER0 TAILR = %u\n", SystemCoreClock, TIMER0->TAILR);

    static const uint32_t testes[] = { 10, 100, 1000, 10000, 50000 };
    for (unsigned i = 0; i < sizeof testes / sizeof testes[0]; i++) {
        uint32_t t0 = cycles();
        timer1_delay_us(testes[i]);
        uint32_t dt = (cycles() - t0) / mhz;
        kprintf("timer1_delay_us(%5u) -> medido %5u us\n", testes[i], dt);
    }

    uint32_t last = 0;
    for (;;) {
        if (tim0_ticks != last) {
            last = tim0_ticks;
            kprintf("TIMER0 tick %u, TIMER2 = %u ciclos\n", last, cycles());
        }
        __WFI();
    }
}
