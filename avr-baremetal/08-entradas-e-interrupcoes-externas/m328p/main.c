/*
 * Módulo 08 - Entradas digitais e interrupções externas (ATmega328P)
 *
 *   Botão entre o pino 2 (PD2 = INT0) e o GND -> interrupção INT0 na descida
 *   Fio/botão no pino 8 (PB0 = PCINT0) e GND  -> interrupção de mudança de pino
 *   O LED (pino 13) inverte a cada aperto no pino 2.
 */
#include "system.h"

static volatile uint16_t g_int0_count, g_pcint_count;
static volatile uint32_t g_last_ms;
static volatile uint8_t  g_event;

/* INT0: um pino só, com escolha de borda (subida, descida, ambas, nível) */
ISR(INT0_vect)
{
    uint32_t now = millis();
    if (now - g_last_ms >= 50) {          /* debounce: ignora quiques por 50 ms */
        g_last_ms = now;
        g_int0_count++;
        g_event = 1;
    }
}

/* PCINT0: qualquer mudança em qualquer pino habilitado da porta B */
ISR(PCINT0_vect)
{
    g_pcint_count++;
}

int main(void)
{
    system_init();
    uart_init(115200);
    led_init();

    btn_init();                                   /* PD2 entrada com pull-up */
    EICRA = (1U << 1);                            /* ISC01:ISC00 = 10 -> borda de descida */
    EIFR  = (1U << 0);                            /* limpa pendência antiga (escrevendo 1) */
    EIMSK = (1U << 0);                            /* habilita INT0 */

    DDRB  &= (uint8_t)~(1U << 0);                 /* PB0 entrada */
    PORTB |= (1U << 0);                           /* pull-up */
    PCMSK0 = (1U << 0);                           /* PCINT0 = PB0 */
    PCICR  = (1U << 0);                           /* habilita o grupo PCINT0..7 */
    sei();

    kprintf("\n=== Modulo 08: entradas e interrupcoes externas ===\n");
    kprintf("aperte o botao no pino 2; aterre o pino 8 para gerar PCINT\n");

    uint16_t pc_prev = 0;
    for (;;) {
        if (g_event) {
            g_event = 0;
            led_toggle();
            kprintf("INT0: %u apertos (pino 2 agora = %u)\n", g_int0_count, (PIND >> 2) & 1U);
        }
        uint8_t s = SREG; cli(); uint16_t pc = g_pcint_count; SREG = s;   /* leitura de 16 bits protegida */
        if (pc != pc_prev) {
            pc_prev = pc;
            kprintf("PCINT0: %u mudancas (pino 8 = %u)\n", pc, PINB & 1U);
        }
    }
}
