/*
 * adc.h - driver mínimo de ADC por polling (f0/adc.c, f4/adc.c)
 */
#ifndef ADC_H
#define ADC_H
#include <stdint.h>

#if defined(STM32F0)
#  define ADC_CH_A0    0U     /* PA0 = ADC_IN0          */
#  define ADC_CH_TEMP  16U    /* sensor de temperatura  */
#  define ADC_CH_VREF  17U    /* VREFINT (~1,23 V)      */
#else
#  define ADC_CH_A0    0U     /* PA0 = ADC1_IN0         */
#  define ADC_CH_TEMP  18U    /* F446: sensor em IN18   */
#  define ADC_CH_VREF  17U    /* VREFINT (~1,21 V)      */
#endif

void     adc_init(void);
uint16_t adc_read(uint32_t ch);   /* uma conversão de 12 bits (0..4095) */

#endif
