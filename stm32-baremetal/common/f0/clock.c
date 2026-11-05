/*
 * clock.c (F0) - clock de 48 MHz a partir do HSI. Explicado no módulo 04.
 */
#include "system.h"

/* Valores de reset: o F0 inicia com HSI = 8 MHz e prescalers = 1 */
uint32_t SystemCoreClock = 8000000UL;
uint32_t APB1Clock       = 8000000UL;
uint32_t APB2Clock       = 8000000UL;   /* F0 só tem um APB; mantido por simetria */

/*
 * SYSCLK = (HSI / 2) * 12 = 4 MHz * 12 = 48 MHz
 * HCLK   = SYSCLK / 1  = 48 MHz
 * PCLK   = HCLK   / 1  = 48 MHz
 */
void clock_init(void)
{
    /* 1) Flash: 1 wait state (24 < SYSCLK <= 48 MHz) + prefetch.
     *    Precisa ser feito ANTES de subir o clock. */
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;

    /* 2) Garante HSI ligado e estável */
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY)) { }

    /* 3) Se já estiver rodando do PLL (ex.: chamada repetida), volta p/ HSI,
     *    porque não se pode reconfigurar o PLL com ele ligado. */
    RCC->CFGR &= ~RCC_CFGR_SW_Msk;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != 0) { }
    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY) { }

    /* 4) Configura PLL: fonte HSI/2, multiplicador x12 (PLLMUL = 1010b),
     *    AHB /1 e APB /1 */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_PLLSRC_Msk | RCC_CFGR_PLLMUL_Msk |
                               RCC_CFGR_HPRE_Msk   | RCC_CFGR_PPRE_Msk))
              | RCC_CFGR_PLLSRC_HSI_DIV2
              | ((12U - 2U) << RCC_CFGR_PLLMUL_Pos);

    /* 5) Liga o PLL e espera travar */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) { }

    /* 6) Troca o SYSCLK para o PLL e confirma pelo SWS */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL) { }

    SystemCoreClock = 48000000UL;
    APB1Clock       = 48000000UL;
    APB2Clock       = 48000000UL;
}
