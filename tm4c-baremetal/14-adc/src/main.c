/*
 * Módulo 14 - ADC
 *   AIN0 = PE3 nas duas placas (potenciômetro entre 3V3 e GND)
 *   sensor de temperatura interno (bit TS do sequenciador)
 * Sequenciador 3 (1 amostra), disparo por software, média de 16 por hardware.
 * O TM4C não tem VREFINT: a referência do ADC é VDDA (3,3 V nominais).
 */
#include "system.h"
#include "gpio.h"

#define ADC_CH_AIN0 0U
#define ADC_TEMP    0x100U    /* marcador para "sensor de temperatura" */

static void adc_init(void)
{
    periph_enable(&SYSCTL_RCGCADC, &SYSCTL_PRADC, 0);
    gpio_port_enable(PORTE);
    gpio_analog(GPIOE, 3);                     /* AFSEL=1, DEN=0, AMSEL=1 */
#if defined(TM4C129)
    ADC0->CC = 0x1U;                           /* CS = ALTCLK (PIOSC 16 MHz) */
#endif
    ADC0->ACTSS &= ~(1U << 3);                 /* desliga o SS3 para configurar  */
    ADC0->EMUX  &= ~(0xFU << 12);              /* SS3: trigger = processador (PSSI) */
    ADC0->SAC    = 4U;                         /* média de 2^4 = 16 amostras     */
    ADC0->IM    &= ~(1U << 3);                 /* sem interrupção no NVIC (polling) */
    ADC0->ACTSS |=  (1U << 3);
}

static uint32_t adc_read(uint32_t ch)
{
    ADC0->ACTSS &= ~(1U << 3);
    if (ch == ADC_TEMP) {
        ADC0->SS[3].CTL = ADC_SSCTL_TS0 | ADC_SSCTL_IE0 | ADC_SSCTL_END0;
    } else {
        ADC0->SS[3].MUX = ch;
        ADC0->SS[3].CTL = ADC_SSCTL_IE0 | ADC_SSCTL_END0;   /* passo 0 é o último */
    }
    ADC0->ACTSS |= (1U << 3);

    ADC0->PSSI = (1U << 3);                    /* dispara o SS3 */
    while (!(ADC0->RIS & (1U << 3))) { }       /* IE0 seta o RIS mesmo sem IM */
    uint32_t v = ADC0->SS[3].FIFO & 0xFFFU;
    ADC0->ISC = (1U << 3);                     /* limpa (escreve 1) */
    return v;
}

int main(void)
{
    system_init();
    uart_init(115200);
    adc_init();
    kprintf("\n=== Modulo 14: ADC (AIN0 = PE3) ===\n");

    for (;;) {
        uint32_t raw  = adc_read(ADC_CH_AIN0);
        uint32_t mv   = raw * 3300U / 4095U;
        uint32_t rawt = adc_read(ADC_TEMP);
        /* datasheet: TEMP = 147,5 - (75 * 3,3 * ADC / 4096)  -> em décimos de grau */
        int32_t  t_dc = 1475 - (int32_t)(2475U * rawt / 4096U);
        kprintf("AIN0 = %4u (%4u mV) | sensor = %4u -> %d.%d C\n",
                raw, mv, rawt, t_dc / 10, (t_dc < 0 ? -t_dc : t_dc) % 10);
        delay_ms(500);
    }
}
