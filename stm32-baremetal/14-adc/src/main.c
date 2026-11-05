/*
 * Módulo 14 - ADC
 *
 * Lê a cada 500 ms:
 *   - A0 (PA0): ligue um potenciômetro (pontas em 3V3/GND, cursor em A0)
 *   - VREFINT: referência interna -> calcula a tensão REAL de VDDA
 *   - sensor de temperatura interno (com a calibração de fábrica)
 *
 * Tudo em aritmética inteira (mV e décimos de grau).
 */
#include "system.h"
#include "adc.h"

/* média de N leituras para reduzir ruído */
static uint32_t adc_avg(uint32_t ch, uint32_t n)
{
    uint32_t acc = 0;
    for (uint32_t i = 0; i < n; i++) acc += adc_read(ch);
    return acc / n;
}

int main(void)
{
    system_init();
    uart_init(115200);
    adc_init();

    const uint32_t vref_cal = *VREFINT_CAL_ADDR;   /* leitura de VREFINT com VDDA = 3,3 V */
    const int32_t  ts_cal1  = *TS_CAL1_ADDR;       /* leitura do sensor a 30 C  */
    const int32_t  ts_cal2  = *TS_CAL2_ADDR;       /* leitura do sensor a 110 C */

    kprintf("\n=== Modulo 14: ADC ===\n");
    kprintf("Calibracao de fabrica: VREFINT_CAL=%u TS_CAL1=%d TS_CAL2=%d\n",
            vref_cal, ts_cal1, ts_cal2);

    for (;;) {
        uint32_t raw_vref = adc_avg(ADC_CH_VREF, 16);
        uint32_t raw_a0   = adc_avg(ADC_CH_A0, 16);
        uint32_t raw_ts   = adc_avg(ADC_CH_TEMP, 16);

        /* VDDA = 3,3 V * VREFINT_CAL / VREFINT_lido */
        uint32_t vdda_mv = 3300U * vref_cal / raw_vref;

        /* Tensão em A0 em relação ao VDDA real */
        uint32_t a0_mv = raw_a0 * vdda_mv / 4095U;

        /* Temperatura: interpolação linear entre os dois pontos de fábrica.
         * A leitura é normalizada para 3,3 V, como na calibração. */
        int32_t ts_norm = (int32_t)(raw_ts * vdda_mv / 3300U);
        int32_t t_dc    = 300 + (ts_norm - ts_cal1) * (1100 - 300) / (ts_cal2 - ts_cal1);

        kprintf("VDDA = %4u mV | A0 = %4u (%4u mV) | Temp = %d.%d C\n",
                vdda_mv, raw_a0, a0_mv, t_dc / 10, (t_dc < 0 ? -t_dc : t_dc) % 10);
        delay_ms(500);
    }
}
