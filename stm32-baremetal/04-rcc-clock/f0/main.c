/*
 * Módulo 04 - RCC: árvore de clock do STM32F072
 *
 * 1. Pisca o LED 5x com o clock de reset (HSI 8 MHz)
 * 2. Configura o SYSCLK = 48 MHz usando a fonte escolhida em CLOCK_SOURCE
 * 3. Pisca com o MESMO laço de atraso -> agora 6x mais rápido
 * 4. Coloca SYSCLK/16 no pino PA8 (MCO) para medir com osciloscópio
 *    (48 MHz / 16 = 3 MHz)
 *
 * Programa autocontido (não usa common/f0/clock.c).
 */
#include "device.h"

#define SRC_HSI_PLL    1   /* HSI 8 MHz /2 x12 -> 48 MHz (padrão, sempre funciona)   */
#define SRC_HSE_PLL    2   /* HSE 8 MHz (bypass: MCO do ST-LINK) x6 -> 48 MHz         */
#define SRC_HSI48      3   /* oscilador interno de 48 MHz do F072, sem PLL            */

#ifndef CLOCK_SOURCE
#define CLOCK_SOURCE SRC_HSI_PLL
#endif

static void delay(volatile uint32_t n) { while (n--) { __asm volatile ("nop"); } }

static void blink(int vezes)
{
    for (int i = 0; i < vezes * 2; i++) {
        GPIOA->ODR ^= (1U << 5);
        delay(150000);
    }
}

static void switch_sysclk(uint32_t sw)
{
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | sw;
    while (((RCC->CFGR & RCC_CFGR_SWS_Msk) >> 2) != sw) { }   /* SWS espelha SW */
}

static void clock_48mhz(void)
{
    /* Flash precisa de 1 wait state acima de 24 MHz: configurar ANTES */
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;

#if CLOCK_SOURCE == SRC_HSI48
    RCC->CR2 |= RCC_CR2_HSI48ON;
    while (!(RCC->CR2 & RCC_CR2_HSI48RDY)) { }
    switch_sysclk(RCC_CFGR_SW_HSI48);

#else
    /* PLL deve estar desligado para ser configurado */
    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY) { }

#  if CLOCK_SOURCE == SRC_HSE_PLL
    /* Nucleo: o ST-LINK fornece 8 MHz em OSC_IN -> modo bypass (sem cristal).
     * Se o seu board tiver cristal, remova o HSEBYP. */
    RCC->CR |= RCC_CR_HSEBYP | RCC_CR_HSEON;
    for (uint32_t t = 0; !(RCC->CR & RCC_CR_HSERDY); t++) {
        if (t > 500000) {                /* HSE não subiu: fica no HSI e avisa */
            for (;;) { blink(1); delay(2000000); }
        }
    }
    RCC->CFGR2 = 0;                      /* PREDIV = /1 */
    RCC->CFGR  = (RCC->CFGR & ~(RCC_CFGR_PLLSRC_Msk | RCC_CFGR_PLLMUL_Msk))
               | RCC_CFGR_PLLSRC_HSE_PREDIV
               | ((6U - 2U) << RCC_CFGR_PLLMUL_Pos);    /* x6 */
#  else
    RCC->CFGR  = (RCC->CFGR & ~(RCC_CFGR_PLLSRC_Msk | RCC_CFGR_PLLMUL_Msk))
               | RCC_CFGR_PLLSRC_HSI_DIV2
               | ((12U - 2U) << RCC_CFGR_PLLMUL_Pos);   /* x12 */
#  endif

    /* AHB e APB sem divisão (HPRE = 0xxx, PPRE = 0xx) */
    RCC->CFGR &= ~(RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE_Msk);

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) { }
    switch_sysclk(RCC_CFGR_SW_PLL);
#endif
}

static void mco_init(void)
{
    /* PA8 em função alternativa AF0 = MCO, velocidade máxima */
    GPIOA->MODER   = (GPIOA->MODER & ~(3U << 16)) | (2U << 16);
    GPIOA->OSPEEDR |= (3U << 16);
    GPIOA->AFR[1] &= ~(0xFU << 0);

    /* MCO[27:24] = 0100 (SYSCLK), MCOPRE[30:28] = 100 (/16) */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_MCO_Msk | (7U << 28)))
              | (4U << RCC_CFGR_MCO_Pos) | (4U << 28);
}

int main(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER = (GPIOA->MODER & ~(3U << 10)) | (1U << 10);

    blink(5);           /* 8 MHz */
    delay(1000000);

    clock_48mhz();
    mco_init();

    for (;;) {
        blink(1);       /* 48 MHz: mesmo laço, 6x mais rápido */
    }
}
