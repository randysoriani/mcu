/*
 * Módulo 18 - Sleep e economia de energia (ATmega328P)
 *
 * Menu pela serial:
 *   1 = IDLE: só a CPU para; qualquer interrupção acorda (inclusive o tick de 1 ms,
 *       por isso o desligamos antes)
 *   2 = POWER-DOWN: quase tudo para (~0,1 uA no chip, sem a placa). Só acordam
 *       fontes assíncronas: interrupção de mudança de pino, INT0 por nível,
 *       watchdog, TWI. Aqui: PCINT no pino 2 (PD2 = PCINT18) -> aperte o botão.
 * O registrador PRR desliga periféricos não usados.
 */
#include "system.h"

ISR(PCINT2_vect) { }                              /* só para acordar */

static void dorme(uint8_t modo_sm)
{
    while (!(UCSR0A & (1U << TXC0))) { }          /* espera a serial terminar de enviar */
    TIMSK0 = 0;                                   /* sem o tick de 1 ms */
    SMCR = (uint8_t)(modo_sm << SM0) | (1U << SE);   /* escolhe o modo e habilita o sleep */
    sei();
    sleep_cpu();                                  /* instrução SLEEP: dorme aqui */
    SMCR = 0;                                     /* acordou: desabilita o sleep */
    TIMSK0 = (1U << OCIEA);
}

int main(void)
{
    system_init();
    uart_init(115200);
    led_init();
    btn_init();
    PRR = (1U << PRTWI) | (1U << PRSPI) | (1U << PRADC) | (1U << PRTIM1) | (1U << PRTIM2);  /* desliga o que não usamos */

    PCMSK2 = (1U << 2);                           /* PCINT18 = PD2 */
    PCICR  = (1U << 2);
    sei();

    kprintf("\n=== Modulo 18: sleep ===\n");
    for (;;) {
        kprintf("1 = idle, 2 = power-down (acorda com o botao no pino 2) > ");
        int c;
        uint32_t t = 0;
        while ((c = uart_getc()) < 0) {
            if (millis() - t > 250) { t = millis(); led_toggle(); }
        }
        kprintf("%c\n", c);
        UCSR0A |= (1U << TXC0);                   /* TXC0 limpa escrevendo 1 */
        led_off();
        if (c == '1') { kprintf("IDLE...\n");       UCSR0A |= (1U << TXC0); dorme(0); kprintf("acordou do IDLE\n"); }
        if (c == '2') { kprintf("POWER-DOWN...\n"); UCSR0A |= (1U << TXC0); dorme(2); kprintf("acordou do POWER-DOWN\n"); }
    }
}
