/*
 * tick.c (ATmega328P) - base de tempo de 1 ms. Explicado no módulo 06.
 *
 * O AVR não tem SysTick: usamos o Timer0 em modo CTC
 *   16 MHz / 64 (prescaler) / 250 (OCR0A+1) = 1000 interrupções por segundo
 * (o mesmo timer que o Arduino usa para millis()).
 *
 * Com -DQEMU usa o Timer1, porque o QEMU só emula os timers de 16 bits.
 */
#include "system.h"

static volatile uint32_t s_ms;

#if defined(QEMU)
ISR(TIMER1_COMPA_vect)
#else
ISR(TIMER0_COMPA_vect)
#endif
{
    s_ms++;
}

void tick_init(void)
{
#if defined(QEMU)
    TCCR1A = 0;
    OCR1A  = 249;
    TCCR1B = (1U << WGM12) | (1U << CS11) | (1U << CS10);   /* CTC, ÷64 */
    TIMSK1 = (1U << OCIEA);
#else
    TCCR0A = (1U << WGM01);                     /* modo CTC: zera ao atingir OCR0A */
    OCR0A  = 249;                               /* 250 contagens */
    TCCR0B = (1U << CS01) | (1U << CS00);       /* prescaler ÷64 -> 250 kHz */
    TIMSK0 = (1U << OCIEA);                     /* interrupção na comparação A */
#endif
    sei();
}

/* Ler 4 bytes num processador de 8 bits leva 4 instruções: a interrupção
 * pode mudar s_ms no meio da leitura. Por isso desligamos as interrupções
 * durante a cópia (e restauramos o estado anterior). */
uint32_t millis(void)
{
    uint8_t sreg = SREG;
    cli();
    uint32_t v = s_ms;
    SREG = sreg;
    return v;
}

void delay_ms(uint32_t ms)
{
    uint32_t t0 = millis();
    while ((millis() - t0) < ms) { }
}

/* Atraso aproximado: o laço abaixo leva ~4 ciclos por volta (16 MHz -> 4 voltas/us) */
void delay_us(uint32_t us)
{
    while (us--) {
        for (uint8_t i = 0; i < (F_CPU / 4000000UL); i++) {
            __asm__ __volatile__ ("nop");
        }
    }
}

void system_init(void)
{
    clock_init();
    tick_init();
}
