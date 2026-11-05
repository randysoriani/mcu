/*
 * Módulo 19 - Watchdog (WDT0)
 *
 * O watchdog da TI é de DOIS estágios:
 *   1º timeout -> interrupção (WATCHDOG_Handler) - um "aviso"
 *   2º timeout sem atendimento -> RESET (se RESEN = 1)
 *
 * DEMO 1 (padrão): timeout de 500 ms; o laço alimenta a cada 200 ms.
 *   SEGURE o botão -> para de alimentar -> aviso + reset (~1 s).
 * DEMO 2: a ISR do 1º estágio imprime um aviso e NÃO alimenta, mostrando
 *   o tempo entre o aviso e o reset.
 *
 * WDT0 usa o SYSCLK. O WDT1 usa o PIOSC (independente do clock principal) e
 * exige esperar WRC após cada escrita.
 */
#include "system.h"
#include "gpio.h"

#ifndef DEMO
#define DEMO 1
#endif

static void print_reset_cause(void)
{
    uint32_t r = SYSCTL_RESC;
    kprintf("Causa do reset:");
    if (r & SYSCTL_RESC_POR)  kprintf(" POWER-ON");
    if (r & SYSCTL_RESC_EXT)  kprintf(" PINO-RST");
    if (r & SYSCTL_RESC_BOR)  kprintf(" BROWN-OUT");
    if (r & SYSCTL_RESC_WDT0) kprintf(" WATCHDOG0");
    if (r & SYSCTL_RESC_WDT1) kprintf(" WATCHDOG1");
    if (r & SYSCTL_RESC_SW)   kprintf(" SOFTWARE");
    if (r & SYSCTL_RESC_MOSCFAIL) kprintf(" FALHA-MOSC");
    kprintf("  (RESC = 0x%08X)\n", r);
    SYSCTL_RESC = 0;                     /* limpa para o próximo boot */
}

static volatile uint32_t aviso_ms;

void WATCHDOG_Handler(void)
{
#if DEMO == 1
    WDT0->ICR = 1;                       /* atende: limpa e recarrega */
#else
    aviso_ms = millis();
    kprintf("!!! WATCHDOG: 1o timeout em %u ms, reset no proximo !!!\n", aviso_ms);
    /* não limpa: a interrupção fica ativa, mas mascaramos no NVIC */
    NVIC_DisableIRQ(WATCHDOG_IRQn);
#endif
}

static void wdt_init_ms(uint32_t ms)
{
    periph_enable(&SYSCTL_RCGCWD, &SYSCTL_PRWD, 0);
    WDT0->LOCK = WDT_UNLOCK;             /* destrava os registradores */
    WDT0->LOAD = (SystemCoreClock / 1000U) * ms;
    WDT0->TEST |= WDT_TEST_STALL;        /* para quando o debugger para a CPU */
    WDT0->CTL  = WDT_CTL_RESEN | WDT_CTL_INTEN;   /* INTEN uma vez setado só sai com reset */
    WDT0->LOCK = 0;                      /* qualquer valor != chave trava */
    NVIC_EnableIRQ(WATCHDOG_IRQn);
}

static inline void wdt_feed(void)
{
    WDT0->LOCK = WDT_UNLOCK;
    WDT0->ICR  = 1;                      /* qualquer escrita recarrega o contador */
    WDT0->LOCK = 0;
}

int main(void)
{
    system_init();
    uart_init(115200);
    gpio_port_enable(LED_PORT_NUM);
    gpio_output(LED_PORT, LED_PIN);
    gpio_port_enable(BTN_PORT_NUM);
    gpio_input(BTN_PORT, BTN_PIN, GPIO_PULLUP);

    kprintf("\n=== Modulo 19: watchdog (DEMO %d) ===\n", DEMO);
    print_reset_cause();
    wdt_init_ms(500);
    kprintf("WDT0 ativo (500 ms por estagio). Segure o botao para travar.\n");

    for (uint32_t n = 0;; n++) {
#if DEMO == 1
        if (gpio_read(BTN_PORT, BTN_PIN)) {
            wdt_feed();
            gpio_toggle(LED_PORT, LED_PIN);
            if (n % 5 == 0) kprintf("vivo (%u)\n", n);
        } else {
            kprintf("travado...\n");
        }
        delay_ms(200);
#else
        (void)n;
        delay_ms(100);                   /* nunca alimenta */
#endif
    }
}
