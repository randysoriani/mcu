/*
 * Módulo 15 - DMA
 *
 *   1. dma_memcpy(): copia 1 KB por DMA e compara com a CPU
 *   2. ADC contínuo (A0 + VREFINT) -> DMA circular -> buffer na RAM.
 *      A CPU não faz nada para as amostras chegarem: só lê o buffer.
 *   3. Texto longo enviado pela USART2 via DMA enquanto a CPU conta
 *      quantas iterações de laço consegue fazer "de graça".
 */
#include <string.h>
#include "system.h"
#include "dma_demo.h"

#define NSAMPLES 32                      /* par: [A0, VREF, A0, VREF, ...] */
static volatile uint16_t adc_buf[NSAMPLES];

static uint32_t src_buf[256], dst_buf[256];

static const char texto[] =
    "Este texto esta sendo enviado pela USART2 via DMA. Enquanto isso a CPU\r\n"
    "fica livre: ela apenas conta iteracoes de um laco. Sem DMA, a CPU teria\r\n"
    "que esperar cada byte (~87 us a 115200 bps) antes de escrever o proximo.\r\n";

int main(void)
{
    system_init();
    uart_init(115200);
    kprintf("\n=== Modulo 15: DMA ===\n");

    /* ---- 1. memória -> memória ---- */
    for (int i = 0; i < 256; i++) src_buf[i] = 0xA5000000U | (uint32_t)i;
    uint32_t t0 = SysTick->VAL;
    dma_memcpy(dst_buf, src_buf, 256);
    uint32_t t1 = SysTick->VAL;
    kprintf("dma_memcpy 1 KB: %s, ~%u ciclos\n",
            memcmp(src_buf, dst_buf, sizeof src_buf) == 0 ? "OK" : "ERRO",
            (t0 >= t1) ? t0 - t1 : t0 + SysTick->LOAD + 1U - t1);

    /* ---- 2. ADC + DMA circular ---- */
    adc_dma_start(adc_buf, NSAMPLES);

    for (;;) {
        /* ---- 3. UART por DMA ---- */
        uint32_t livres = 0;
        uart_dma_send(texto, sizeof texto - 1U);
        while (uart_dma_busy()) livres++;          /* CPU livre durante o envio */
        kprintf("CPU executou %u iteracoes durante o envio por DMA\n", livres);

        /* médias do buffer que o DMA mantém atualizado */
        uint32_t a0 = 0, vref = 0;
        for (int i = 0; i < NSAMPLES; i += 2) { a0 += adc_buf[i]; vref += adc_buf[i + 1]; }
        a0 /= NSAMPLES / 2; vref /= NSAMPLES / 2;
        uint32_t vdda = 3300U * (*VREFINT_CAL_ADDR) / (vref ? vref : 1U);
        kprintf("ADC via DMA: A0 = %u (%u mV), VREFINT = %u -> VDDA = %u mV\n\n",
                a0, a0 * vdda / 4095U, vref, vdda);
        delay_ms(2000);
    }
}
