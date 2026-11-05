/*
 * Módulo 04 - RCC: árvore de clock do STM32F446
 *
 * 1. Pisca o LED 5x com o clock de reset (HSI 16 MHz)
 * 2. Configura SYSCLK = 180 MHz (PLL + over-drive) a partir de CLOCK_SOURCE
 * 3. Pisca com o MESMO laço -> ~11x mais rápido (menos, por causa dos wait
 *    states da Flash; o ART accelerator compensa boa parte)
 * 4. MCO1 (PA8)  = PLL / 5     = 36 MHz
 *    MCO2 (PC9)  = SYSCLK / 5  = 36 MHz
 *
 * Programa autocontido (não usa common/f4/clock.c).
 */
#include "device.h"

#define SRC_HSI_PLL  1   /* HSI 16 MHz /8 = 2 MHz -> x180 = 360 -> /2 = 180 MHz         */
#define SRC_HSE_PLL  2   /* HSE 8 MHz (bypass, vem do ST-LINK) /4 = 2 MHz -> idem      */

#ifndef CLOCK_SOURCE
#define CLOCK_SOURCE SRC_HSI_PLL
#endif

static void delay(volatile uint32_t n) { while (n--) { __asm volatile ("nop"); } }

static void blink(int vezes)
{
    for (int i = 0; i < vezes * 2; i++) {
        GPIOA->ODR ^= (1U << 5);
        delay(300000);
    }
}

static void clock_180mhz(void)
{
    uint32_t pllm, pllsrc;

    /* 1) Regulador: clock do PWR + Voltage Scale 1 (VOS = 11) */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR;
    PWR->CR = (PWR->CR & ~PWR_CR_VOS_Msk) | (3U << PWR_CR_VOS_Pos);

    /* 2) Fonte do PLL */
#if CLOCK_SOURCE == SRC_HSE_PLL
    RCC->CR |= RCC_CR_HSEBYP | RCC_CR_HSEON;
    for (uint32_t t = 0; !(RCC->CR & RCC_CR_HSERDY); t++) {
        if (t > 1000000) { for (;;) { blink(1); delay(4000000); } }
    }
    pllm   = 4;                          /* 8 MHz / 4 = 2 MHz */
    pllsrc = RCC_PLLCFGR_PLLSRC_HSE;
#else
    pllm   = 8;                          /* 16 MHz / 8 = 2 MHz */
    pllsrc = 0;
#endif

    /* 3) PLL: VCO_in = 2 MHz (faixa 1..2 MHz), VCO = 2*180 = 360 MHz
     *    (faixa 100..432), P = /2 -> 180 MHz, Q = /8 = 45 MHz, R = /2 */
    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY) { }
    RCC->PLLCFGR = pllsrc
                 | (pllm  << RCC_PLLCFGR_PLLM_Pos)
                 | (180U  << RCC_PLLCFGR_PLLN_Pos)
                 | (0U    << RCC_PLLCFGR_PLLP_Pos)   /* 00 -> P = 2 */
                 | (8U    << RCC_PLLCFGR_PLLQ_Pos)
                 | (2U    << RCC_PLLCFGR_PLLR_Pos);
    RCC->CR |= RCC_CR_PLLON;

    /* 4) Over-drive (obrigatório acima de 168 MHz) */
    PWR->CR |= PWR_CR_ODEN;
    while (!(PWR->CSR & PWR_CSR_ODRDY)) { }
    PWR->CR |= PWR_CR_ODSWEN;
    while (!(PWR->CSR & PWR_CSR_ODSWRDY)) { }

    /* 5) Flash: 5 WS para 150 < HCLK <= 180 MHz @ 2.7-3.6 V, + ART (caches) */
    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | 5U;

    /* 6) Prescalers: AHB /1, APB1 /4 (45 MHz), APB2 /2 (90 MHz) */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk | RCC_CFGR_PPRE2_Msk))
              | (5U << RCC_CFGR_PPRE1_Pos)
              | (4U << RCC_CFGR_PPRE2_Pos);

    /* 7) Espera o PLL travar e troca o SYSCLK */
    while (!(RCC->CR & RCC_CR_PLLRDY)) { }
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL) { }
}

static void mco_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;

    /* PA8 -> MCO1 (AF0), PC9 -> MCO2 (AF0), very high speed */
    GPIOA->MODER   = (GPIOA->MODER & ~(3U << 16)) | (2U << 16);
    GPIOA->OSPEEDR |= (3U << 16);
    GPIOA->AFR[1] &= ~(0xFU << 0);
    GPIOC->MODER   = (GPIOC->MODER & ~(3U << 18)) | (2U << 18);
    GPIOC->OSPEEDR |= (3U << 18);
    GPIOC->AFR[1] &= ~(0xFU << 4);

    /* MCO1 = PLL (11), MCO1PRE = /5 (111); MCO2 = SYSCLK (00), MCO2PRE = /5 (111) */
    RCC->CFGR = (RCC->CFGR & ~((3U << RCC_CFGR_MCO1_Pos) | (7U << RCC_CFGR_MCO1PRE_Pos) |
                               (7U << RCC_CFGR_MCO2PRE_Pos) | (3U << RCC_CFGR_MCO2_Pos)))
              | (3U << RCC_CFGR_MCO1_Pos) | (7U << RCC_CFGR_MCO1PRE_Pos)
              | (0U << RCC_CFGR_MCO2_Pos) | (7U << RCC_CFGR_MCO2PRE_Pos);
}

int main(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;
    GPIOA->MODER = (GPIOA->MODER & ~(3U << 10)) | (1U << 10);

    blink(5);              /* 16 MHz */
    delay(2000000);

    clock_180mhz();
    mco_init();

    for (;;) {
        blink(1);          /* 180 MHz */
    }
}
