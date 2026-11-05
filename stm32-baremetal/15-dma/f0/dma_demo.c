/*
 * dma_demo.c (F0) - DMA1 do STM32F072 (controlador por CANAIS).
 *
 * No F0 cada periférico tem um canal FIXO (tabela "DMA requests for each
 * channel" do RM0091). Usados aqui:
 *   Canal 1 -> ADC
 *   Canal 4 -> USART2_TX
 *   Canal 2 -> memória->memória (qualquer canal serve com MEM2MEM)
 */
#include "system.h"
#include "gpio.h"
#include "dma_demo.h"

#define CH_ADC   1U
#define CH_UART  4U
#define CH_M2M   2U
#define DMA_CH(n) (&DMA1->CH[(n) - 1U])

static volatile int s_uart_busy;

void adc_dma_start(volatile uint16_t *buf, uint32_t n)
{
    RCC->AHBENR  |= RCC_AHBENR_DMAEN | RCC_AHBENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADCEN;
    gpio_mode(GPIOA, 0, GPIO_MODE_ANALOG);

    /* --- ADC: calibra com ADEN = 0 e DMAEN = 0 --- */
    ADC1->CFGR2 = (2U << ADC_CFGR2_CKMODE_Pos);         /* PCLK/4 */
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) { }
    for (volatile int i = 0; i < 20; i++) { }

    ADC1->SMPR   = 7U;                                  /* 239,5 ciclos */
    ADC1->CHSELR = (1U << 0) | (1U << 17);              /* A0 e VREFINT (ordem crescente) */
    ADC_COMMON->CCR |= ADC_CCR_VREFEN;
    ADC1->CFGR1  = ADC_CFGR1_DMAEN | ADC_CFGR1_DMACFG    /* DMA em modo circular */
                 | ADC_CFGR1_CONT;                      /* conversão contínua   */

    /* --- DMA canal 1: periférico (ADC1->DR, 16 bits) -> memória (16 bits) --- */
    DMA_Channel_TypeDef *ch = DMA_CH(CH_ADC);
    ch->CCR   = 0;
    ch->CPAR  = (uint32_t)&ADC1->DR;
    ch->CMAR  = (uint32_t)buf;
    ch->CNDTR = n;
    ch->CCR   = (1U << DMA_CCR_MSIZE_Pos) | (1U << DMA_CCR_PSIZE_Pos)  /* 16/16 bits */
              | DMA_CCR_MINC                   /* incrementa só a memória */
              | DMA_CCR_CIRC                   /* volta ao início sozinho  */
              | (1U << DMA_CCR_PL_Pos)         /* prioridade média         */
              | DMA_CCR_EN;

    /* --- liga o ADC e dispara --- */
    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { }
    ADC1->CR |= ADC_CR_ADSTART;
}

void DMA1_Channel4_5_6_7_IRQHandler(void)
{
    if (DMA1->ISR & DMA_TCIF(CH_UART)) {
        DMA1->IFCR = DMA_GIF(CH_UART);      /* GIF limpa todas as flags do canal */
        s_uart_busy = 0;
    }
}

void uart_dma_send(const void *data, uint32_t len)
{
    static int init;
    if (!init) {
        init = 1;
        RCC->AHBENR |= RCC_AHBENR_DMAEN;
        USART2->CR3 |= USART_CR3_DMAT;      /* USART pede DMA quando TXE = 1 */
        NVIC_SetPriority(DMA1_Channel4_5_6_7_IRQn, 2);
        NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);
    }
    while (s_uart_busy) { }

    DMA_Channel_TypeDef *ch = DMA_CH(CH_UART);
    ch->CCR   = 0;                          /* canal precisa estar desligado p/ configurar */
    ch->CPAR  = (uint32_t)&USART2->TDR;
    ch->CMAR  = (uint32_t)data;
    ch->CNDTR = len;
    s_uart_busy = 1;
    ch->CCR   = DMA_CCR_DIR                 /* memória -> periférico */
              | DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_EN;   /* 8/8 bits */
}

int uart_dma_busy(void) { return s_uart_busy; }

void dma_memcpy(void *dst, const void *src, uint32_t len_words)
{
    DMA_Channel_TypeDef *ch = DMA_CH(CH_M2M);
    RCC->AHBENR |= RCC_AHBENR_DMAEN;
    ch->CCR   = 0;
    DMA1->IFCR = DMA_GIF(CH_M2M);
    ch->CPAR  = (uint32_t)src;              /* no MEM2MEM, "periférico" = origem */
    ch->CMAR  = (uint32_t)dst;
    ch->CNDTR = len_words;
    ch->CCR   = DMA_CCR_MEM2MEM | (2U << DMA_CCR_MSIZE_Pos) | (2U << DMA_CCR_PSIZE_Pos)
              | DMA_CCR_MINC | DMA_CCR_PINC | DMA_CCR_EN;   /* DIR = 0: CPAR -> CMAR */
    while (!(DMA1->ISR & DMA_TCIF(CH_M2M))) { }
    DMA1->IFCR = DMA_GIF(CH_M2M);
    ch->CCR = 0;
}
