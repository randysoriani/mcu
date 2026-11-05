/*
 * Módulo 15 - uDMA (micro DMA, um controlador ARM PL230 em todos os TM4C)
 *
 *   1. memória -> memória pelo canal de SOFTWARE (30), modo AUTO
 *   2. UART0 TX por uDMA (canal 9, encoding 0) enquanto a CPU fica livre
 *   3. ADC0 SS3 em modo contínuo -> uDMA (canal 17, encoding 0) -> buffer
 *
 * A configuração de cada canal NÃO fica em registradores: fica numa TABELA
 * DE CONTROLE na SRAM, alinhada a 1024 bytes, cujo endereço vai em CTLBASE.
 */
#include <string.h>
#include "system.h"
#include "gpio.h"

#define CH_UART0_TX 9U
#define CH_ADC0_SS3 17U
#define CH_SW       30U

/* 32 estruturas primárias + 32 alternativas, 16 bytes cada */
static UDMA_Entry ctl_table[64] __attribute__((aligned(1024)));

static uint32_t src_buf[256], dst_buf[256];
#define NSAMPLES 32
static volatile uint32_t adc_buf[NSAMPLES];   /* FIFO do ADC = 32 bits; origem e destino do mesmo tamanho */

static const char texto[] =
    "Este texto esta sendo enviado pela UART0 via uDMA. Enquanto isso a CPU\r\n"
    "fica livre: ela apenas conta iteracoes de um laco. Sem DMA, a CPU teria\r\n"
    "que esperar cada byte (~87 us a 115200 bps) antes de escrever o proximo.\r\n";

static void udma_init(void)
{
    periph_enable(&SYSCTL_RCGCDMA, &SYSCTL_PRDMA, 0);
    UDMA->CFG     = 1U;                          /* MASTEN */
    UDMA->CTLBASE = (uint32_t)ctl_table;
}

static inline int udma_busy(uint32_t ch) { return (UDMA->ENASET >> ch) & 1U; }  /* EN volta a 0 no fim */

static void dma_memcpy(void *dst, const void *src, uint32_t words)
{
    UDMA_Entry *e = &ctl_table[CH_SW];
    e->SRCENDP = (uint32_t *)src + words - 1;    /* endereço do ÚLTIMO item */
    e->DSTENDP = (uint32_t *)dst + words - 1;
    e->CHCTL   = UDMA_CHCTL(UDMA_INC_32, UDMA_SIZE_32, UDMA_INC_32, UDMA_SIZE_32,
                            3 /* arb 8 */, words, UDMA_MODE_AUTO);
    UDMA->ENASET = 1U << CH_SW;
    UDMA->SWREQ  = 1U << CH_SW;                  /* AUTO: um pedido transfere tudo */
    while (udma_busy(CH_SW)) { }
}

static void uart_dma_send(const void *data, uint32_t len)
{
    UDMA_Entry *e = &ctl_table[CH_UART0_TX];
    e->SRCENDP = (const uint8_t *)data + len - 1;
    e->DSTENDP = &UART0->DR;
    e->CHCTL   = UDMA_CHCTL(UDMA_INC_NONE, UDMA_SIZE_8, UDMA_INC_8, UDMA_SIZE_8,
                            2 /* arb 4: cabe na FIFO */, len, UDMA_MODE_BASIC);
    UDMA->CHMAP[1] &= ~(0xFU << 4);              /* canal 9 -> encoding 0 (UART0 TX) */
    UART0->DMACTL  |= UART_DMACTL_TXDMAE;
    UDMA->ENASET    = 1U << CH_UART0_TX;
}

static void adc_dma_arm(void)
{
    UDMA_Entry *e = &ctl_table[CH_ADC0_SS3];
    e->SRCENDP = &ADC0->SS[3].FIFO;
    e->DSTENDP = &adc_buf[NSAMPLES - 1];
    e->CHCTL   = UDMA_CHCTL(UDMA_INC_32, UDMA_SIZE_32, UDMA_INC_NONE, UDMA_SIZE_32,
                            0, NSAMPLES, UDMA_MODE_BASIC);
    UDMA->ENASET = 1U << CH_ADC0_SS3;
}

static void adc_continuous_init(void)
{
    periph_enable(&SYSCTL_RCGCADC, &SYSCTL_PRADC, 0);
    gpio_port_enable(PORTE);
    gpio_analog(GPIOE, 3);
#if defined(TM4C129)
    ADC0->CC = 0x1U;
#endif
    ADC0->ACTSS &= ~(1U << 3);
    ADC0->EMUX   = (ADC0->EMUX & ~(0xFU << 12)) | (0xFU << 12);   /* SS3: "always" */
    ADC0->SS[3].MUX = 0;                                            /* AIN0 */
    ADC0->SS[3].CTL = ADC_SSCTL_IE0 | ADC_SSCTL_END0;
    ADC0->SAC = 6U;                                                 /* média de 64 (desacelera) */
    UDMA->CHMAP[2] &= ~(0xFU << 4);                                 /* canal 17 -> encoding 0 */
    adc_dma_arm();
#if defined(TM4C129)
    ADC0->ACTSS |= (1U << 3) | (1U << 11);   /* ASEN3 + ADEN3 (DMA do SS3) */
#else
    ADC0->ACTSS |= (1U << 3);
#endif
}

int main(void)
{
    system_init();
    uart_init(115200);
    udma_init();
    kprintf("\n=== Modulo 15: uDMA ===\n");

    for (int i = 0; i < 256; i++) src_buf[i] = 0xA5000000U | (uint32_t)i;
    uint32_t t0 = SysTick->VAL;
    dma_memcpy(dst_buf, src_buf, 256);
    uint32_t t1 = SysTick->VAL;
    kprintf("dma_memcpy 1 KB: %s, ~%u ciclos\n",
            memcmp(src_buf, dst_buf, sizeof src_buf) == 0 ? "OK" : "ERRO",
            (t0 >= t1) ? t0 - t1 : t0 + SysTick->LOAD + 1U - t1);

    adc_continuous_init();

    for (;;) {
        uint32_t livres = 0;
        uart_dma_send(texto, sizeof texto - 1U);
        while (udma_busy(CH_UART0_TX)) livres++;
        while (UART0->FR & UART_FR_BUSY) { }
        UART0->DMACTL &= ~UART_DMACTL_TXDMAE;    /* devolve a UART ao kprintf */
        kprintf("CPU executou %u iteracoes durante o envio por uDMA\n", livres);

        if (!udma_busy(CH_ADC0_SS3)) {           /* buffer cheio */
            uint32_t acc = 0;
            for (int i = 0; i < NSAMPLES; i++) acc += adc_buf[i] & 0xFFFU;
            acc /= NSAMPLES;
            kprintf("ADC via uDMA: AIN0 = %u (%u mV), overflow SS3 = %u\n\n",
                    acc, acc * 3300U / 4095U, (ADC0->OSTAT >> 3) & 1U);
            ADC0->OSTAT = (1U << 3);
            adc_dma_arm();                       /* re-arma: BASIC para no fim */
        }
        delay_ms(2000);
    }
}
