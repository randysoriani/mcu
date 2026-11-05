/*
 * Módulo 09 - Timers básicos: base de tempo, interrupção e one-pulse
 *
 *   TIM6  -> interrupção de update a 1 Hz (pisca o LED)
 *   TIM2  -> contador livre de 32 bits a 1 MHz -> micros()
 *   TIM7  -> one-pulse mode para atrasos precisos em microssegundos
 *
 * Clock dos timers (regra do RCC): se o prescaler do APB for 1, TIMxCLK = PCLK;
 * caso contrário TIMxCLK = 2 x PCLK.
 *   F0: PCLK = 48 MHz (/1)  -> TIMxCLK = 48 MHz
 *   F4: PCLK1 = 45 MHz (/4) -> TIMxCLK = 90 MHz (TIM2..7, 12..14)
 */
#include "system.h"
#include "gpio.h"

static uint32_t tim_apb1_clock(void)
{
#if defined(STM32F0)
    return APB1Clock;                       /* APB /1 */
#else
    return (APB1Clock == SystemCoreClock) ? APB1Clock : 2U * APB1Clock;
#endif
}

static void tim_clocks_on(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM6EN | RCC_APB1ENR_TIM7EN;
    (void)RCC->APB1ENR;
}

/* ---------------- TIM6: interrupção periódica ---------------- */
static volatile uint32_t tim6_ticks;

void TIM6_DAC_IRQHandler(void)
{
    if (TIM6->SR & TIM_SR_UIF) {
        TIM6->SR = ~TIM_SR_UIF;               /* rc_w0: escreve 0 para limpar */
        tim6_ticks++;
        gpio_toggle(LED_PORT, LED_PIN);
    }
}

static void tim6_init_hz(uint32_t hz)
{
    uint32_t clk = tim_apb1_clock();
    TIM6->CR1  = 0;
    TIM6->PSC  = clk / 10000U - 1U;           /* contador anda a 10 kHz      */
    TIM6->ARR  = 10000U / hz - 1U;            /* estoura 'hz' vezes/segundo  */
    TIM6->EGR  = TIM_EGR_UG;                  /* carrega PSC/ARR agora       */
    TIM6->SR   = 0;                           /* UG setou UIF: limpa         */
    TIM6->DIER = TIM_DIER_UIE;
    TIM6->CR1  = TIM_CR1_ARPE | TIM_CR1_CEN;

    NVIC_SetPriority(TIM6_DAC_IRQn, 2);
    NVIC_EnableIRQ(TIM6_DAC_IRQn);
}

/* ---------------- TIM2: contador livre de 32 bits ---------------- */
static void tim2_free_running_1mhz(void)
{
    TIM2->CR1 = 0;
    TIM2->PSC = tim_apb1_clock() / 1000000U - 1U;   /* 1 tick = 1 us        */
    TIM2->ARR = 0xFFFFFFFFU;                        /* TIM2 é de 32 bits    */
    TIM2->EGR = TIM_EGR_UG;
    TIM2->CR1 = TIM_CR1_CEN;
}

static inline uint32_t micros(void) { return TIM2->CNT; }

/* ---------------- TIM7: one-pulse ---------------- */
static void tim7_init(void)
{
    TIM7->CR1 = TIM_CR1_OPM | TIM_CR1_URS;    /* para sozinho após 1 update */
    TIM7->PSC = tim_apb1_clock() / 1000000U - 1U;
    TIM7->EGR = TIM_EGR_UG;                   /* URS=1: UG não seta UIF     */
}

static void tim7_delay_us(uint16_t us)
{
    if (us < 2) return;
    TIM7->ARR = us - 1U;
    TIM7->CNT = 0;
    TIM7->CR1 |= TIM_CR1_CEN;
    while (TIM7->CR1 & TIM_CR1_CEN) { }       /* hardware zera CEN no fim */
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
    gpio_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT);

    tim_clocks_on();
    tim2_free_running_1mhz();
    tim7_init();
    tim6_init_hz(2);                          /* 2 toggles/s = LED a 1 Hz */

    kprintf("\n=== Modulo 09: timers basicos ===\n");
    kprintf("TIMxCLK (APB1) = %u Hz, TIM6 PSC = %u ARR = %u\n",
            tim_apb1_clock(), TIM6->PSC, TIM6->ARR);

    /* Mede atrasos do TIM7 usando o TIM2 */
    static const uint16_t testes[] = { 10, 100, 1000, 10000, 50000 };
    for (unsigned i = 0; i < sizeof testes / sizeof testes[0]; i++) {
        uint32_t t0 = micros();
        tim7_delay_us(testes[i]);
        uint32_t dt = micros() - t0;
        kprintf("tim7_delay_us(%5u) -> medido %5u us\n", testes[i], dt);
    }

    uint32_t last = 0;
    for (;;) {
        if (tim6_ticks != last) {
            last = tim6_ticks;
            kprintf("TIM6 tick %u, micros = %u\n", last, micros());
        }
        __WFI();
    }
}
