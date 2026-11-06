/*
 * Módulo 17 - Watchdog (AT32UC3A)
 *
 *   Timeout = 2^(PSEL+1) / f_RC (115 kHz); PSEL = 16 -> ~1,1 s.
 *   O registrador CTRL é protegido: cada configuração exige duas escritas,
 *   a primeira com KEY = 0x55 e a segunda com KEY = 0xAA.
 *   SEGURE o botão do joystick: o programa para de alimentar e o chip reinicia.
 */
#include "system.h"

static void wdt_write_ctrl(uint32_t v)
{
    WDT->CTRL = v | (0x55UL << 24);
    WDT->CTRL = v | (0xAAUL << 24);
}

int main(void)
{
    system_init();
    uart_init(115200);
    led_init();
    btn_init();

    uint32_t rc = PM->RCAUSE;
    kprintf("\n=== Modulo 17: watchdog ===\ncausa do reset:");
    if (rc & PM_RCAUSE_POR)    kprintf(" POWER-ON");
    if (rc & PM_RCAUSE_BOD)    kprintf(" BROWN-OUT");
    if (rc & PM_RCAUSE_EXT)    kprintf(" PINO-RESET");
    if (rc & PM_RCAUSE_WDT)    kprintf(" WATCHDOG");
    if (rc & PM_RCAUSE_JTAG)   kprintf(" JTAG");
    if (rc & PM_RCAUSE_CPUERR) kprintf(" ERRO-DE-CPU");
    kprintf(" (RCAUSE = 0x%08lx)\n", (unsigned long)rc);

    wdt_write_ctrl((16UL << 8) | 1UL);            /* PSEL = 16, EN = 1 */
    for (uint32_t n = 0;; n++) {
        if (!btn_pressed()) { WDT->CLR = 0; led_toggle(); if (n % 5 == 0) kprintf("vivo (%lu)\n", (unsigned long)n); }
        else kprintf("travado...\n");
        delay_ms(200);
    }
}
