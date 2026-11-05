/* Blink mínimo para o exemplo de Makefile do módulo 00 (EK-TM4C123GXL, LED verde PF3) */
#include "device.h"

int main(void)
{
    SYSCTL_RCGCGPIO |= (1U << 5);                    /* clock da porta F */
    while (!(SYSCTL_PRGPIO & (1U << 5))) { }
    GPIOF->DIR |= (1U << 3);
    GPIOF->DEN |= (1U << 3);
    for (;;) {
        GPIOF->DATA_BITS[1U << 3] ^= 0xFF;
        for (volatile uint32_t i = 0; i < 500000; i++) { }
    }
}
