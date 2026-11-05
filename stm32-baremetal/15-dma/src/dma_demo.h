/*
 * dma_demo.h - funções do módulo 15 (f0/dma_demo.c, f4/dma_demo.c)
 */
#ifndef DMA_DEMO_H
#define DMA_DEMO_H
#include <stdint.h>
#include <stddef.h>

/* ADC em modo contínuo, varrendo A0 e VREFINT, despejando via DMA circular
 * em buf[] intercalado: buf[0]=A0, buf[1]=VREF, buf[2]=A0, ... (n par) */
void adc_dma_start(volatile uint16_t *buf, uint32_t n);

/* Envia 'len' bytes pela USART2 via DMA (não bloqueia) */
void uart_dma_send(const void *data, uint32_t len);
int  uart_dma_busy(void);

/* Cópia memória -> memória por DMA (bloqueante, para medir) */
void dma_memcpy(void *dst, const void *src, uint32_t len_words);

#endif
