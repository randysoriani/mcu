/*
 * Módulo 08 - GPIO como entrada, EXTI e NVIC
 *
 * Botão B1 (PC13, ativo em nível baixo, pull-up externo na Nucleo).
 *   - Interrupção EXTI13 na borda de descida (aperto)
 *   - Debounce por software usando millis()
 *   - Distingue clique curto (alterna o LED) de clique longo (> 1 s)
 *   - Uma segunda "entrada" lida por polling: PA10 (D2) com pull-up interno
 */
#include "system.h"
#include "gpio.h"

#define DEBOUNCE_MS   50U
#define LONG_PRESS_MS 1000U

static volatile uint32_t g_press_count;
static volatile uint32_t g_last_edge_ms;
static volatile uint8_t  g_event;          /* 1 = apertou (já filtrado) */

#if defined(STM32F0)
#  define BTN_IRQn         EXTI4_15_IRQn     /* F0 agrupa as linhas 4..15 numa IRQ */
#  define BTN_IRQHandler   EXTI4_15_IRQHandler
#else
#  define BTN_IRQn         EXTI15_10_IRQn    /* F4: linhas 10..15 numa IRQ        */
#  define BTN_IRQHandler   EXTI15_10_IRQHandler
#endif

void BTN_IRQHandler(void)
{
    /* Várias linhas compartilham o vetor: confira QUAL disparou */
    if (EXTI->PR & (1U << BTN_PIN)) {
        EXTI->PR = (1U << BTN_PIN);          /* limpa escrevendo 1 (rc_w1) */

        uint32_t now = millis();
        if (now - g_last_edge_ms >= DEBOUNCE_MS) {
            g_last_edge_ms = now;
            g_press_count++;
            g_event = 1;
        }
    }
}

static void button_exti_init(void)
{
#if defined(STM32F0)
    RCC->AHBENR  |= RCC_AHBENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;    /* SYSCFG tem o mux da EXTI */
#else
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
#endif

    /* PC13 como entrada, sem pull interno (a Nucleo já tem pull-up de 4k7) */
    gpio_mode(BTN_PORT, BTN_PIN, GPIO_MODE_INPUT);
    gpio_pull(BTN_PORT, BTN_PIN, GPIO_NOPULL);

    /* EXTICR[3] controla as linhas 12..15, 4 bits cada. Linha 13 = bits [7:4].
     * Valor = porta: 0 = A, 1 = B, 2 = C, 3 = D ... */
    SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~(0xFU << 4)) | (2U << 4);

    EXTI->FTSR |=  (1U << BTN_PIN);          /* borda de descida            */
    EXTI->RTSR &= ~(1U << BTN_PIN);          /* sem borda de subida         */
    EXTI->PR    =  (1U << BTN_PIN);          /* limpa pendência antiga      */
    EXTI->IMR  |=  (1U << BTN_PIN);          /* desmascara a interrupção    */

    NVIC_SetPriority(BTN_IRQn, 3);
    NVIC_EnableIRQ(BTN_IRQn);
}

int main(void)
{
    system_init();
    uart_init(115200);

#if defined(STM32F0)
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
#else
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
#endif
    gpio_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT);

    /* PA10 (Arduino D2): entrada com pull-up interno -> ligue um fio ao GND */
    gpio_mode(GPIOA, 10, GPIO_MODE_INPUT);
    gpio_pull(GPIOA, 10, GPIO_PULLUP);

    button_exti_init();

    kprintf("\n=== Modulo 08: GPIO entrada + EXTI ===\n");
    kprintf("Aperte B1 (curto = LED, longo = mensagem). Aterre D2/PA10 para teste.\n");

    uint32_t pa10_prev = gpio_read(GPIOA, 10);
    for (;;) {
        if (g_event) {
            g_event = 0;
            /* mede quanto tempo o botão ficou apertado (nível baixo) */
            uint32_t t0 = millis();
            while (gpio_read(BTN_PORT, BTN_PIN) == 0) {
                if (millis() - t0 > 5000) break;
            }
            uint32_t dur = millis() - t0;
            if (dur >= LONG_PRESS_MS) {
                kprintf("clique LONGO (%u ms), total = %u\n", dur, g_press_count);
            } else {
                gpio_toggle(LED_PORT, LED_PIN);
                kprintf("clique curto (%u ms), total = %u\n", dur, g_press_count);
            }
        }

        /* polling simples com detecção de mudança */
        uint32_t pa10 = gpio_read(GPIOA, 10);
        if (pa10 != pa10_prev) {
            pa10_prev = pa10;
            kprintf("PA10 = %u (IDR = 0x%04X)\n", pa10, GPIOA->IDR & 0xFFFFU);
            delay_ms(DEBOUNCE_MS);
        }
        __WFI();
    }
}
