/*
 * Módulo 04 - Clock do TM4C123 (registradores RCC/RCC2)
 *
 * 1. Pisca o LED 5x com o clock de reset (PIOSC 16 MHz)
 * 2. Configura o PLL: SYSCLK = 400 MHz / PLL_DIV  (padrão PLL_DIV = 5 -> 80 MHz)
 * 3. Pisca com o MESMO laço -> 5x mais rápido
 *
 *   make clean && make MCU=123 EXTRA=-DPLL_DIV=8     (50 MHz)
 * O TM4C123 não tem pino de saída de clock (MCO). Meça pelo LED ou pela UART.
 */
#include "device.h"

#ifndef PLL_DIV
#define PLL_DIV 5      /* 5..128 ; 400/5 = 80 MHz é o máximo do chip */
#endif
#if PLL_DIV < 5
#error "PLL_DIV < 5 passaria de 80 MHz"
#endif

static void delay(volatile uint32_t n) { while (n--) { __asm volatile ("nop"); } }

static void blink(int vezes)
{
    for (int i = 0; i < vezes * 2; i++) {
        GPIOF->DATA_BITS[1U << 3] ^= 0xFF;
        delay(200000);
    }
}

static void pll_init(void)
{
    SYSCTL_RCC2 |= SYSCTL_RCC2_USERCC2;                    /* usa o RCC2       */
    SYSCTL_RCC2 |= SYSCTL_RCC2_BYPASS2;                    /* roda do oscilador enquanto configura */
    SYSCTL_RCC   = (SYSCTL_RCC & ~(SYSCTL_RCC_XTAL_Msk | SYSCTL_RCC_MOSCDIS))
                 | SYSCTL_RCC_XTAL_16MHZ;                  /* cristal da LaunchPad = 16 MHz */
    SYSCTL_RCC2 &= ~SYSCTL_RCC2_OSCSRC2_Msk;               /* fonte = MOSC     */
    SYSCTL_RCC2 &= ~SYSCTL_RCC2_PWRDN2;                    /* liga o PLL       */
    SYSCTL_RCC  |= SYSCTL_RCC_USESYSDIV;
    SYSCTL_RCC2 |= SYSCTL_RCC2_DIV400;                     /* divisor aplicado aos 400 MHz */
    SYSCTL_RCC2  = (SYSCTL_RCC2 & ~SYSCTL_RCC2_SYSDIV2_Msk)
                 | ((uint32_t)(PLL_DIV - 1) << SYSCTL_RCC2_SYSDIV2_Pos);
    while (!(SYSCTL_RIS & SYSCTL_RIS_PLLLRIS)) { }         /* PLL travado      */
    SYSCTL_RCC2 &= ~SYSCTL_RCC2_BYPASS2;                   /* SYSCLK = PLL     */
}

int main(void)
{
    SYSCTL_RCGCGPIO |= (1U << PORTF);
    while (!(SYSCTL_PRGPIO & (1U << PORTF))) { }
    GPIOF->DIR |= (1U << 3);
    GPIOF->DEN |= (1U << 3);

    blink(5);            /* 16 MHz */
    delay(1000000);
    pll_init();
    for (;;) blink(1);   /* 400/PLL_DIV MHz */
}
