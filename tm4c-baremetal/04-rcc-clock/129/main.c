/*
 * Módulo 04 - Clock do TM4C129 (MOSC + PLL + MEMTIM0 + DIVSCLK)
 *
 * 1. Pisca D1 5x com o clock de reset (PIOSC 16 MHz)
 * 2. SYSCLK = VCO 480 MHz / (PSYSDIV + 1)   (padrão PSYSDIV = 3 -> 120 MHz)
 *    com os wait states da Flash/EEPROM ajustados para a frequência escolhida
 * 3. Coloca SYSCLK/12 no pino PQ4 (DIVSCLK) para medir com osciloscópio
 *
 *   make clean && make MCU=129 EXTRA=-DPSYSDIV=7     (60 MHz)
 */
#include "device.h"

#ifndef PSYSDIV
#define PSYSDIV 3
#endif
#if PSYSDIV < 3
#error "PSYSDIV < 3 passaria de 120 MHz"
#endif
#define SYSCLK_HZ (480000000UL / (PSYSDIV + 1))

static void delay(volatile uint32_t n) { while (n--) { __asm volatile ("nop"); } }

static void blink(int vezes)
{
    for (int i = 0; i < vezes * 2; i++) {
        GPION->DATA_BITS[1U << 1] ^= 0xFF;
        delay(300000);
    }
}

/* Tabela "MEMTIM0 Register Configuration versus Frequency" do datasheet */
static uint32_t memtim0_for(uint32_t hz)
{
    uint32_t ws, bcht;
    if      (hz <=  16000000UL) { ws = 0; bcht = 0; }
    else if (hz <=  40000000UL) { ws = 1; bcht = 2; }
    else if (hz <=  60000000UL) { ws = 2; bcht = 3; }
    else if (hz <=  80000000UL) { ws = 3; bcht = 4; }
    else if (hz <= 100000000UL) { ws = 4; bcht = 5; }
    else                        { ws = 5; bcht = 6; }
    /* EBCHT[25:22] EWS[19:16] (EEPROM) | FBCHT[9:6] FWS[3:0] (Flash) | bits 20 e 4 = 1 */
    return (bcht << 22) | (ws << 16) | (1U << 20) | (bcht << 6) | (1U << 4) | ws;
}

static void pll_init(void)
{
    SYSCTL_MOSCCTL = (SYSCTL_MOSCCTL & ~(SYSCTL_MOSCCTL_NOXTAL | SYSCTL_MOSCCTL_PWRDN))
                   | SYSCTL_MOSCCTL_OSCRNG;             /* cristal de 25 MHz (> 10 MHz) */
    while (!(SYSCTL_RIS & SYSCTL_RIS_MOSCPUPRIS)) { }

    SYSCTL_RSCLKCFG = SYSCTL_RSCLKCFG_OSCSRC_MOSC | SYSCTL_RSCLKCFG_PLLSRC_MOSC;
    SYSCTL_PLLFREQ1 = 4U;                               /* N = 4 -> 25/5 = 5 MHz */
    SYSCTL_PLLFREQ0 = SYSCTL_PLLFREQ0_PLLPWR | 96U;     /* MINT = 96 -> VCO 480 MHz */
    SYSCTL_MEMTIM0  = memtim0_for(SYSCLK_HZ);
    SYSCTL_RSCLKCFG |= SYSCTL_RSCLKCFG_NEWFREQ;
    while (!(SYSCTL_PLLSTAT & SYSCTL_PLLSTAT_LOCK)) { }
    SYSCTL_RSCLKCFG = SYSCTL_RSCLKCFG_MEMTIMU | SYSCTL_RSCLKCFG_USEPLL
                    | SYSCTL_RSCLKCFG_OSCSRC_MOSC | SYSCTL_RSCLKCFG_PLLSRC_MOSC
                    | (PSYSDIV << SYSCTL_RSCLKCFG_PSYSDIV_Pos);
}

static void divsclk_init(void)
{
    /* PQ4 = DIVSCLK (PCTL 7) */
    SYSCTL_RCGCGPIO |= (1U << PORTQ);
    while (!(SYSCTL_PRGPIO & (1U << PORTQ))) { }
    GPIOQ->AFSEL |= (1U << 4);
    GPIOQ->PCTL   = (GPIOQ->PCTL & ~(0xFU << 16)) | (7U << 16);
    GPIOQ->DR8R  |= (1U << 4);
    GPIOQ->DEN   |= (1U << 4);
    /* EN [31], SRC [17:16] = 0 (SYSCLK), DIV [7:0] = divisor - 1 */
    SYSCTL_DIVSCLK = (1U << 31) | (0U << 16) | (12U - 1U);
}

int main(void)
{
    SYSCTL_RCGCGPIO |= (1U << PORTN);
    while (!(SYSCTL_PRGPIO & (1U << PORTN))) { }
    GPION->DIR |= (1U << 1);
    GPION->DEN |= (1U << 1);

    blink(5);                /* 16 MHz */
    delay(1000000);
    pll_init();
    divsclk_init();
    for (;;) blink(1);       /* SYSCLK_HZ */
}
