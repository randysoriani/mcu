/*
 * Módulo 09 - Timers (ATmega328P)
 *
 *   Timer1 (16 bits) em CTC: interrupção a 2 Hz -> inverte o LED
 *   Timer2 (8 bits)  em CTC: interrupção a 1 kHz -> conta
 *   (o Timer0 já é a base de tempo da biblioteca, módulo 06)
 *
 * Fórmula do CTC: f = F_CPU / (prescaler * (OCR + 1))
 */
#include "system.h"

static volatile uint16_t t1_ticks;
static volatile uint32_t t2_ticks;

ISR(TIMER1_COMPA_vect) { t1_ticks++; led_toggle(); }
ISR(TIMER2_COMPA_vect) { t2_ticks++; }

int main(void)
{
    system_init();
    uart_init(115200);
    led_init();

    /* Timer1: ÷256 -> 62 500 Hz; OCR1A = 31249 -> 2 Hz */
    TCCR1A = 0;
    TCCR1B = (1U << WGM12) | (1U << CS12);        /* CTC (modo 4), ÷256 */
    OCR1A  = 31249;
    TIMSK1 = (1U << OCIEA);

    /* Timer2: ÷64 -> 250 kHz; OCR2A = 249 -> 1 kHz */
    TCCR2A = (1U << WGM21);                       /* CTC */
    TCCR2B = (1U << CS22);                        /* no Timer2, CS22 sozinho = ÷64 */
    OCR2A  = 249;
    TIMSK2 = (1U << OCIEA);
    sei();

    kprintf("\n=== Modulo 09: timers ===\n");
    uint32_t t = 0;
    for (;;) {
        if (millis() - t >= 1000) {
            t = millis();
            uint8_t s = SREG; cli();
            uint16_t a = t1_ticks; uint32_t b = t2_ticks; uint16_t cnt = TCNT1;
            SREG = s;
            kprintf("Timer1: %u interrupcoes (TCNT1 = %u) | Timer2: %lu | millis = %lu\n",
                    a, cnt, (unsigned long)b, (unsigned long)t);
        }
    }
}
