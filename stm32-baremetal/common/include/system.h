/*
 * system.h - interface comum da "biblioteca" do curso.
 *
 *   common/<mcu>/clock.c  -> clock_init() (módulo 04)
 *   common/lib/systick.c  -> base de tempo de 1 ms (módulo 06)
 *   common/<mcu>/uart.c   -> USART2 no ST-LINK VCP (módulo 07)
 *   common/lib/printf.c   -> kprintf() minimalista e portátil
 *
 * No Makefile do módulo, a variável LIB escolhe quais deles linkar.
 */
#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include <stddef.h>
#include "device.h"

/* ---- clock (clock.c) e tempo (systick.c) ---- */
extern uint32_t SystemCoreClock;   /* HCLK em Hz  */
extern uint32_t APB1Clock;         /* PCLK1 em Hz */
extern uint32_t APB2Clock;         /* PCLK2 em Hz (no F0 = PCLK) */

void     clock_init(void);         /* F0: 48 MHz, F4: 180 MHz (a partir do HSI) */
void     systick_init(void);       /* tick de 1 ms */
uint32_t millis(void);
void     delay_ms(uint32_t ms);
void     delay_us(uint32_t us);    /* busy-wait aproximado */
void     system_init(void);        /* clock_init() + systick_init()   */

/* ---- console serial (uart.c) - USART2 PA2/PA3 -> ST-LINK VCP, 8N1 ---- */
void     uart_init(uint32_t baud);
void     uart_putc(char c);
void     uart_puts(const char *s);
int      uart_getc(void);          /* -1 se não houver byte */

/* ---- printf mínimo (printf.c) - %d %i %u %x %X %c %s %p %% e largura ----
 * Sem __attribute__((format)): aqui int e long têm 32 bits, então %u/%d/%x
 * aceitam uint32_t/int32_t direto (o GCC reclamaria que uint32_t é "long"). */
int      kprintf(const char *fmt, ...);

/* ---- LED e botão da Nucleo-64 (iguais na F072RB e F446RE) ---- */
#define LED_PORT   GPIOA
#define LED_PIN    5U      /* LD2 (verde) - ativo em nível alto  */
#define BTN_PORT   GPIOC
#define BTN_PIN    13U     /* B1 (azul)   - ativo em nível BAIXO */

#endif
