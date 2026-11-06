/*
 * Módulo 14 - ADC (AT32UC3A): 10 bits, 8 canais
 *   EVK1100: AD0 = PA21 sensor de temperatura (NTC), AD1 = PA22
 *   potenciômetro, AD2 = PA23 sensor de luz (função A). Referência 3,3 V.
 */
#include "system.h"

#define ADC_SR_EOC(n) (1UL << (n))

static uint32_t adc_read(uint32_t ch)
{
    ADC->CHER = 1UL << ch;
    ADC->CR   = 1UL << 1;                         /* START */
    while (!(ADC->SR & ADC_SR_EOC(ch))) { }
    uint32_t v = ADC->CDR[ch];                    /* ler CDR limpa o EOC */
    ADC->CHDR = 1UL << ch;
    return v;
}

int main(void)
{
    system_init();
    uart_init(115200);

    gpio_periph(PA(21), 0);
    gpio_periph(PA(22), 0);
    gpio_periph(PA(23), 0);
    /* ADCclk = PBA / ((PRESCAL+1) * 2) = 48 MHz / 10 = 4,8 MHz (máx. 5 MHz em 10 bits) */
    ADC->MR = (4UL << 8)                          /* PRESCAL */
            | (10UL << 16)                        /* STARTUP */
            | (3UL << 24);                        /* SHTIM: tempo de amostragem */

    kprintf("\n=== Modulo 14: ADC ===\n");
    for (;;) {
        uint32_t t = adc_read(0), p = adc_read(1), l = adc_read(2);
        kprintf("temperatura (bruto) = %4lu | pot = %4lu (%4lu mV) | luz = %4lu\n",
                (unsigned long)t, (unsigned long)p, (unsigned long)(p * 3300UL / 1023UL), (unsigned long)l);
        delay_ms(500);
    }
}
