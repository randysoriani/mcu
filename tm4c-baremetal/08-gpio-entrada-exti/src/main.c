/*
 * Módulo 08 - GPIO como entrada e interrupções de GPIO
 *
 *   TM4C123: SW1 = PF4 (interrupção), SW2 = PF0 (pino PROTEGIDO: precisa unlock)
 *   TM4C129: USR_SW1 = PJ0 (interrupção), USR_SW2 = PJ1
 * Os dois botões são ativos em nível baixo e precisam do pull-up interno.
 * O botão 1 gera interrupção na borda de descida (com debounce); o botão 2 é
 * lido por polling.
 */
#include "system.h"
#include "gpio.h"

#define DEBOUNCE_MS   50U
#define LONG_PRESS_MS 1000U

#if defined(TM4C123)
#  define BTN2_PORT GPIOF
#  define BTN2_PIN  0U
#else
#  define BTN2_PORT GPIOJ
#  define BTN2_PIN  1U
#endif

static volatile uint32_t g_press_count, g_last_edge_ms;
static volatile uint8_t  g_event;

/* No TM4C cada PORTA tem a sua interrupção; os 8 pinos dividem o vetor */
void BTN_IRQHandler(void)
{
    uint32_t mis = BTN_PORT->MIS;          /* quais pinos dispararam */
    BTN_PORT->ICR = mis;                   /* limpa escrevendo 1 */
    if (mis & (1U << BTN_PIN)) {
        uint32_t now = millis();
        if (now - g_last_edge_ms >= DEBOUNCE_MS) {
            g_last_edge_ms = now;
            g_press_count++;
            g_event = 1;
        }
    }
}

static void buttons_init(void)
{
    gpio_port_enable(BTN_PORT_NUM);
#if defined(TM4C123)
    gpio_unlock(GPIOF, 0);                 /* PF0 é NMI por padrão: LOCK + CR */
#endif
    gpio_input(BTN_PORT, BTN_PIN, GPIO_PULLUP);
    gpio_input(BTN2_PORT, BTN2_PIN, GPIO_PULLUP);

    BTN_PORT->IM  &= ~(1U << BTN_PIN);     /* mascara durante a configuração   */
    BTN_PORT->IS  &= ~(1U << BTN_PIN);     /* 0 = sensível a borda             */
    BTN_PORT->IBE &= ~(1U << BTN_PIN);     /* 0 = uma borda só (definida por IEV) */
    BTN_PORT->IEV &= ~(1U << BTN_PIN);     /* 0 = borda de descida             */
    BTN_PORT->ICR  =  (1U << BTN_PIN);     /* limpa pendência antiga           */
    BTN_PORT->IM  |=  (1U << BTN_PIN);     /* habilita                         */

    NVIC_SetPriority(BTN_IRQn, 3);
    NVIC_EnableIRQ(BTN_IRQn);
}

int main(void)
{
    system_init();
    uart_init(115200);
    gpio_port_enable(LED_PORT_NUM);
    gpio_output(LED_PORT, LED_PIN);
    buttons_init();

    kprintf("\n=== Modulo 08: GPIO entrada + interrupcao ===\n");
    kprintf("Botao 1: curto = LED, longo = mensagem. Botao 2 lido por polling.\n");

    uint32_t b2_prev = gpio_read(BTN2_PORT, BTN2_PIN);
    for (;;) {
        if (g_event) {
            g_event = 0;
            uint32_t t0 = millis();
            while (gpio_read(BTN_PORT, BTN_PIN) == 0 && millis() - t0 < 5000) { }
            uint32_t dur = millis() - t0;
            if (dur >= LONG_PRESS_MS) {
                kprintf("clique LONGO (%u ms), total = %u\n", dur, g_press_count);
            } else {
                gpio_toggle(LED_PORT, LED_PIN);
                kprintf("clique curto (%u ms), total = %u\n", dur, g_press_count);
            }
        }
        uint32_t b2 = gpio_read(BTN2_PORT, BTN2_PIN);
        if (b2 != b2_prev) {
            b2_prev = b2;
            kprintf("botao 2 = %u (DATA = 0x%02X)\n", b2, BTN2_PORT->DATA & 0xFFU);
            delay_ms(DEBOUNCE_MS);
        }
        __WFI();
    }
}
