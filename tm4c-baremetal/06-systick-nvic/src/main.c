/*
 * Módulo 06 - SysTick e NVIC (igual nos dois TM4C: o SysTick é do núcleo ARM)
 */
#include "system.h"
#include "gpio.h"

static volatile uint32_t g_ms;

void SysTick_Handler(void) { g_ms++; }
uint32_t millis(void) { return g_ms; }
void delay_ms(uint32_t ms)
{
    uint32_t t0 = g_ms;
    while ((g_ms - t0) < ms) { __WFI(); }
}

static void my_systick_init(uint32_t hz)
{
    uint32_t reload = SystemCoreClock / hz - 1U;       /* precisa caber em 24 bits */
    SysTick->CTRL = 0;
    SysTick->LOAD = reload & 0x00FFFFFFU;
    SysTick->VAL  = 0;
    SCB->SHP[11]  = (uint8_t)(1U << 5);                /* SysTick (exceção 15) prioridade 1 de 0..7 */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE | SysTick_CTRL_TICKINT | SysTick_CTRL_ENABLE;
}

#define DEMCR      (*(volatile uint32_t *)0xE000EDFCUL)
#define DWT_CTRL   (*(volatile uint32_t *)0xE0001000UL)
#define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004UL)

static uint32_t ciclos_systick(void (*f)(void))
{
    uint32_t load = SysTick->LOAD + 1U;
    uint32_t a = SysTick->VAL;
    f();
    uint32_t b = SysTick->VAL;
    return (a >= b) ? (a - b) : (a + load - b);       /* contador DECRESCENTE */
}

static volatile uint32_t sink;
static void trabalho(void) { for (uint32_t i = 0; i < 100; i++) sink += i; }

int main(void)
{
    clock_init();
    my_systick_init(1000);
    uart_init(115200);
    DEMCR |= (1U << 24);  DWT_CYCCNT = 0;  DWT_CTRL |= 1U;

    gpio_port_enable(LED_PORT_NUM);
    gpio_output(LED_PORT, LED_PIN);

    kprintf("\n=== Modulo 06: SysTick ===\n");
    kprintf("SYSCLK = %u Hz, LOAD = %u, CALIB = 0x%08X\n", SystemCoreClock, SysTick->LOAD, SysTick->CALIB);
    uint32_t c = ciclos_systick(trabalho);
    kprintf("trabalho(): %u ciclos (SysTick) = %u ns\n", c, c * 1000U / (SystemCoreClock / 1000000U));
    uint32_t t0 = DWT_CYCCNT;
    trabalho();
    kprintf("trabalho(): %u ciclos (DWT)\n", DWT_CYCCNT - t0);

    uint32_t t_led = 0, t_print = 0, segundos = 0;
    for (;;) {
        uint32_t agora = millis();
        if (agora - t_led >= 250)   { t_led += 250;   gpio_toggle(LED_PORT, LED_PIN); }
        if (agora - t_print >= 1000) { t_print += 1000; kprintf("uptime %u s (millis = %u)\n", ++segundos, agora); }
        __WFI();
    }
}
