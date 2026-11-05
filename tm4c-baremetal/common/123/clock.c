/*
 * clock.c (TM4C123) - 80 MHz a partir do cristal de 16 MHz + PLL.
 * Explicado no módulo 04.
 *
 *   MOSC 16 MHz -> PLL (fixo em 400 MHz) -> /2 (DIV400=1 pula este /2) ...
 *   com DIV400 = 1: SYSCLK = 400 MHz / (SYSDIV2:SYSDIV2LSB + 1) = 400 / 5 = 80 MHz
 *
 * A Flash do TM4C123 ajusta os wait states sozinha (não há registrador).
 */
#include "system.h"

uint32_t SystemCoreClock = 16000000UL;   /* reset: PIOSC 16 MHz */

void clock_init(void)
{
    /* 1) RCC2 tem precedência sobre RCC (campos maiores, DIV400) */
    SYSCTL_RCC2 |= SYSCTL_RCC2_USERCC2;

    /* 2) BYPASS: enquanto configura, o sistema roda direto do oscilador */
    SYSCTL_RCC2 |= SYSCTL_RCC2_BYPASS2;

    /* 3) Cristal de 16 MHz, oscilador principal ligado, fonte = MOSC */
    SYSCTL_RCC   = (SYSCTL_RCC & ~(SYSCTL_RCC_XTAL_Msk | SYSCTL_RCC_MOSCDIS))
                 | SYSCTL_RCC_XTAL_16MHZ;
    SYSCTL_RCC2 &= ~SYSCTL_RCC2_OSCSRC2_Msk;      /* OSCSRC2 = 000: MOSC */

    /* 4) Liga o PLL */
    SYSCTL_RCC2 &= ~SYSCTL_RCC2_PWRDN2;

    /* 5) DIV400 + divisor 5 ({SYSDIV2,SYSDIV2LSB} = 4) -> 80 MHz */
    SYSCTL_RCC  |= SYSCTL_RCC_USESYSDIV;          /* usa o divisor do sistema */
    SYSCTL_RCC2 |= SYSCTL_RCC2_DIV400;
    SYSCTL_RCC2  = (SYSCTL_RCC2 & ~SYSCTL_RCC2_SYSDIV2_Msk) | (4U << SYSCTL_RCC2_SYSDIV2_Pos);

    /* 6) Espera o PLL travar */
    while (!(SYSCTL_RIS & SYSCTL_RIS_PLLLRIS)) { }

    /* 7) Tira o bypass: SYSCLK passa a vir do PLL */
    SYSCTL_RCC2 &= ~SYSCTL_RCC2_BYPASS2;

    SystemCoreClock = 80000000UL;
}
