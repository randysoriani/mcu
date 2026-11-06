/*
 * Módulo 09 - Timers (AT32UC3A): o bloco TC, com 3 canais de 16 bits
 *
 *   Canal 0: modo "waveform", conta até RC e recomeça; interrupção de
 *            comparação com RC a 10 Hz -> LED inverte a cada 5 (= 1 Hz)
 *   Canal 1: contador livre a PBA/2 = 24 MHz, para medir tempos curtos
 *
 * Clocks do TC (campo TCCLKS): 0 = 32 kHz, 1 = PBA/2, 2 = PBA/8,
 * 3 = PBA/32, 4 = PBA/128.
 */
#include "system.h"

static volatile uint32_t ch0_ticks;

ISR_ATTR static void tc_handler(void)
{
    if (TC->CH[0].SR & TC_SR_CPCS) {               /* ler SR limpa as flags */
        if (++ch0_ticks % 5U == 0) led_toggle();
    }
}

int main(void)
{
    system_init();
    uart_init(115200);
    led_init();

    /* Canal 0: PBA/128 = 375 kHz; RC = 37500 -> 10 Hz */
    TC->CH[0].CCR = TC_CCR_CLKDIS;
    TC->CH[0].CMR = (4UL << 0)                    /* TCCLKS = PBA/128 */
                  | (1UL << 15)                   /* WAVE = 1 (modo waveform) */
                  | (2UL << 13);                  /* WAVSEL = 10: sobe até RC e recomeça */
    TC->CH[0].RC  = 37500;
    TC->CH[0].IER = TC_SR_CPCS;                   /* interrupção quando CV == RC */
    intc_register(IRQ_GRP_TC, tc_handler, 1);
    TC->CH[0].CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;  /* liga e zera o contador */

    /* Canal 1: contador livre a 24 MHz (16 bits: dá a volta a cada 2,7 ms) */
    TC->CH[1].CMR = (1UL << 0) | (1UL << 15);     /* PBA/2, waveform, WAVSEL = 00 (livre) */
    TC->CH[1].CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;

    kprintf("\n=== Modulo 09: timers (TC) ===\n");
    static const uint16_t testes[] = { 10, 100, 1000 };
    for (unsigned i = 0; i < 3; i++) {
        uint16_t a = (uint16_t)TC->CH[1].CV;
        delay_us(testes[i]);
        uint16_t b = (uint16_t)TC->CH[1].CV;
        kprintf("delay_us(%u) -> %u ciclos de 24 MHz = %u us\n", testes[i],
                (unsigned)(uint16_t)(b - a), (unsigned)((uint16_t)(b - a) / 24U));
    }
    uint32_t t = 0;
    for (;;) {
        if (millis() - t >= 1000) {
            t = millis();
            kprintf("canal 0: %lu interrupcoes\n", (unsigned long)ch0_ticks);
        }
    }
}
