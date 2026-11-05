/*
 * dma_demo.c (F4) - DMA do STM32F446 (STREAMS + seleção de CANAL).
 *
 * Cada controlador (DMA1, DMA2) tem 8 streams; cada stream escolhe entre 8
 * "canais" (fontes de requisição) pelo campo CHSEL. Tabela no RM0390
 * ("DMA1/DMA2 request mapping"). Usados aqui:
 *   DMA2 Stream0 Canal0 -> ADC1
 *   DMA1 Stream6 Canal4 -> USART2_TX
 *   DMA2 Stream1        -> memória->memória (só o DMA2 faz M2M)
 */
#include "system.h"
#include "gpio.h"
#include "dma_demo.h"

static volatile int s_uart_busy;

static void stream_disable(DMA_Stream_TypeDef *s)
{
    s->CR &= ~DMA_SxCR_EN;
    while (s->CR & DMA_SxCR_EN) { }        /* só reconfigure depois que EN ler 0 */
}

void adc_dma_start(volatile uint16_t *buf, uint32_t n)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN | RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    (void)RCC->APB2ENR;
    gpio_mode(GPIOA, 0, GPIO_MODE_ANALOG);

    /* --- DMA2 Stream0, canal 0: ADC1->DR (16 bits) -> buf (16 bits), circular --- */
    DMA_Stream_TypeDef *s = &DMA2->S[0];
    stream_disable(s);
    DMA2->LIFCR = DMA_ALLF << dma_flag_shift(0);
    s->PAR  = (uint32_t)&ADC1->DR;
    s->M0AR = (uint32_t)buf;
    s->NDTR = n;
    s->FCR  = 0;                               /* modo direto (sem FIFO) */
    s->CR   = (0U << DMA_SxCR_CHSEL_Pos)
            | (1U << DMA_SxCR_MSIZE_Pos) | (1U << DMA_SxCR_PSIZE_Pos)
            | DMA_SxCR_MINC | DMA_SxCR_CIRC
            | (0U << DMA_SxCR_DIR_Pos)         /* periférico -> memória */
            | (1U << DMA_SxCR_PL_Pos);
    s->CR  |= DMA_SxCR_EN;

    /* --- ADC1: scan de 2 canais, contínuo, DMA com DDS --- */
    ADC_COMMON->CCR = (1U << ADC_CCR_ADCPRE_Pos) | ADC_CCR_TSVREFE;
    ADC1->CR1   = ADC_CR1_SCAN;
    ADC1->SMPR2 = (7U << 0);                    /* ch0: 480 ciclos  */
    ADC1->SMPR1 = (7U << (3U * (17U - 10U)));   /* ch17: 480 ciclos */
    ADC1->SQR1  = (1U << ADC_SQR1_L_Pos);       /* L = 1 -> 2 conversões */
    ADC1->SQR3  = (0U << 0) | (17U << 5);       /* SQ1 = ch0, SQ2 = ch17 */
    ADC1->CR2   = ADC_CR2_DMA | ADC_CR2_DDS     /* DDS: continua pedindo DMA após o fim do buffer */
                | ADC_CR2_CONT | ADC_CR2_ADON;
    delay_us(10);
    ADC1->CR2  |= ADC_CR2_SWSTART;
}

void DMA1_Stream6_IRQHandler(void)
{
    uint32_t sh = dma_flag_shift(6);
    if (DMA1->HISR & (DMA_TCIF << sh)) {
        DMA1->HIFCR = DMA_ALLF << sh;
        s_uart_busy = 0;
    }
}

void uart_dma_send(const void *data, uint32_t len)
{
    static int init;
    if (!init) {
        init = 1;
        RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
        USART2->CR3 |= USART_CR3_DMAT;
        NVIC_SetPriority(DMA1_Stream6_IRQn, 5);
        NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    }
    while (s_uart_busy) { }

    DMA_Stream_TypeDef *s = &DMA1->S[6];
    stream_disable(s);
    DMA1->HIFCR = DMA_ALLF << dma_flag_shift(6);   /* flags antigas impedem o EN */
    s->PAR  = (uint32_t)&USART2->DR;
    s->M0AR = (uint32_t)data;
    s->NDTR = len;
    s->FCR  = 0;
    s->CR   = (4U << DMA_SxCR_CHSEL_Pos)       /* canal 4 = USART2_TX */
            | (1U << DMA_SxCR_DIR_Pos)         /* memória -> periférico */
            | DMA_SxCR_MINC | DMA_SxCR_TCIE;   /* 8/8 bits */
    s_uart_busy = 1;
    s->CR  |= DMA_SxCR_EN;
}

int uart_dma_busy(void) { return s_uart_busy; }

void dma_memcpy(void *dst, const void *src, uint32_t len_words)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    DMA_Stream_TypeDef *s = &DMA2->S[1];
    stream_disable(s);
    DMA2->LIFCR = DMA_ALLF << dma_flag_shift(1);
    s->PAR  = (uint32_t)src;                   /* em M2M, PAR = origem */
    s->M0AR = (uint32_t)dst;
    s->NDTR = len_words;
    s->FCR  = DMA_SxFCR_DMDIS | 3U;            /* M2M exige FIFO; limiar cheio */
    s->CR   = (2U << DMA_SxCR_DIR_Pos)         /* memória -> memória */
            | (2U << DMA_SxCR_MSIZE_Pos) | (2U << DMA_SxCR_PSIZE_Pos)
            | DMA_SxCR_MINC | DMA_SxCR_PINC;
    s->CR  |= DMA_SxCR_EN;
    while (!(DMA2->LISR & (DMA_TCIF << dma_flag_shift(1)))) { }
    DMA2->LIFCR = DMA_ALLF << dma_flag_shift(1);
}
