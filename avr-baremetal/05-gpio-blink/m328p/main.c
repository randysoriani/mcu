/*
 * Módulo 05 - Hello World: pisca o LED "L" do Arduino Uno (pino 13 = PB5).
 * Programa autocontido: só usa o arquivo de registradores.
 */
#include "device.h"

static void delay(volatile uint32_t n)
{
    while (n--) { __asm__ __volatile__ ("nop"); }
}

int main(void)
{
    /* No AVR de 8 bits o GPIO não precisa de "ligar o clock": basta
     * dizer que o pino é saída, escrevendo 1 no bit 5 do DDRB. */
    DDRB |= (1U << 5);

    for (;;) {
        PORTB |= (1U << 5);                 /* PB5 = 1: LED aceso   */
        delay(200000);
        PORTB &= (uint8_t)~(1U << 5);       /* PB5 = 0: LED apagado */
        delay(200000);
    }
}
