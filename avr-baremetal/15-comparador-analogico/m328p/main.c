/*
 * Módulo 15 - Comparador analógico (só ATmega328P; o AT32UC3A não tem)
 *
 *   Entrada positiva: referência interna de 1,1 V (ACBG = 1)
 *   Entrada negativa: AIN1 = pino 7 (PD7) -> ligue o cursor de um potenciômetro
 *   ACO = 1 quando a tensão no pino 7 é MENOR que 1,1 V
 *   Interrupção a cada mudança da saída; o LED mostra o estado.
 */
#include "system.h"

static volatile uint16_t g_trocas;

ISR(ANALOG_COMP_vect)
{
    g_trocas++;
    if (ACSR & (1U << ACO)) led_on(); else led_off();
}

int main(void)
{
    system_init();
    uart_init(115200);
    led_init();

    DIDR1 = (1U << 1);                            /* desliga a entrada digital do AIN1 */
    ACSR  = (1U << ACBG)                          /* + = bandgap de 1,1 V */
          | (1U << ACI)                           /* limpa flag antiga (escrevendo 1) */
          | (1U << ACIE);                         /* interrupção; ACIS = 00: em qualquer mudança */
    sei();

    kprintf("\n=== Modulo 15: comparador analogico (pino 7 vs 1,1 V) ===\n");
    uint16_t ant = 0xFFFF;
    for (;;) {
        uint8_t s = SREG; cli(); uint16_t n = g_trocas; SREG = s;
        if (n != ant) {
            ant = n;
            kprintf("pino 7 %s 1,1 V (trocas: %u)\n", (ACSR & (1U << ACO)) ? "<" : ">", n);
        }
    }
}
