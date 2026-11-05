/*
 * clock.c (TM4C129) - 120 MHz a partir do cristal de 25 MHz + PLL.
 * Explicado no módulo 04.
 *
 *   MOSC 25 MHz /(N+1 = 5) = 5 MHz x MINT 96 = VCO 480 MHz /(PSYSDIV+1 = 4) = 120 MHz
 *
 * Diferente do 123, aqui os wait states da Flash/EEPROM (MEMTIM0) são
 * responsabilidade do software e são aplicados junto com a troca de clock.
 */
#include "system.h"

uint32_t SystemCoreClock = 16000000UL;   /* reset: PIOSC 16 MHz */

void clock_init(void)
{
    /* 1) Oscilador principal: cristal > 10 MHz (OSCRNG), tira do power-down */
    SYSCTL_MOSCCTL = (SYSCTL_MOSCCTL & ~(SYSCTL_MOSCCTL_NOXTAL | SYSCTL_MOSCCTL_PWRDN))
                   | SYSCTL_MOSCCTL_OSCRNG;
    while (!(SYSCTL_RIS & SYSCTL_RIS_MOSCPUPRIS)) { }

    /* 2) MOSC como fonte do oscilador e do PLL */
    SYSCTL_RSCLKCFG = SYSCTL_RSCLKCFG_OSCSRC_MOSC | SYSCTL_RSCLKCFG_PLLSRC_MOSC;

    /* 3) PLL: Q = 0 (/1), N = 4 (/5) ; MINT = 96, MFRAC = 0, liga o PLL */
    SYSCTL_PLLFREQ1 = (0U << 8) | 4U;
    SYSCTL_PLLFREQ0 = SYSCTL_PLLFREQ0_PLLPWR | 96U;

    /* 4) Wait states para 120 MHz (valores da tabela "MEMTIM0 Register
     *    Configuration" do datasheet): FWS = EWS = 5, FBCHT = EBCHT = 6
     *    (3.5 ciclos), bits 4 e 20 reservados = 1 */
    SYSCTL_MEMTIM0 = (6U << 22) | (5U << 16) | (1U << 20) | (6U << 6) | (1U << 4) | 5U;

    /* 5) Aplica a nova configuração do PLL e espera travar */
    SYSCTL_RSCLKCFG |= SYSCTL_RSCLKCFG_NEWFREQ;
    while (!(SYSCTL_PLLSTAT & SYSCTL_PLLSTAT_LOCK)) { }

    /* 6) Usa o PLL, divisor do sistema = 3 (/4) e aplica MEMTIM0 (MEMTIMU) */
    SYSCTL_RSCLKCFG = SYSCTL_RSCLKCFG_MEMTIMU | SYSCTL_RSCLKCFG_USEPLL
                    | SYSCTL_RSCLKCFG_OSCSRC_MOSC | SYSCTL_RSCLKCFG_PLLSRC_MOSC
                    | (3U << SYSCTL_RSCLKCFG_PSYSDIV_Pos);

    SystemCoreClock = 120000000UL;
}
