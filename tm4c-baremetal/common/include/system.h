/*
 * system.h - interface comum da "biblioteca" do curso TM4C.
 *
 *   common/<mcu>/clock.c  -> clock_init() (módulo 04): 80 MHz (123) / 120 MHz (129)
 *   common/lib/systick.c  -> base de tempo de 1 ms (módulo 06)
 *   common/lib/uart.c     -> UART0 no ICDI (virtual COM), módulo 07
 *   common/lib/printf.c   -> kprintf() minimalista
 */
#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include <stddef.h>
#include "device.h"

extern uint32_t SystemCoreClock;   /* sysclk em Hz (núcleo e periféricos) */

void     clock_init(void);
void     systick_init(void);
uint32_t millis(void);
void     delay_ms(uint32_t ms);
void     delay_us(uint32_t us);
void     system_init(void);        /* clock_init() + systick_init() */

/* UART0: PA0 = RX, PA1 = TX -> ICDI -> /dev/ttyACM0, 8N1 */
void     uart_init(uint32_t baud);
void     uart_putc(char c);
void     uart_puts(const char *s);
int      uart_getc(void);          /* -1 se não houver byte */

/* %d %i %u %x %X %c %s %p %% e largura (sem float) */
int      kprintf(const char *fmt, ...);

/* ---- LED e botão de cada LaunchPad ---- */
#if defined(TM4C123)
#  define LED_PORT      GPIOF
#  define LED_PORT_NUM  PORTF
#  define LED_PIN       3U       /* LED RGB: PF1 vermelho, PF2 azul, PF3 verde */
#  define BTN_PORT      GPIOF
#  define BTN_PORT_NUM  PORTF
#  define BTN_PIN       4U       /* SW1, ativo em nível BAIXO, precisa pull-up interno */
#  define BTN_IRQn      GPIOF_IRQn
#  define BTN_IRQHandler GPIOF_Handler
#else
#  define LED_PORT      GPION
#  define LED_PORT_NUM  PORTN
#  define LED_PIN       1U       /* D1 = PN1, D2 = PN0, D3 = PF4, D4 = PF0 */
#  define BTN_PORT      GPIOJ
#  define BTN_PORT_NUM  PORTJ
#  define BTN_PIN       0U       /* USR_SW1 = PJ0 (SW2 = PJ1), ativo em BAIXO */
#  define BTN_IRQn      GPIOJ_IRQn
#  define BTN_IRQHandler GPIOJ_Handler
#endif

#endif
