/*
 * Módulo 10 - PWM com timers de uso geral
 *
 *   TIM2_CH1 -> PA5 (LED LD2): PWM de 1 kHz, efeito "respiração" (fade)
 *   TIM3_CH1 -> PA6 (Arduino D12): 50 Hz, pulso de 1..2 ms (servo RC)
 *
 * Funções alternativas (datasheet, tabela "Alternate functions"):
 *            PA5/TIM2_CH1   PA6/TIM3_CH1
 *   F072        AF2            AF1
 *   F446        AF1            AF2
 */
#include "system.h"
#include "gpio.h"

#if defined(STM32F0)
#  define AF_TIM2_PA5 2U
#  define AF_TIM3_PA6 1U
#else
#  define AF_TIM2_PA5 1U
#  define AF_TIM3_PA6 2U
#endif

static uint32_t tim_apb1_clock(void)
{
#if defined(STM32F0)
    return APB1Clock;
#else
    return (APB1Clock == SystemCoreClock) ? APB1Clock : 2U * APB1Clock;
#endif
}

/*
 * Configura o canal 1 de um timer em PWM modo 1:
 *   saída = 1 enquanto CNT < CCR1, 0 de CCR1 até ARR
 *   f_pwm = TIMxCLK / ((PSC+1) * (ARR+1))
 *   duty  = CCR1 / (ARR+1)
 */
static void pwm_ch1_init(TIM_TypeDef *tim, uint32_t tick_hz, uint32_t period_ticks)
{
    tim->CR1   = 0;
    tim->PSC   = tim_apb1_clock() / tick_hz - 1U;
    tim->ARR   = period_ticks - 1U;
    tim->CCR1  = 0;

    /* CCMR1: CC1S[1:0] = 00 (saída), OC1PE = 1 (preload do CCR1),
     *        OC1M[6:4] = 110 (PWM modo 1) */
    tim->CCMR1 = (tim->CCMR1 & ~0xFFU) | (6U << 4) | (1U << 3);

    /* CCER: CC1E = 1 (habilita o pino), CC1P = 0 (ativo alto) */
    tim->CCER  = (tim->CCER & ~0xFU) | TIM_CCER_CC1E;

    tim->EGR   = TIM_EGR_UG;                  /* transfere PSC/ARR/CCR1 dos preloads */
    tim->CR1   = TIM_CR1_ARPE | TIM_CR1_CEN;  /* ARR com preload + liga             */
    /* Obs.: em timers AVANÇADOS (TIM1/TIM8) é preciso também BDTR |= MOE */
}

int main(void)
{
    system_init();
    uart_init(115200);

#if defined(STM32F0)
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
#else
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
#endif
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN;

    gpio_af(GPIOA, 5, AF_TIM2_PA5);
    gpio_af(GPIOA, 6, AF_TIM3_PA6);

    pwm_ch1_init(TIM2, 1000000U, 1000U);      /* tick 1 us, período 1000 us = 1 kHz */
    pwm_ch1_init(TIM3, 1000000U, 20000U);     /* tick 1 us, período 20 ms = 50 Hz   */
    TIM3->CCR1 = 1500;                        /* servo no centro (1,5 ms)           */

    kprintf("\n=== Modulo 10: PWM ===\n");
    kprintf("TIM2: PSC=%u ARR=%u -> %u Hz\n", TIM2->PSC, TIM2->ARR,
            tim_apb1_clock() / ((TIM2->PSC + 1U) * (TIM2->ARR + 1U)));
    kprintf("TIM3: PSC=%u ARR=%u -> %u Hz\n", TIM3->PSC, TIM3->ARR,
            tim_apb1_clock() / ((TIM3->PSC + 1U) * (TIM3->ARR + 1U)));

    int32_t  nivel = 0, passo = 1;
    uint32_t servo = 1000, t_servo = 0;
    for (;;) {
        /* Fade: o olho percebe brilho de forma ~logarítmica, então usamos
         * uma curva quadrática (correção de gama simples). 0..1000 */
        nivel += passo;
        if (nivel >= 100 || nivel <= 0) passo = -passo;
        TIM2->CCR1 = (uint32_t)(nivel * nivel) / 10U;

        /* Servo: varre 1,0 -> 2,0 ms em passos de 100 us a cada 200 ms */
        if (millis() - t_servo >= 200) {
            t_servo = millis();
            servo += 100;
            if (servo > 2000) servo = 1000;
            TIM3->CCR1 = servo;
        }
        delay_ms(10);
    }
}
