/*
 * clock.c (UC3A) - 48 MHz a partir do cristal de 12 MHz da EVK1100.
 * Explicado no módulo 04.
 *
 *   OSC0 12 MHz ─► PLL0: (PLLMUL+1)/PLLDIV = 8/1 -> VCO 96 MHz ─► ÷2 -> 48 MHz
 *
 * No reset o UC3A roda do oscilador RC interno de ~115 kHz (bem lento!).
 */
#include "system.h"

uint32_t SystemCoreClock = 115000UL;

void clock_init(void)
{
    /* 1) OSC0: cristal acima de 8 MHz (MODE = 7), tempo de partida longo */
    PM->OSCCTRL0 = (7UL << 0) | (6UL << 8);
    PM->MCCTRL  |= PM_MCCTRL_OSC0EN;
    while (!(PM->POSCSR & PM_POSCSR_OSC0RDY)) { }

    /* 2) PLL0: fonte OSC0, MUL = 7 (x8), DIV = 1, PLLOPT = 011
     *    (faixa do VCO 80-180 MHz, saída dividida por 2), PLLCOUNT = 16 */
    PM->PLL0 = (16UL << 24) | (7UL << 16) | (1UL << 8) | (3UL << 2) | (0UL << 1) | 1UL;
    while (!(PM->POSCSR & PM_POSCSR_LOCK0)) { }

    /* 3) A Flash precisa de 1 wait state acima de 33 MHz: ANTES de acelerar */
    FLASHC->FCR |= FLASHC_FCR_FWS;

    /* 4) CPU, HSB, PBA e PBB sem divisor (todos a 48 MHz) */
    PM->CKSEL = 0;

    /* 5) Clock principal = PLL0 */
    PM->MCCTRL = (PM->MCCTRL & ~PM_MCCTRL_MCSEL_Msk) | PM_MCCTRL_MCSEL_PLL0;

    SystemCoreClock = 48000000UL;
}
