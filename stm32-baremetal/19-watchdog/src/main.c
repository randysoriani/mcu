/*
 * Módulo 19 - Watchdogs (IWDG e WWDG)
 *
 * DEMO 1 (padrão) - IWDG, timeout de ~1 s:
 *     o laço "alimenta" o cão a cada 200 ms. SEGURE o botão B1 para simular
 *     um travamento -> após ~1 s o chip reinicia e informa a causa.
 * DEMO 2 - WWDG (janela):
 *     a recarga só é aceita DENTRO de uma janela de tempo. Recarga a cada
 *     ~30 ms (ok). Segure B1 -> recarga cedo demais (5 ms) -> reset.
 *
 *   make clean && make MCU=f0 EXTRA=-DDEMO=2
 */
#include "system.h"
#include "gpio.h"

#ifndef DEMO
#define DEMO 1
#endif

static void print_reset_cause(void)
{
    uint32_t csr = RCC->CSR;
    kprintf("Causa do reset:");
    if (csr & RCC_CSR_LPWRRSTF) kprintf(" LOW-POWER");
    if (csr & RCC_CSR_WWDGRSTF) kprintf(" WWDG");
    if (csr & RCC_CSR_IWDGRSTF) kprintf(" IWDG");
    if (csr & RCC_CSR_SFTRSTF)  kprintf(" SOFTWARE");
    if (csr & RCC_CSR_PORRSTF)  kprintf(" POWER-ON");
    if (csr & RCC_CSR_PINRSTF)  kprintf(" PINO-NRST");
#if defined(STM32F4)
    if (csr & RCC_CSR_BORRSTF)  kprintf(" BROWN-OUT");
#else
    if (csr & RCC_CSR_OBLRSTF)  kprintf(" OPTION-BYTES");
#endif
    kprintf("  (RCC_CSR = 0x%08X)\n", csr);
    RCC->CSR |= RCC_CSR_RMVF;            /* limpa as flags para o próximo reset */
}

static void freeze_on_debug(void)
{
    /* Com o debugger parado num breakpoint, os watchdogs também param */
#if defined(STM32F0)
    RCC->APB2ENR |= RCC_APB2ENR_DBGMCUEN;
#endif
    DBGMCU->APB1FZ |= (1U << 12) | (1U << 11);   /* DBG_IWDG_STOP, DBG_WWDG_STOP */
}

#if DEMO == 1
/* ------------------------------------------------------------------ */
static void iwdg_init_ms(uint32_t ms)
{
#if defined(STM32F0)
    const uint32_t lsi = 40000;          /* LSI nominal (30..50 kHz!) */
#else
    const uint32_t lsi = 32000;          /* LSI nominal (17..47 kHz!) */
#endif
    IWDG->KR  = 0xCCCC;                  /* liga o IWDG (e o LSI, automaticamente) */
    IWDG->KR  = 0x5555;                  /* libera escrita em PR e RLR */
    IWDG->PR  = 3;                       /* prescaler /32 */
    IWDG->RLR = (lsi / 32U) * ms / 1000U;/* 12 bits: máx 4095 */
    while (IWDG->SR & 3U) { }            /* PVU/RVU: espera a atualização (domínio LSI) */
    IWDG->KR  = 0xAAAA;                  /* recarrega */
}

static inline void iwdg_feed(void) { IWDG->KR = 0xAAAA; }

static void demo(void)
{
    iwdg_init_ms(1000);
    kprintf("IWDG ativo (1 s). Segure B1 para parar de alimentar.\n");
    for (uint32_t n = 0;; n++) {
        if (gpio_read(BTN_PORT, BTN_PIN) != 0) {     /* solto: alimenta */
            iwdg_feed();
            gpio_toggle(LED_PORT, LED_PIN);
            if (n % 5 == 0) kprintf("vivo (%u)\n", n);
        } else {
            kprintf("travado...\n");
        }
        delay_ms(200);
    }
}

#elif DEMO == 2
/* ------------------------------------------------------------------ */
/*
 * WWDG: contador de 7 bits decrescente, clock = PCLK/4096/2^WDGTB.
 *   - reset quando T6 cai (contador passa de 0x40 para 0x3F)
 *   - reset se recarregar com contador > W (cedo demais)
 *   F0: 48 MHz/4096/8 = 1465 Hz -> 0,68 ms/tick | F4: 45 MHz/4096/8 = 1373 Hz -> 0,73 ms/tick
 *   T = 0x7F, W = 0x5F: janela abre após 32 ticks (~22 ms) e fecha após 64 (~45 ms)
 */
#define WWDG_T 0x7FU
#define WWDG_W 0x5FU

static void demo(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_WWDGEN;
    WWDG->CFR = (3U << 7) | WWDG_W;               /* WDGTB = 11 (/8), janela W */
    WWDG->CR  = (1U << 7) | WWDG_T;               /* WDGA = 1 liga; não dá para desligar */
    kprintf("WWDG ativo. Recarga a cada ~30 ms. Segure B1 = recarga a 5 ms.\n");

    for (uint32_t n = 0;; n++) {
        uint32_t espera = gpio_read(BTN_PORT, BTN_PIN) ? 30U : 5U;
        delay_ms(espera);
        WWDG->CR = WWDG_T;                        /* recarrega (WDGA já é 1) */
        if (n % 33 == 0) { gpio_toggle(LED_PORT, LED_PIN); kprintf("ok, contador = 0x%02X\n", WWDG->CR & 0x7FU); }
    }
}
#endif

int main(void)
{
    system_init();
    uart_init(115200);

#if defined(STM32F0)
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOCEN;
#else
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;
#endif
    gpio_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT);
    gpio_mode(BTN_PORT, BTN_PIN, GPIO_MODE_INPUT);

    kprintf("\n=== Modulo 19: watchdog (DEMO %d) ===\n", DEMO);
    print_reset_cause();
    freeze_on_debug();
    demo();
}
