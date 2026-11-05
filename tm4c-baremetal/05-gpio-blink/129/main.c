/*
 * Módulo 05 - Hello World: pisca os LEDs D1 (PN1) e D2 (PN0) da EK-TM4C1294XL,
 * alternadamente. Clock de reset (PIOSC 16 MHz).
 */
#include "device.h"

static void delay(volatile uint32_t n)
{
    while (n--) { __asm volatile ("nop"); }
}

int main(void)
{
    /* Porta N = bit 12 do RCGCGPIO */
    SYSCTL_RCGCGPIO |= (1U << 12);
    while (!(SYSCTL_PRGPIO & (1U << 12))) { }

    GPION->DIR |= (1U << 1) | (1U << 0);
    GPION->DEN |= (1U << 1) | (1U << 0);

    /* Endereço mascarado com os bits 0 e 1: DATA_BITS[0x3] */
    GPION->DATA_BITS[0x3] = 0x2;                 /* D1 aceso, D2 apagado */
    for (;;) {
        GPION->DATA_BITS[0x3] ^= 0x3;            /* inverte os dois */
        delay(400000);
    }
}
