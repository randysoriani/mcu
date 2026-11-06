/*
 * Módulo 06 - Interrupções e base de tempo (implementação própria, sem tick.c)
 *
 *   ATmega328P: Timer0 em CTC a 1 kHz, vetor TIMER0_COMPA
 *   AT32UC3A:   COUNT/COMPARE do núcleo, grupo 0 do INTC
 *
 * Faz duas "tarefas" cooperativas (LED a 2 Hz, mensagem a 1 Hz) e mede
 * quanto tempo de CPU a interrupção consome.
 */
#include "system.h"

static volatile uint32_t g_ms;

#if defined(AVR8)
ISR(TIMER0_COMPA_vect)
{
    g_ms++;
}

static void meu_tick_init(void)
{
    TCCR0A = (1U << WGM01);                  /* CTC */
    OCR0A  = 249;                            /* 250 contagens */
    TCCR0B = (1U << CS01) | (1U << CS00);    /* ÷64 -> 250 kHz -> 1 kHz */
    TIMSK0 = (1U << OCIEA);
    sei();                                   /* bit I do SREG = 1 */
}

uint32_t millis(void)
{
    uint8_t s = SREG;                        /* leitura de 32 bits em 4 passos: protege */
    cli();
    uint32_t v = g_ms;
    SREG = s;
    return v;
}
#else
static uint32_t ciclos_por_ms;

ISR_ATTR static void compare_handler(void)
{
    mtsr(SYSREG_COUNT, 0);
    mtsr(SYSREG_COMPARE, ciclos_por_ms);     /* escrever COMPARE limpa o pedido */
    g_ms++;
}

static void meu_tick_init(void)
{
    ciclos_por_ms = SystemCoreClock / 1000UL;
    intc_init();
    intc_register(IRQ_GRP_COMPARE, compare_handler, 0);   /* nível 0 (o menos urgente) */
    mtsr(SYSREG_COUNT, 0);
    mtsr(SYSREG_COMPARE, ciclos_por_ms);
    irq_enable();                            /* GM = 0 no registrador de status */
}

uint32_t millis(void) { return g_ms; }
#endif

void delay_ms(uint32_t ms)
{
    uint32_t t0 = millis();
    while (millis() - t0 < ms) { }
}

int main(void)
{
    clock_init();
    meu_tick_init();
    uart_init(115200);
    led_init();

    kprintf("\n=== Modulo 06: interrupcoes e base de tempo ===\n");
    kprintf("clock da CPU = %lu Hz\n", (unsigned long)SystemCoreClock);

    /* Quantas voltas de um laço vazio cabem em 100 ms, com e sem o tick? */
    uint32_t t0 = millis(), voltas = 0;
    while (millis() - t0 < 100) voltas++;
    kprintf("laco vazio: %lu voltas em 100 ms (com o tick rodando)\n", (unsigned long)voltas);

    uint32_t t_led = 0, t_msg = 0, seg = 0;
    for (;;) {
        uint32_t agora = millis();
        if (agora - t_led >= 250)  { t_led += 250;  led_toggle(); }
        if (agora - t_msg >= 1000) { t_msg += 1000; kprintf("uptime %lu s\n", (unsigned long)++seg); }
    }
}
