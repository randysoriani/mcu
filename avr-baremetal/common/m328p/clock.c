/*
 * clock.c (ATmega328P) - garante o clock máximo da placa (16 MHz).
 * Explicado no módulo 04.
 *
 * No AVR de 8 bits a FONTE de clock (cristal, oscilador interno...) é
 * escolhida pelos FUSES, gravados pelo programador, não pelo programa.
 * O software só pode mexer no divisor CLKPR (÷1 a ÷256).
 */
#include "system.h"

uint32_t SystemCoreClock = F_CPU;

void clock_init(void)
{
    /* CLKPR exige uma sequência temporizada: escrever CLKPCE=1 e, em até
     * 4 ciclos, o novo divisor. Interrupções desligadas para não atrasar. */
    uint8_t sreg = SREG;
    cli();
    CLKPR = (1U << CLKPCE);
    CLKPR = 0;                       /* divisor ÷1 (desfaz o fuse CKDIV8, se gravado) */
    SREG = sreg;
    SystemCoreClock = F_CPU;
}
