/*
 * Módulo 14 - ADC (ATmega328P): 10 bits, 6 entradas (A0..A5) + canais internos
 *   A0 (PC0): potenciômetro entre 5 V e GND
 *   canal 14: referência interna de 1,1 V (bandgap) -> calcula o Vcc real
 *   canal 8 : sensor de temperatura interno (com a referência de 1,1 V)
 */
#include "system.h"

static uint16_t adc_read(uint8_t admux)
{
    ADMUX = admux;
    ADCSRA |= (1U << ADSC);                       /* inicia a conversão */
    while (ADCSRA & (1U << ADSC)) { }             /* ADSC volta a 0 no fim */
    return ADC;                                   /* ADCL e depois ADCH */
}

static uint16_t adc_read_settled(uint8_t admux)  /* ao trocar a referência, descarta a 1ª */
{
    adc_read(admux);
    delay_ms(2);
    return adc_read(admux);
}

int main(void)
{
    system_init();
    uart_init(115200);

    DIDR0  = (1U << 0);                           /* desliga a entrada digital do A0 (menos ruído/consumo) */
    ADCSRA = (1U << ADEN) | (7U << ADPS0);        /* liga, ÷128 -> 125 kHz (deve ficar entre 50 e 200 kHz) */

    kprintf("\n=== Modulo 14: ADC ===\n");
    for (;;) {
        /* Vcc: mede 1,1 V usando o próprio Vcc como referência -> Vcc = 1,1 * 1024 / leitura */
        uint16_t bg   = adc_read_settled((1U << REFS0) | 14U);
        uint32_t vcc  = 1100UL * 1024UL / bg;     /* em mV */
        uint16_t a0   = adc_read_settled((1U << REFS0) | 0U);
        uint32_t a0mv = (uint32_t)a0 * vcc / 1024UL;
        /* temperatura: ~314 mV a 25 C, 1 mV/C (datasheet; precisa calibrar) */
        uint16_t t    = adc_read_settled((1U << REFS1) | (1U << REFS0) | 8U);
        int32_t  tmv  = (int32_t)t * 1100L / 1024L;
        int32_t  temp = tmv - 314L + 25L;
        kprintf("Vcc = %lu mV | A0 = %u (%lu mV) | temp ~ %ld C\n",
                (unsigned long)vcc, a0, (unsigned long)a0mv, (long)temp);
        delay_ms(500);
    }
}
