/*
 * system.h - interface comum da "biblioteca" do curso AVR.
 *
 *   common/<mcu>/clock.c  -> clock_init()         (módulo 04)
 *   common/<mcu>/tick.c   -> millis(), delay_ms() (módulo 06)
 *   common/<mcu>/uart.c   -> serial               (módulo 07)
 *   common/lib/printf.c   -> kprintf()
 *   common/<mcu>/board.h  -> LED e botão da placa
 */
#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include <stddef.h>
#include "device.h"

extern uint32_t SystemCoreClock;

void     clock_init(void);
void     tick_init(void);
uint32_t millis(void);
void     delay_ms(uint32_t ms);
void     delay_us(uint32_t us);
void     system_init(void);          /* clock_init() + tick_init() */

void     uart_init(uint32_t baud);   /* 8N1 */
void     uart_putc(char c);
void     uart_puts(const char *s);
int      uart_getc(void);            /* -1 se não houver byte */

int      kprintf(const char *fmt, ...);

#include "board.h"

#endif
