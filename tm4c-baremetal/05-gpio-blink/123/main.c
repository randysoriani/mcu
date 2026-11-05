/*
 * Módulo 05 - Hello World: pisca o LED verde (PF3) da EK-TM4C123GXL.
 * Programa autocontido: só usa o header de registradores. Clock de reset (PIOSC 16 MHz).
 */
#include "device.h"

static void delay(volatile uint32_t n)
{
    while (n--) { __asm volatile ("nop"); }
}

int main(void)
{
    /* 1) Clock da porta F (bit 5 do RCGCGPIO) e espera o PRGPIO confirmar */
    SYSCTL_RCGCGPIO |= (1U << 5);
    while (!(SYSCTL_PRGPIO & (1U << 5))) { }

    /* 2) PF3 como saída digital: DIR = 1 e DEN = 1 (sem DEN o pino fica morto!) */
    GPIOF->DIR |= (1U << 3);
    GPIOF->DEN |= (1U << 3);

    for (;;) {
        /* Escrita MASCARADA: o endereço DATA_BITS[1<<3] (= base + 0x020) só
         * afeta o bit 3. Nada de read-modify-write. */
        GPIOF->DATA_BITS[1U << 3] = 0xFF;      /* acende  */
        delay(400000);
        GPIOF->DATA_BITS[1U << 3] = 0x00;      /* apaga   */
        delay(400000);
    }
}
