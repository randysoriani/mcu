/*
 * Módulo 08 - Entradas digitais e interrupções de pino (AT32UC3A / EVK1100)
 *
 *   Botão PUSH do joystick (PA20, ativo em baixo) -> interrupção do GPIO na
 *   borda de descida. O LED0 inverte a cada aperto.
 *
 * No UC3 as interrupções de pino ficam no próprio GPIO (grupo 2 do INTC).
 * Cada linha do grupo atende 8 pinos: o pino 20 é a linha 20/8 = 2.
 */
#include "system.h"

static volatile uint16_t g_count;
static volatile uint32_t g_last_ms;
static volatile uint8_t  g_event;

ISR_ATTR static void gpio_handler(void)
{
    GPIO_Port *p = GPIO_PORT(BTN_PIN);
    if (p->IFR.VAL & GPIO_MASK(BTN_PIN)) {
        p->IFR.CLR = GPIO_MASK(BTN_PIN);          /* IFRC: limpa a flag */
        uint32_t now = millis();
        if (now - g_last_ms >= 50) {              /* debounce */
            g_last_ms = now;
            g_count++;
            g_event = 1;
        }
    }
}

int main(void)
{
    system_init();
    uart_init(115200);
    led_init();
    btn_init();                                   /* PA20 entrada com pull-up */

    GPIO_Port *p = GPIO_PORT(BTN_PIN);
    uint32_t   m = GPIO_MASK(BTN_PIN);
    p->IMR0.CLR = m;                              /* IMR1:IMR0 = 10 -> borda de descida */
    p->IMR1.SET = m;
    p->GFER.SET = m;                              /* filtro de glitch */
    p->IFR.CLR  = m;
    p->IER.SET  = m;                              /* habilita a interrupção do pino */
    intc_register(IRQ_GRP_GPIO, gpio_handler, 1);

    kprintf("\n=== Modulo 08: entradas e interrupcoes de pino ===\n");
    for (;;) {
        if (g_event) {
            g_event = 0;
            led_toggle();
            kprintf("botao: %u apertos (PA20 = %lu)\n", g_count,
                    (unsigned long)((p->PVR.VAL >> 20) & 1UL));
        }
    }
}
