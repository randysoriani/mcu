/*
 * board.h (Arduino Uno / Nano, ATmega328P)
 *
 *   LED "L" da placa: pino digital 13 = PB5 (acende com nível alto)
 *   Botão: a placa não tem botão de usuário. Ligue um botão entre o
 *   pino digital 2 (PD2, que também é a interrupção externa INT0) e o GND;
 *   o pull-up interno mantém o pino em 1 quando o botão está solto.
 */
#ifndef BOARD_H
#define BOARD_H
#include "m328p.h"

#define LED_BIT 5
#define BTN_BIT 2

static inline void led_init(void)   { DDRB |= (1U << LED_BIT); }
static inline void led_on(void)     { PORTB |= (1U << LED_BIT); }
static inline void led_off(void)    { PORTB &= (uint8_t)~(1U << LED_BIT); }
static inline void led_toggle(void) { PINB = (1U << LED_BIT); }   /* escrever 1 no PIN inverte o pino */

static inline void btn_init(void)
{
    DDRD  &= (uint8_t)~(1U << BTN_BIT);   /* entrada */
    PORTD |= (1U << BTN_BIT);             /* pull-up interno */
}
static inline uint8_t btn_pressed(void) { return !(PIND & (1U << BTN_BIT)); }   /* ativo em 0 */

#endif
