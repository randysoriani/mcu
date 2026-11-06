/*
 * board.h (EVK1100, AT32UC3A0512)
 *
 *   LED0..LED3 = PB27..PB30, acesos com nível BAIXO
 *   Botão: botão central (PUSH) do joystick em PA20, ativo em nível baixo
 *   Confira os pinos no esquemático da sua revisão da placa.
 */
#ifndef BOARD_H
#define BOARD_H
#include "uc3a.h"

#define LED_PIN  PB(27)
#define BTN_PIN  PA(20)

static inline void led_init(void)
{
    GPIO_Port *p = GPIO_PORT(LED_PIN);
    p->GPER.SET = GPIO_MASK(LED_PIN);     /* pino controlado pelo GPIO */
    p->OVR.SET  = GPIO_MASK(LED_PIN);     /* 1 = apagado (ativo em baixo) */
    p->ODER.SET = GPIO_MASK(LED_PIN);     /* saída */
}
static inline void led_on(void)     { GPIO_PORT(LED_PIN)->OVR.CLR = GPIO_MASK(LED_PIN); }
static inline void led_off(void)    { GPIO_PORT(LED_PIN)->OVR.SET = GPIO_MASK(LED_PIN); }
static inline void led_toggle(void) { GPIO_PORT(LED_PIN)->OVR.TGL = GPIO_MASK(LED_PIN); }

static inline void btn_init(void)
{
    GPIO_Port *p = GPIO_PORT(BTN_PIN);
    p->GPER.SET = GPIO_MASK(BTN_PIN);
    p->ODER.CLR = GPIO_MASK(BTN_PIN);     /* entrada */
    p->PUER.SET = GPIO_MASK(BTN_PIN);     /* pull-up */
}
static inline uint8_t btn_pressed(void) { return !(GPIO_PORT(BTN_PIN)->PVR.VAL & GPIO_MASK(BTN_PIN)); }

#endif
