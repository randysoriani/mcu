/*
 * adc.c (F0) - ADC de 12 bits do STM32F072 por polling.
 *
 * Particularidades do ADC do F0:
 *   - seleção de canais por MÁSCARA (CHSELR), não por sequência
 *   - um único tempo de amostragem para todos os canais (SMPR)
 *   - calibração obrigatória com ADEN = 0 antes de habilitar
 */
#include "system.h"
#include "gpio.h"
#include "adc.h"

void adc_init(void)
{
    RCC->AHBENR  |= RCC_AHBENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADCEN;
    gpio_mode(GPIOA, 0, GPIO_MODE_ANALOG);          /* PA0 analógico (A0) */

    /* Clock do ADC: CKMODE = 10 -> PCLK/4 = 12 MHz (máx. 14 MHz).
     * Só pode ser alterado com ADEN = 0. */
    ADC1->CFGR2 = (2U << ADC_CFGR2_CKMODE_Pos);

    /* Garante ADC desligado */
    if (ADC1->CR & ADC_CR_ADEN) {
        ADC1->CR |= ADC_CR_ADDIS;
        while (ADC1->CR & ADC_CR_ADEN) { }
    }

    /* Calibração: ADCAL = 1 e espera o hardware zerar. O fator fica em DR. */
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) { }
    for (volatile int i = 0; i < 20; i++) { }       /* >= 4 ciclos de ADC antes do ADEN */

    ADC1->CFGR1 = 0;                                /* 12 bits, alinhado à direita, single */
    ADC1->SMPR  = 7U;                               /* 239,5 ciclos (~20 us): bom p/ VREF/TS */

    /* Liga VREFINT e sensor de temperatura */
    ADC_COMMON->CCR |= ADC_CCR_VREFEN | ADC_CCR_TSEN;

    /* Habilita: limpa ADRDY (escrevendo 1), seta ADEN, espera ADRDY */
    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { }
}

uint16_t adc_read(uint32_t ch)
{
    ADC1->CHSELR = 1U << ch;                        /* um canal só */
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC)) { }
    return (uint16_t)ADC1->DR;                      /* ler DR limpa EOC */
}
