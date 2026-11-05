/*
 * Módulo 16 - "DAC" sem DAC: PWM rápido + filtro RC
 *
 * A família TM4C NÃO tem conversor D/A. A técnica padrão é gerar um PWM de
 * alta frequência cujo duty cycle é a amostra, e filtrar com um passa-baixas:
 *
 *   pino PWM ──[ 1 kΩ ]──┬── saída analógica      fc = 1/(2π·R·C) ≈ 1,6 kHz
 *                       [100 nF]                  (2 estágios RC = bem melhor)
 *                        GND
 *
 *   123: M0PWM0 = PB6 (PCTL 4)      129: M0PWM2 = PF2 (PCTL 6)
 *   Portadora: SYSCLK / 512 -> 156 kHz (123) / 234 kHz (129), 9 bits
 *   TIMER0 a 32 kHz troca a amostra (CMPA) -> senoide de 1 kHz (32 pontos)
 *
 * DEMO 2: rampa triangular gerada por software.
 */
#include "system.h"
#include "gpio.h"

#ifndef DEMO
#define DEMO 1
#endif

#if defined(TM4C123)
#  define DPWM_GEN 0
#else
#  define DPWM_GEN 1
#endif
#define PWM_TOP   512U
#define N_SAMPLES 32U
#define F_SINE    1000U

/* 256 + 240*sin(2*pi*i/32): 9 bits */
__attribute__((unused)) static const uint16_t sine9[N_SAMPLES] = {
    256, 303, 348, 389, 426, 456, 478, 491, 496, 491, 478, 456, 426, 389, 348, 303,
    256, 209, 164, 123,  86,  56,  34,  21,  16,  21,  34,  56,  86, 123, 164, 209,
};

static volatile uint32_t idx;
static volatile int32_t  tri = 0, dir = 4;

static inline void dac_write(uint32_t v)      /* 0..511 */
{
    if (v < 1) v = 1;
    if (v > PWM_TOP - 2U) v = PWM_TOP - 2U;
    PWM0->GEN[DPWM_GEN].CMPA = (PWM_TOP - 1U) - v;
}

void TIMER0A_Handler(void)
{
    TIMER0->ICR = TIMER_INT_TATO;
#if DEMO == 1
    dac_write(sine9[idx]);
    idx = (idx + 1U) & (N_SAMPLES - 1U);
#else
    tri += dir;
    if (tri >= 508 || tri <= 4) dir = -dir;
    dac_write((uint32_t)tri);
#endif
}

int main(void)
{
    system_init();
    uart_init(115200);

    periph_enable(&SYSCTL_RCGCPWM, &SYSCTL_PRPWM, 0);
#if defined(TM4C123)
    SYSCTL_RCC &= ~SYSCTL_RCC_USEPWMDIV;        /* PWM a SYSCLK: portadora alta */
    gpio_port_enable(PORTB);
    gpio_af(GPIOB, 6, 4);
    const uint32_t out = 0;
#else
    PWM0->CC = 0;
    gpio_port_enable(PORTF);
    gpio_af(GPIOF, 2, 6);
    const uint32_t out = 2;
#endif
    PWM_GEN_TypeDef *g = &PWM0->GEN[DPWM_GEN];
    g->CTL  = 0;
    g->LOAD = PWM_TOP - 1U;
    g->CMPA = PWM_TOP / 2U;
    g->GENA = (PWM_ACT_HIGH << PWM_ACT_LOAD_Pos) | (PWM_ACT_LOW << PWM_ACT_CMPAD_Pos);
    g->CTL  = PWM_GEN_CTL_ENABLE;
    PWM0->ENABLE |= (1U << out);

    periph_enable(&SYSCTL_RCGCTIMER, &SYSCTL_PRTIMER, 0);
    TIMER0->CTL   = 0;
    TIMER0->CFG   = TIMER_CFG_32;
    TIMER0->TAMR  = TIMER_TAMR_PERIOD;
    TIMER0->TAILR = SystemCoreClock / (N_SAMPLES * F_SINE) - 1U;
    TIMER0->IMR   = TIMER_INT_TATO;
    TIMER0->CTL   = TIMER_CTL_TAEN;
    NVIC_SetPriority(TIMER0A_IRQn, 1);
    NVIC_EnableIRQ(TIMER0A_IRQn);

    kprintf("\n=== Modulo 16: DAC por PWM + RC ===\n");
    kprintf("portadora %u Hz, %u niveis, taxa de amostras %u Hz, %s\n",
            SystemCoreClock / PWM_TOP, PWM_TOP, N_SAMPLES * F_SINE,
            DEMO == 1 ? "senoide 1 kHz" : "triangular");
    for (;;) __WFI();
}
