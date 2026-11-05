/*
 * Módulo 10 - PWM com o módulo PWM dedicado
 *
 *            fade (1 kHz)                    servo (50 Hz)
 *   123: PWM1 gen3 A = M1PWM6 = PF2 (LED azul)   PWM0 gen0 A = M0PWM0 = PB6
 *   129: PWM0 gen0 A = M0PWM0 = PF0 (LED D4)     PWM0 gen1 A = M0PWM2 = PF2
 *
 * Clock do PWM = SYSCLK / 64:  123 -> 1,25 MHz   129 -> 1,875 MHz
 * Contadores de 16 bits, contando para BAIXO (LOAD -> 0).
 */
#include "system.h"
#include "gpio.h"

#if defined(TM4C123)
#  define FADE_PWM   PWM1
#  define FADE_GEN   3
#  define FADE_OUT   6          /* bit em ENABLE */
#  define SERVO_PWM  PWM0
#  define SERVO_GEN  0
#  define SERVO_OUT  0
#else
#  define FADE_PWM   PWM0
#  define FADE_GEN   0
#  define FADE_OUT   0
#  define SERVO_PWM  PWM0
#  define SERVO_GEN  1
#  define SERVO_OUT  2
#endif

static uint32_t pwm_clk;

static void pwm_clock_init(void)
{
#if defined(TM4C123)
    periph_enable(&SYSCTL_RCGCPWM, &SYSCTL_PRPWM, 0);
    periph_enable(&SYSCTL_RCGCPWM, &SYSCTL_PRPWM, 1);
    /* No 123 o divisor do PWM fica no RCC: USEPWMDIV + PWMDIV = 101 (/64) */
    SYSCTL_RCC = (SYSCTL_RCC & ~(7U << SYSCTL_RCC_PWMDIV_Pos))
               | SYSCTL_RCC_USEPWMDIV | (5U << SYSCTL_RCC_PWMDIV_Pos);
#else
    periph_enable(&SYSCTL_RCGCPWM, &SYSCTL_PRPWM, 0);
    /* No 129 o divisor fica no próprio PWM: PWMCC.USEPWM + PWMDIV = 101 (/64) */
    PWM0->CC = (1U << 8) | 5U;
#endif
    pwm_clk = SystemCoreClock / 64U;
}

/*
 * Gerador em modo down-count, saída A:
 *   no LOAD  -> nível 1 ; ao cruzar CMPA descendo -> nível 0
 *   tempo em 1 = LOAD - CMPA  -> duty = (LOAD - CMPA) / (LOAD + 1)
 */
static void pwm_gen_init(PWM_TypeDef *pwm, int gen, uint32_t period_ticks)
{
    PWM_GEN_TypeDef *g = &pwm->GEN[gen];
    g->CTL  = 0;                                /* desliga, modo down */
    g->LOAD = period_ticks - 1U;
    g->CMPA = period_ticks - 1U;                /* ~0% */
    g->GENA = (PWM_ACT_HIGH << PWM_ACT_LOAD_Pos) | (PWM_ACT_LOW << PWM_ACT_CMPAD_Pos);
    g->CTL  = PWM_GEN_CTL_ENABLE;
}

static void pwm_set_high_ticks(PWM_TypeDef *pwm, int gen, uint32_t high)
{
    PWM_GEN_TypeDef *g = &pwm->GEN[gen];
    if (high == 0) high = 1;
    if (high > g->LOAD) high = g->LOAD;
    g->CMPA = g->LOAD - high;                   /* atualizado no fim do período */
}

int main(void)
{
    system_init();
    uart_init(115200);
    pwm_clock_init();

#if defined(TM4C123)
    gpio_port_enable(PORTF);
    gpio_port_enable(PORTB);
    gpio_af(GPIOF, 2, 5);                       /* PF2 = M1PWM6 */
    gpio_af(GPIOB, 6, 4);                       /* PB6 = M0PWM0 */
#else
    gpio_port_enable(PORTF);
    gpio_af(GPIOF, 0, 6);                       /* PF0 = M0PWM0 */
    gpio_af(GPIOF, 2, 6);                       /* PF2 = M0PWM2 */
#endif

    uint32_t fade_period  = pwm_clk / 1000U;    /* 1 kHz  */
    uint32_t servo_period = pwm_clk / 50U;      /* 50 Hz  */
    pwm_gen_init(FADE_PWM, FADE_GEN, fade_period);
    pwm_gen_init(SERVO_PWM, SERVO_GEN, servo_period);
    FADE_PWM->ENABLE  |= (1U << FADE_OUT);      /* liga os pinos de saída */
    SERVO_PWM->ENABLE |= (1U << SERVO_OUT);

    kprintf("\n=== Modulo 10: PWM ===\n");
    kprintf("clock PWM = %u Hz, fade LOAD = %u, servo LOAD = %u\n",
            pwm_clk, FADE_PWM->GEN[FADE_GEN].LOAD, SERVO_PWM->GEN[SERVO_GEN].LOAD);

    int32_t  nivel = 0, passo = 1;
    uint32_t servo_us = 1000, t_servo = 0;
    for (;;) {
        nivel += passo;
        if (nivel >= 100 || nivel <= 0) passo = -passo;
        pwm_set_high_ticks(FADE_PWM, FADE_GEN, (uint32_t)(nivel * nivel) * fade_period / 10000U);

        if (millis() - t_servo >= 200) {
            t_servo = millis();
            servo_us += 100;
            if (servo_us > 2000) servo_us = 1000;
            pwm_set_high_ticks(SERVO_PWM, SERVO_GEN, servo_us * (pwm_clk / 1000U) / 1000U);
        }
        delay_ms(10);
    }
}
