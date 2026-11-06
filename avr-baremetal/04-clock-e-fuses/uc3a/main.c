/*
 * Módulo 04 - Clock do AT32UC3A0512 (programa autocontido)
 *
 * 1. Pisca o LED0 5x com o oscilador RC de reset (~115 kHz: bem devagar)
 * 2. Liga o cristal de 12 MHz (OSC0) e passa a rodar dele
 * 3. Liga o PLL0 e passa a 48 MHz
 * Com o MESMO laço de atraso o LED fica cada vez mais rápido.
 */
#include "device.h"
#include "board.h"

static void delay_fixo(uint32_t n) { while (n--) { nop(); } }
static void blink(int vezes, uint32_t n)
{
    for (int i = 0; i < vezes * 2; i++) { led_toggle(); delay_fixo(n); }
}

int main(void)
{
    led_init();
    blink(5, 2000);                       /* ~115 kHz */

    /* OSC0: cristal > 8 MHz (MODE = 7), partida lenta; espera OSC0RDY */
    PM->OSCCTRL0 = (7UL << 0) | (6UL << 8);
    PM->MCCTRL  |= PM_MCCTRL_OSC0EN;
    while (!(PM->POSCSR & PM_POSCSR_OSC0RDY)) { }
    PM->MCCTRL = (PM->MCCTRL & ~PM_MCCTRL_MCSEL_Msk) | 1UL;   /* MCSEL = OSC0 */
    blink(5, 2000);                       /* 12 MHz: ~100x mais rápido */
    blink(5, 200000);                     /* compensa para dar para ver */

    /* PLL0: 12 MHz x 8 = 96 MHz (VCO), saída ÷2 = 48 MHz */
    PM->PLL0 = (16UL << 24) | (7UL << 16) | (1UL << 8) | (3UL << 2) | 1UL;
    while (!(PM->POSCSR & PM_POSCSR_LOCK0)) { }
    FLASHC->FCR |= FLASHC_FCR_FWS;        /* 1 wait state ANTES de passar de 33 MHz */
    PM->CKSEL = 0;                        /* CPU, HSB, PBA, PBB sem divisor */
    PM->MCCTRL = (PM->MCCTRL & ~PM_MCCTRL_MCSEL_Msk) | PM_MCCTRL_MCSEL_PLL0;

    for (;;) blink(1, 200000);            /* 48 MHz: 4x mais rápido que a 12 MHz */
}
