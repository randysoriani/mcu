/*
 * adc.c (F4) - ADC1 de 12 bits do STM32F446 por polling.
 *
 * Particularidades do ADC do F4:
 *   - canais organizados em SEQUÊNCIA (SQR1..3), até 16 conversões
 *   - tempo de amostragem por canal (SMPR1/SMPR2)
 *   - grupo "injetado" separado (JSQR/JDRx) com prioridade
 *   - sem calibração por software
 */
#include "system.h"
#include "gpio.h"
#include "adc.h"

static void set_sample_time(uint32_t ch, uint32_t smp)
{
    if (ch <= 9) {
        ADC1->SMPR2 = (ADC1->SMPR2 & ~(7U << (3U * ch))) | (smp << (3U * ch));
    } else {
        uint32_t s = 3U * (ch - 10U);
        ADC1->SMPR1 = (ADC1->SMPR1 & ~(7U << s)) | (smp << s);
    }
}

void adc_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    (void)RCC->APB2ENR;
    gpio_mode(GPIOA, 0, GPIO_MODE_ANALOG);

    /* ADCCLK = PCLK2 / 4 = 22,5 MHz (máx. 36 MHz). TSVREFE liga VREFINT e TS. */
    ADC_COMMON->CCR = (1U << ADC_CCR_ADCPRE_Pos) | ADC_CCR_TSVREFE;

    ADC1->CR1 = 0;                      /* RES = 00: 12 bits, sem scan */
    ADC1->CR2 = 0;                      /* single, alinhado à direita */
    ADC1->SQR1 = 0;                     /* L = 0: sequência de 1 conversão */

    set_sample_time(ADC_CH_A0,   7U);   /* 480 ciclos (~21 us) */
    set_sample_time(ADC_CH_VREF, 7U);
    set_sample_time(ADC_CH_TEMP, 7U);   /* sensor exige >= 10 us */

    ADC1->CR2 |= ADC_CR2_ADON;
    delay_us(20);                        /* tSTAB do ADC + start-up do sensor */
}

uint16_t adc_read(uint32_t ch)
{
    ADC1->SQR3 = ch;                     /* SQ1 = canal desejado */
    ADC1->CR2 |= ADC_CR2_SWSTART;
    while (!(ADC1->SR & ADC_SR_EOC)) { }
    return (uint16_t)ADC1->DR;           /* ler DR limpa EOC */
}
