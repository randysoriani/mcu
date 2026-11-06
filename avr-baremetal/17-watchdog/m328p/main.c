/*
 * Módulo 17 - Watchdog (ATmega328P)
 *
 *   Timeout de ~1 s. O programa "alimenta" o watchdog a cada 200 ms.
 *   SEGURE o botão (pino 2 ao GND): ele para de alimentar e o chip reinicia.
 *   No boot, imprime a causa do reset (registrador MCUSR).
 *
 * ATENÇÃO: depois de um reset pelo watchdog, ELE CONTINUA LIGADO com o
 * menor tempo (16 ms). Por isso o programa o desliga logo no começo.
 */
#include "system.h"

static uint8_t causa;

/* Executada antes do main (seção .init8 não existe aqui; chamamos primeiro no main) */
static void wdt_off(void)
{
    causa = MCUSR;
    MCUSR = 0;                                    /* precisa zerar WDRF para conseguir desligar */
    cli();
    WDTCSR = (1U << WDCE) | (1U << WDE);          /* sequência temporizada: até 4 ciclos */
    WDTCSR = 0;
}

static void wdt_on_1s(void)
{
    cli();
    wdt_reset();
    WDTCSR = (1U << WDCE) | (1U << WDE);
    WDTCSR = (1U << WDE) | (1U << WDP2) | (1U << WDP1);   /* WDP = 0110: 1 s, modo reset */
    sei();
}

int main(void)
{
    wdt_off();
    system_init();
    uart_init(115200);
    led_init();
    btn_init();

    kprintf("\n=== Modulo 17: watchdog ===\ncausa do reset:");
    if (causa & (1U << PORF))  kprintf(" POWER-ON");
    if (causa & (1U << EXTRF)) kprintf(" BOTAO-RESET");
    if (causa & (1U << BORF))  kprintf(" BROWN-OUT");
    if (causa & (1U << WDRF))  kprintf(" WATCHDOG");
    kprintf(" (MCUSR = 0x%02x)\n", causa);

    wdt_on_1s();
    for (uint16_t n = 0;; n++) {
        if (!btn_pressed()) { wdt_reset(); led_toggle(); if (n % 5 == 0) kprintf("vivo (%u)\n", n); }
        else kprintf("travado...\n");
        delay_ms(200);
    }
}
