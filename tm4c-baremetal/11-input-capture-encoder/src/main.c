/*
 * Módulo 11 - Captura de bordas (edge-time), contador de eventos e encoder (QEI)
 *
 *   make clean && make MCU=123 EXTRA=-DDEMO=2
 *
 * Gerador de teste (PWM do módulo 10, tick de 1 us):  PF2 nas duas placas
 *   123: M1PWM6 (PCTL 5)      129: M0PWM2 (PCTL 6)
 * Entrada de captura:
 *   123: T1CCP0 = PB4 (PCTL 7)   129: T0CCP0 = PD0 (PCTL 3)
 *   >>> jumper PF2 -> PB4 (123)  ou  PF2 -> PD0 (129) <<<
 *
 * DEMO 1: mede período e tempo em alto (timer em edge-time, ambas as bordas)
 * DEMO 2: conta bordas de subida (timer em edge-count) -> frequencímetro
 * DEMO 3: encoder em quadratura no QEI
 *         123: QEI1 PhA = PC5, PhB = PC6 (PCTL 6)   129: QEI0 PhA = PL1, PhB = PL2 (PCTL 6)
 */
#include "system.h"
#include "gpio.h"

#ifndef DEMO
#define DEMO 1
#endif

#if defined(TM4C123)
#  define GEN_PWM   PWM1
#  define GEN_GEN   3
#  define GEN_OUT   6
#  define CAP_TIMER TIMER1
#  define CAP_TNUM  1
#  define CAP_IRQn  TIMER1A_IRQn
#  define CAP_IRQHandler TIMER1A_Handler
#  define CAP_PORT  GPIOB
#  define CAP_PORTN PORTB
#  define CAP_PIN   4
#  define CAP_PCTL  7
#else
#  define GEN_PWM   PWM0
#  define GEN_GEN   1
#  define GEN_OUT   2
#  define CAP_TIMER TIMER0
#  define CAP_TNUM  0
#  define CAP_IRQn  TIMER0A_IRQn
#  define CAP_IRQHandler TIMER0A_Handler
#  define CAP_PORT  GPIOD
#  define CAP_PORTN PORTD
#  define CAP_PIN   0
#  define CAP_PCTL  3
#endif

static uint32_t mhz;

/* --------- gerador de teste: PWM com tick de 1 us --------- */
static void gen_init(void)
{
#if defined(TM4C123)
    periph_enable(&SYSCTL_RCGCPWM, &SYSCTL_PRPWM, 1);
    SYSCTL_RCC &= ~SYSCTL_RCC_USEPWMDIV;            /* PWM sem divisor ... */
    gpio_port_enable(PORTF);
    gpio_af(GPIOF, 2, 5);
#else
    periph_enable(&SYSCTL_RCGCPWM, &SYSCTL_PRPWM, 0);
    PWM0->CC = 0;
    gpio_port_enable(PORTF);
    gpio_af(GPIOF, 2, 6);
#endif
    GEN_PWM->GEN[GEN_GEN].CTL  = 0;
    GEN_PWM->GEN[GEN_GEN].GENA = (PWM_ACT_HIGH << PWM_ACT_LOAD_Pos) | (PWM_ACT_LOW << PWM_ACT_CMPAD_Pos);
    GEN_PWM->GEN[GEN_GEN].CTL  = PWM_GEN_CTL_ENABLE;
    GEN_PWM->ENABLE |= (1U << GEN_OUT);
}

static void gen_set(uint32_t period_us, uint32_t high_us)
{
    /* ... então usamos o PWM com clock = SYSCLK: ticks = us * MHz (cabe em 16 bits
     * só até ~800 us no 123). Para períodos maiores, ativamos /64. */
    uint32_t div = 1, ticks = period_us * mhz;
    if (ticks > 65535U) {
        div = 64; ticks = period_us * mhz / 64U;
#if defined(TM4C123)
        SYSCTL_RCC = (SYSCTL_RCC & ~(7U << SYSCTL_RCC_PWMDIV_Pos)) | SYSCTL_RCC_USEPWMDIV | (5U << SYSCTL_RCC_PWMDIV_Pos);
#else
        PWM0->CC = (1U << 8) | 5U;
#endif
    } else {
#if defined(TM4C123)
        SYSCTL_RCC &= ~SYSCTL_RCC_USEPWMDIV;
#else
        PWM0->CC = 0;
#endif
    }
    GEN_PWM->GEN[GEN_GEN].LOAD = ticks - 1U;
    GEN_PWM->GEN[GEN_GEN].CMPA = ticks - 1U - high_us * mhz / div;
}

__attribute__((unused)) static void cap_pin_timer_init(void)
{
    periph_enable(&SYSCTL_RCGCTIMER, &SYSCTL_PRTIMER, CAP_TNUM);
    gpio_port_enable(CAP_PORTN);
    gpio_af(CAP_PORT, CAP_PIN, CAP_PCTL);
}

#if DEMO == 1
/* ------------------------------------------------------------------ */
/* Timer A em 16 bits + prescaler de 8 bits como EXTENSÃO = 24 bits.
 * Em edge-time o contador desce de 0xFFFFFF; cada borda copia o valor
 * para TAR e gera CAE (capture event). */
static volatile uint32_t t_rise, period, high;
static volatile uint8_t  have;

void CAP_IRQHandler(void)
{
    CAP_TIMER->ICR = TIMER_INT_CAE;
    uint32_t t = CAP_TIMER->TAR & 0xFFFFFFU;
    if (gpio_read(CAP_PORT, CAP_PIN)) {            /* pino alto agora -> foi subida */
        period = (t_rise - t) & 0xFFFFFFU;         /* contador DECRESCENTE */
        t_rise = t;
        have = 1;
    } else {
        high = (t_rise - t) & 0xFFFFFFU;
    }
}

static void demo(void)
{
    static const uint16_t cfg[][2] = { {1000, 250}, {500, 400}, {20000, 1500}, {100, 10} };
    cap_pin_timer_init();
    CAP_TIMER->CTL   = 0;
    CAP_TIMER->CFG   = TIMER_CFG_16;
    CAP_TIMER->TAMR  = TIMER_TAMR_CAP | TIMER_TAMR_TACMR;   /* captura, edge-time */
    CAP_TIMER->TAILR = 0xFFFF;
    CAP_TIMER->TAPR  = 0xFF;                                /* extensão: 24 bits  */
    CAP_TIMER->ICR   = TIMER_INT_CAE;
    CAP_TIMER->IMR   = TIMER_INT_CAE;
    CAP_TIMER->CTL   = TIMER_CTL_TAEVENT_BOTH | TIMER_CTL_TAEN;
    NVIC_EnableIRQ(CAP_IRQn);
    kprintf("DEMO 1: edge-time (jumper PF2 -> pino de captura)\n");

    unsigned idx = 0;
    uint32_t t_cfg = 0, t_print = 0;
    for (;;) {
        if (millis() - t_cfg >= 2000) {
            t_cfg = millis();
            gen_set(cfg[idx][0], cfg[idx][1]);
            kprintf("\ngerando: periodo %u us, alto %u us\n", cfg[idx][0], cfg[idx][1]);
            idx = (idx + 1U) % 4U;
        }
        if (have && millis() - t_print >= 500) {
            t_print = millis();
            uint32_t p = period / mhz, h = high / mhz;
            if (p) kprintf("medido : periodo %5u us (%5u Hz), alto %5u us, duty %u%%\n",
                           p, 1000000U / p, h, h * 100U / p);
        }
    }
}

#elif DEMO == 2
/* ------------------------------------------------------------------ */
static void demo(void)
{
    cap_pin_timer_init();
    CAP_TIMER->CTL   = 0;
    CAP_TIMER->CFG   = TIMER_CFG_16;
    CAP_TIMER->TAMR  = TIMER_TAMR_CAP;              /* captura, edge-COUNT, conta p/ baixo */
    CAP_TIMER->TAILR = 0xFFFF;
    CAP_TIMER->TAPR  = 0xFF;                        /* 24 bits */
    CAP_TIMER->TAMATCHR = 0;                        /* para ao chegar em 0 (16 M bordas) */
    CAP_TIMER->TAPMR = 0;
    CAP_TIMER->CTL   = TIMER_CTL_TAEVENT_POS | TIMER_CTL_TAEN;
    kprintf("DEMO 2: contador de bordas (jumper PF2 -> pino de captura)\n");

    static const uint16_t per[] = { 1000, 250, 10000, 100 };
    uint32_t last = CAP_TIMER->TAR & 0xFFFFFFU;
    for (unsigned idx = 0;; idx = (idx + 1U) % 4U) {
        gen_set(per[idx], per[idx] / 2U);
        for (int s = 0; s < 3; s++) {
            delay_ms(1000);
            uint32_t now = CAP_TIMER->TAR & 0xFFFFFFU;
            kprintf("gerando %5u Hz -> contou %5u bordas em 1 s\n",
                    1000000U / per[idx], (last - now) & 0xFFFFFFU);
            last = now;
        }
    }
}

#elif DEMO == 3
/* ------------------------------------------------------------------ */
static void demo(void)
{
#if defined(TM4C123)
    QEI_TypeDef *q = QEI1;
    periph_enable(&SYSCTL_RCGCQEI, &SYSCTL_PRQEI, 1);
    gpio_port_enable(PORTC);
    gpio_af(GPIOC, 5, 6); gpio_af(GPIOC, 6, 6);
    GPIOC->PUR |= (1U << 5) | (1U << 6);
#else
    QEI_TypeDef *q = QEI0;
    periph_enable(&SYSCTL_RCGCQEI, &SYSCTL_PRQEI, 0);
    gpio_port_enable(PORTL);
    gpio_af(GPIOL, 1, 6); gpio_af(GPIOL, 2, 6);
    GPIOL->PUR |= (1U << 1) | (1U << 2);
#endif
    q->CTL    = 0;
    q->MAXPOS = 0xFFFFFFFFU;
    q->POS    = 0;
    q->CTL    = QEI_CTL_CAPMODE                  /* conta bordas de A e B (x4) */
              | QEI_CTL_FILTEN | (15U << 16)     /* filtro digital, 17 amostras */
              | QEI_CTL_ENABLE;
    kprintf("DEMO 3: encoder (QEI)\n");
    int32_t last = 0;
    for (;;) {
        int32_t pos = (int32_t)q->POS;
        if (pos != last) {
            kprintf("posicao %d  (%s)\n", pos / 4, (q->STAT & QEI_STAT_DIR) ? "anti-horario" : "horario");
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
    mhz = SystemCoreClock / 1000000U;
    kprintf("\n=== Modulo 11: captura / contador / encoder ===\n");
    gen_init();
    gen_set(1000, 250);
    demo();
}
