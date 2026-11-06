/*
 * Módulo 02 - Startup: confere se o startup preparou .data e .bss e mostra
 * a troca da rotina de interrupção padrão por uma definida aqui.
 *   LED piscando a 1 Hz  = tudo certo
 *   LED piscando a 10 Hz = .data ou .bss com valores errados
 */
#include "device.h"
#include "board.h"

uint16_t valor_data[4] = { 0x1111, 0x2222, 0x3333, 0x4444 };   /* .data */
uint16_t buffer_bss[16];                                       /* .bss  */

static volatile uint16_t periodo_ms = 500;
static volatile uint16_t ticks;

static int startup_ok(void)
{
    static const uint16_t esperado[4] = { 0x1111, 0x2222, 0x3333, 0x4444 };
    for (int i = 0; i < 4; i++)  if (valor_data[i] != esperado[i]) return 0;
    for (int i = 0; i < 16; i++) if (buffer_bss[i] != 0) return 0;
    return 1;
}

static void a_cada_ms(void)
{
    if (++ticks >= periodo_ms) { ticks = 0; led_toggle(); }
}

#if defined(AVR8)
/* Substitui o __vector_14 "fraco" do startup.S (que reiniciaria o chip) */
ISR(TIMER0_COMPA_vect) { a_cada_ms(); }

int main(void)
{
    led_init();
    if (!startup_ok()) periodo_ms = 50;
    TCCR0A = (1U << WGM01);                     /* 1 ms com o Timer0 (módulo 06) */
    OCR0A  = 249;
    TCCR0B = (1U << CS01) | (1U << CS00);
    TIMSK0 = (1U << OCIEA);
    sei();
    for (;;) { }
}
#else
ISR_ATTR static void compare_handler(void)
{
    mtsr(SYSREG_COUNT, 0);
    mtsr(SYSREG_COMPARE, 115);                  /* ~1 ms com o oscilador RC de 115 kHz */
    a_cada_ms();
}

int main(void)
{
    led_init();
    if (!startup_ok()) periodo_ms = 50;
    intc_init();
    intc_register(IRQ_GRP_COMPARE, compare_handler, 0);
    mtsr(SYSREG_COUNT, 0);
    mtsr(SYSREG_COMPARE, 115);
    irq_enable();
    for (;;) { }
}
#endif
