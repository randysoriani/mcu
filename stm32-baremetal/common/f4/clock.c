/*
 * clock.c (F4) - clock de 180 MHz (HSI + PLL + over-drive). Explicado no módulo 04.
 */
#include "system.h"

/* Valores de reset: HSI = 16 MHz, prescalers = 1 */
uint32_t SystemCoreClock = 16000000UL;
uint32_t APB1Clock       = 16000000UL;
uint32_t APB2Clock       = 16000000UL;

/*
 *  HSI (16 MHz) -> /M=8 -> 2 MHz -> xN=180 -> VCO 360 MHz -> /P=2 -> 180 MHz
 *  HCLK  = 180 MHz (AHB /1)
 *  PCLK1 =  45 MHz (APB1 /4, máx 45)   -> timers APB1 = 90 MHz
 *  PCLK2 =  90 MHz (APB2 /2, máx 90)   -> timers APB2 = 180 MHz
 */
#define PLL_M 8U
#define PLL_N 180U
#define PLL_P 2U
#define PLL_Q 8U
#define PLL_R 2U

void clock_init(void)
{
#if defined(QEMU)
    /* Emulação (qemu-system-arm -M netduinoplus2, um STM32F405): o QEMU não
     * modela o RCC/PWR, então esperar HSIRDY/PLLRDY travaria. O SysTick do
     * QEMU roda a 168 MHz. Use: make EXTRA=-DQEMU (ver módulo 00). */
    SystemCoreClock = 168000000UL;
    APB1Clock       =  42000000UL;
    APB2Clock       =  84000000UL;
    return;
#endif
    /* 1) Liga o clock do PWR e seleciona Voltage Scale 1 (necessário > 168 MHz) */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR;                     /* pequena espera após habilitar clock */

    /* 2) HSI ligado e estável */
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY)) { }

    /* 3) Volta para HSI e desliga o PLL (caso esta função seja chamada de novo) */
    RCC->CFGR &= ~RCC_CFGR_SW_Msk;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != 0) { }
    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY) { }

    /* VOS só pode ser alterado com o PLL desligado */
    PWR->CR = (PWR->CR & ~PWR_CR_VOS_Msk) | (3U << PWR_CR_VOS_Pos);

    /* 4) Configura o PLL principal (fonte = HSI, bit PLLSRC = 0) */
    RCC->PLLCFGR = (PLL_M << RCC_PLLCFGR_PLLM_Pos)
                 | (PLL_N << RCC_PLLCFGR_PLLN_Pos)
                 | (((PLL_P / 2U) - 1U) << RCC_PLLCFGR_PLLP_Pos)  /* 00 = /2 */
                 | (PLL_Q << RCC_PLLCFGR_PLLQ_Pos)
                 | (PLL_R << RCC_PLLCFGR_PLLR_Pos);

    /* 5) Liga o PLL */
    RCC->CR |= RCC_CR_PLLON;

    /* 6) Over-drive: necessário para 168 < HCLK <= 180 MHz (só F42x/F43x/F446) */
    PWR->CR |= PWR_CR_ODEN;
    while (!(PWR->CSR & PWR_CSR_ODRDY)) { }
    PWR->CR |= PWR_CR_ODSWEN;
    while (!(PWR->CSR & PWR_CSR_ODSWRDY)) { }

    while (!(RCC->CR & RCC_CR_PLLRDY)) { }

    /* 7) Flash: 5 wait states @180 MHz/3.3 V + prefetch + caches */
    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | 5U;
    while ((FLASH->ACR & FLASH_ACR_LATENCY_Msk) != 5U) { }

    /* 8) Prescalers ANTES de trocar o clock: AHB /1, APB1 /4 (101b), APB2 /2 (100b) */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk | RCC_CFGR_PPRE2_Msk))
              | (0U << RCC_CFGR_HPRE_Pos)
              | (5U << RCC_CFGR_PPRE1_Pos)
              | (4U << RCC_CFGR_PPRE2_Pos);

    /* 9) SYSCLK = PLL */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL) { }

    SystemCoreClock = 180000000UL;
    APB1Clock       =  45000000UL;
    APB2Clock       =  90000000UL;
}
