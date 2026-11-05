/*
 * Módulo 11 - Input capture, contador de eventos externos e encoder
 *
 * Escolha a demonstração na compilação:
 *   make clean && make MCU=f4 EXTRA=-DDEMO=1
 *
 * DEMO 1 (padrão) - "PWM input": mede período e largura de pulso.
 *     TIM2_CH1 (PA5/D13) gera um PWM de teste que muda a cada 2 s.
 *     TIM3_CH1 (PA6/D12) mede.   >>> Ligue um jumper D13 -> D12 <<<
 *
 * DEMO 2 - contador de eventos (external clock mode 1).
 *     TIM3 conta bordas de subida em PA6. Com o mesmo jumper, vira um
 *     frequencímetro (bordas por segundo).
 *
 * DEMO 3 - encoder em quadratura.
 *     Encoder rotativo nos pinos PA6 (A) e PA7 (B), comum no GND.
 *     O TIM3 conta para cima/baixo sozinho, com filtro digital.
 */
#include "system.h"
#include "gpio.h"

#ifndef DEMO
#define DEMO 1
#endif

#if defined(STM32F0)
#  define AF_TIM2_PA5 2U
#  define AF_TIM3     1U     /* PA6 = TIM3_CH1, PA7 = TIM3_CH2 */
#else
#  define AF_TIM2_PA5 1U
#  define AF_TIM3     2U
#endif

static uint32_t timclk(void)
{
#if defined(STM32F0)
    return APB1Clock;
#else
    return (APB1Clock == SystemCoreClock) ? APB1Clock : 2U * APB1Clock;
#endif
}

/* Gerador de teste: PWM no TIM2_CH1 (PA5), tick de 1 us */
static void test_pwm_set(uint32_t period_us, uint32_t high_us)
{
    TIM2->ARR  = period_us - 1U;
    TIM2->CCR1 = high_us;
}

static void test_pwm_init(void)
{
    gpio_af(GPIOA, 5, AF_TIM2_PA5);
    TIM2->PSC   = timclk() / 1000000U - 1U;
    TIM2->CCMR1 = (6U << 4) | (1U << 3);       /* PWM1 + preload */
    TIM2->CCER  = TIM_CCER_CC1E;
    test_pwm_set(1000, 250);
    TIM2->EGR   = TIM_EGR_UG;
    TIM2->CR1   = TIM_CR1_ARPE | TIM_CR1_CEN;
}

#if DEMO == 1
/* ------------------------------------------------------------------ */
static void pwm_input_init(void)
{
    gpio_af(GPIOA, 6, AF_TIM3);
    TIM3->CR1   = 0;
    TIM3->PSC   = timclk() / 1000000U - 1U;    /* resolução de 1 us */
    TIM3->ARR   = 0xFFFF;                      /* mede até 65 ms     */

    /* IC1 <- TI1 (CC1S = 01), IC2 <- TI1 (CC2S = 10): os dois canais
     * olham o MESMO pino, um na subida e outro na descida */
    TIM3->CCMR1 = (1U << 0) | (2U << 8);
    TIM3->CCER  = TIM_CCER_CC1E                     /* CC1: borda de subida  */
                | TIM_CCER_CC2E | TIM_CCER_CC2P;    /* CC2: borda de descida */

    /* Slave mode "reset" disparado por TI1FP1: a cada subida o CNT zera.
     * Logo: CCR1 = período, CCR2 = tempo em nível alto. */
    TIM3->SMCR  = (5U << 4)      /* TS  = 101: TI1FP1 */
                | (4U << 0);     /* SMS = 100: reset  */

    TIM3->EGR   = TIM_EGR_UG;
    TIM3->CR1   = TIM_CR1_CEN;
}

static void demo(void)
{
    static const uint16_t cfg[][2] = { {1000, 250}, {500, 400}, {20000, 1500}, {100, 10} };
    unsigned idx = 0;
    uint32_t t_cfg = 0, t_print = 0;
    pwm_input_init();
    kprintf("DEMO 1: PWM input (jumper D13 -> D12)\n");

    for (;;) {
        if (millis() - t_cfg >= 2000) {
            t_cfg = millis();
            test_pwm_set(cfg[idx][0], cfg[idx][1]);
            kprintf("\ngerando: periodo %u us, alto %u us\n", cfg[idx][0], cfg[idx][1]);
            idx = (idx + 1U) % 4U;
        }
        if (millis() - t_print >= 500 && (TIM3->SR & TIM_SR_CC1IF)) {
            t_print = millis();
            uint32_t per  = TIM3->CCR1;        /* ler CCR1 limpa CC1IF */
            uint32_t high = TIM3->CCR2;        /* erro de +-1 tick (sincronização) */
            if (per == 0) continue;
            kprintf("medido : periodo %5u us (%5u Hz), alto %5u us, duty %u%%\n",
                    per, 1000000U / per, high, high * 100U / per);
        }
    }
}

#elif DEMO == 2
/* ------------------------------------------------------------------ */
static void counter_init(void)
{
    gpio_af(GPIOA, 6, AF_TIM3);
    TIM3->CR1   = 0;
    TIM3->PSC   = 0;                    /* conta cada borda              */
    TIM3->ARR   = 0xFFFF;
    TIM3->CCMR1 = (1U << 0)             /* CC1S = 01: TI1 como entrada   */
                | (3U << 4);            /* IC1F = 0011: filtro de 8 amostras */
    TIM3->CCER  = 0;                    /* CC1P = 0: borda de subida     */
    TIM3->SMCR  = (5U << 4)             /* TS  = TI1FP1                  */
                | (7U << 0);            /* SMS = 111: external clock mode 1 */
    TIM3->CNT   = 0;
    TIM3->CR1   = TIM_CR1_CEN;
}

static void demo(void)
{
    counter_init();
    kprintf("DEMO 2: contador de bordas em PA6 (jumper D13 -> D12)\n");
    uint16_t last = 0;
    unsigned idx = 0;
    static const uint16_t per[] = { 1000, 250, 10000, 100 };
    for (;;) {
        test_pwm_set(per[idx], per[idx] / 2U);
        for (int s = 0; s < 3; s++) {
            delay_ms(1000);
            uint16_t now = (uint16_t)TIM3->CNT;
            uint16_t n   = (uint16_t)(now - last);  /* aritmética de 16 bits trata o wrap */
            last = now;
            kprintf("gerando %5u Hz -> contou %5u bordas em 1 s\n", 1000000U / per[idx], n);
        }
        idx = (idx + 1U) % 4U;
    }
}

#elif DEMO == 3
/* ------------------------------------------------------------------ */
static void encoder_init(void)
{
    gpio_af(GPIOA, 6, AF_TIM3);
    gpio_af(GPIOA, 7, AF_TIM3);
    gpio_pull(GPIOA, 6, GPIO_PULLUP);
    gpio_pull(GPIOA, 7, GPIO_PULLUP);

    TIM3->CR1   = 0;
    TIM3->PSC   = 0;
    TIM3->ARR   = 0xFFFF;
    /* CC1S = 01 (TI1), CC2S = 01 (TI2), filtros IC1F/IC2F = 1111 (máximo) */
    TIM3->CCMR1 = (1U << 0) | (0xFU << 4) | (1U << 8) | (0xFU << 12);
    TIM3->CCER  = 0;                    /* sem inversão */
    TIM3->SMCR  = (3U << 0);            /* SMS = 011: encoder mode 3 (x4) */
    TIM3->CNT   = 0;
    TIM3->CR1   = TIM_CR1_CEN;
}

static void demo(void)
{
    encoder_init();
    kprintf("DEMO 3: encoder em PA6/PA7\n");
    int16_t last = 0;
    for (;;) {
        int16_t pos = (int16_t)TIM3->CNT;   /* como int16: valores negativos */
        if (pos != last) {
            kprintf("posicao %d  (%s)\n", pos / 4, (TIM3->CR1 & TIM_CR1_DIR) ? "anti-horario" : "horario");
            last = pos;
        }
        delay_ms(20);
    }
}
#else
#  error "DEMO deve ser 1, 2 ou 3"
#endif

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
    (void)RCC->APB1ENR;

    kprintf("\n=== Modulo 11: input capture / contador / encoder ===\n");
    test_pwm_init();
    demo();
}
