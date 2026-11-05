/*
 * Módulo 06 - SysTick e NVIC (exceções do núcleo)
 *
 * Implementa (sem usar common/lib/systick.c):
 *   - base de tempo de 1 ms com SysTick
 *   - millis()/delay_ms()
 *   - "multitarefa cooperativa" com temporizadores não bloqueantes
 *   - medição de ciclos com o SysTick (e com DWT->CYCCNT no Cortex-M4)
 *   - ajuste da prioridade da exceção SysTick
 */
#include "system.h"
#include "gpio.h"

/* -------------------------------------------------------------------------
 *  Base de tempo
 * ---------------------------------------------------------------------- */
static volatile uint32_t g_ms;     /* volatile: alterada dentro da ISR */

void SysTick_Handler(void)
{
    g_ms++;
}

uint32_t millis(void) { return g_ms; }

void delay_ms(uint32_t ms)
{
    uint32_t t0 = g_ms;
    while ((g_ms - t0) < ms) { __WFI(); }
}

static void set_systick_priority(uint32_t prio)
{
#if defined(STM32F0)
    /* SHPR3 (SCB->SHPR[1]) bits [31:24] = SysTick; M0 usa só os bits [7:6] */
    SCB->SHPR[1] = (SCB->SHPR[1] & 0x00FFFFFFU) | ((prio << 6) << 24);
#else
    /* SHP[n] = prioridade da exceção n+4 -> SysTick (15) = SHP[11]; bits [7:4] */
    SCB->SHP[11] = (uint8_t)(prio << 4);
#endif
}

static void my_systick_init(uint32_t hz)
{
    uint32_t reload = SystemCoreClock / hz - 1U;   /* precisa caber em 24 bits! */
    SysTick->CTRL = 0;                 /* desliga durante a configuração        */
    SysTick->LOAD = reload & 0x00FFFFFFU;
    SysTick->VAL  = 0;                 /* qualquer escrita zera VAL e COUNTFLAG */
    set_systick_priority(1);           /* um nível abaixo do máximo             */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE   /* clock = HCLK (não HCLK/8)       */
                  | SysTick_CTRL_TICKINT     /* gera exceção ao chegar em 0     */
                  | SysTick_CTRL_ENABLE;
}

/* -------------------------------------------------------------------------
 *  DWT cycle counter (só ARMv7-M: M3/M4/M7)
 * ---------------------------------------------------------------------- */
#if defined(STM32F4)
#define DEMCR      (*(volatile uint32_t *)0xE000EDFCUL)
#define DWT_CTRL   (*(volatile uint32_t *)0xE0001000UL)
#define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004UL)
static void dwt_init(void)
{
    DEMCR    |= (1U << 24);   /* TRCENA: liga os blocos de trace/DWT */
    DWT_CYCCNT = 0;
    DWT_CTRL |= 1U;           /* CYCCNTENA */
}
#endif

/* Conta ciclos com o próprio SysTick (vale para M0 e M4, se < 1 ms) */
static uint32_t ciclos_systick(void (*f)(void))
{
    uint32_t load = SysTick->LOAD + 1U;
    uint32_t a = SysTick->VAL;
    f();
    uint32_t b = SysTick->VAL;
    return (a >= b) ? (a - b) : (a + load - b);    /* contador DECRESCENTE */
}

static volatile uint32_t sink;
static void trabalho(void)
{
    for (uint32_t i = 0; i < 100; i++) sink += i;
}

int main(void)
{
    clock_init();
    my_systick_init(1000);
    uart_init(115200);
#if defined(STM32F4)
    dwt_init();
#endif

#if defined(STM32F0)
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
#else
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
#endif
    gpio_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT);

    kprintf("\n=== Modulo 06: SysTick ===\n");
    kprintf("HCLK = %u Hz, LOAD = %u, CALIB = 0x%08X\n",
            SystemCoreClock, SysTick->LOAD, SysTick->CALIB);

    uint32_t c = ciclos_systick(trabalho);
    kprintf("trabalho(): %u ciclos (SysTick) = %u ns\n",
            c, c * 1000U / (SystemCoreClock / 1000000U));
#if defined(STM32F4)
    uint32_t t0 = DWT_CYCCNT;
    trabalho();
    kprintf("trabalho(): %u ciclos (DWT)\n", DWT_CYCCNT - t0);
#endif

    /* Multitarefa cooperativa: cada "tarefa" roda quando chega a sua hora.
     * Nenhuma bloqueia; o laço principal fica livre. */
    uint32_t t_led = 0, t_print = 0, segundos = 0;
    for (;;) {
        uint32_t agora = millis();

        if (agora - t_led >= 250) {          /* tarefa 1: LED a 2 Hz */
            t_led += 250;
            gpio_toggle(LED_PORT, LED_PIN);
        }
        if (agora - t_print >= 1000) {       /* tarefa 2: log a 1 Hz */
            t_print += 1000;
            kprintf("uptime %u s (millis = %u)\n", ++segundos, agora);
        }
        __WFI();                             /* dorme até o próximo tick */
    }
}
