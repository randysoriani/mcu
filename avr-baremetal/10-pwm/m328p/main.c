/*
 * Módulo 10 - PWM (ATmega328P)
 *
 *   Timer2, Fast PWM 8 bits, saída OC2A = pino 11 (PB3): LED externo
 *   "respirando" (ligue LED + resistor de 330 Ω entre o pino 11 e o GND)
 *   Timer1, Fast PWM com TOP = ICR1 (modo 14), saída OC1A = pino 9 (PB1):
 *   50 Hz para servo, pulso de 1 a 2 ms
 */
#include "system.h"

int main(void)
{
    system_init();
    uart_init(115200);

    /* Timer2: Fast PWM (WGM2 = 011), não inverte OC2A (COM2A = 10), ÷64:
     * f = 16 MHz / (64 * 256) = 977 Hz; duty = OCR2A / 256 */
    DDRB  |= (1U << 3);
    TCCR2A = (1U << COM2A1) | (1U << WGM21) | (1U << WGM20);
    TCCR2B = (1U << CS22);
    OCR2A  = 0;

    /* Timer1: modo 14 (WGM13:10 = 1110), TOP = ICR1, ÷8 -> 2 MHz (0,5 us por contagem)
     * ICR1 = 39999 -> 20 ms (50 Hz); OCR1A = 2000..4000 -> 1..2 ms */
    DDRB  |= (1U << 1);
    TCCR1A = (1U << COM1A1) | (1U << WGM11);
    TCCR1B = (1U << WGM13) | (1U << WGM12) | (1U << CS11);
    ICR1   = 39999;
    OCR1A  = 3000;

    kprintf("\n=== Modulo 10: PWM ===\nTimer2: %u Hz no pino 11 | Timer1: 50 Hz no pino 9\n",
            (unsigned)(F_CPU / 64UL / 256UL));

    int16_t nivel = 0, passo = 1;
    uint16_t servo = 2000;
    uint32_t t_servo = 0;
    for (;;) {
        nivel += passo;
        if (nivel >= 100 || nivel <= 0) passo = (int16_t)-passo;
        OCR2A = (uint8_t)((uint16_t)(nivel * nivel) * 255U / 10000U);   /* curva quadrática */

        if (millis() - t_servo >= 200) {
            t_servo = millis();
            servo += 200;
            if (servo > 4000) servo = 2000;
            OCR1A = servo;                        /* 16 bits: o compilador escreve o byte alto primeiro */
        }
        delay_ms(10);
    }
}
