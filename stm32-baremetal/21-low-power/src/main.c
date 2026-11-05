/*
 * Módulo 21 - Modos de baixo consumo: Sleep, Stop e Standby
 *
 * Menu pela serial (115200):
 *   1 - SLEEP   : CPU para, periféricos e clocks continuam. Acorda com
 *                 qualquer interrupção habilitada (aqui: botão B1 via EXTI).
 *   2 - STOP    : todos os clocks de alta velocidade param, RAM e registradores
 *                 preservados. Acorda pela EXTI (botão B1). Ao acordar o
 *                 SYSCLK é o HSI -> é preciso reconfigurar o clock.
 *   3 - STANDBY : domínio de 1,2 V desligado, RAM perdida. Acorda pelo pino
 *                 WKUP1 (PA0 = A0: encoste em 3V3), pelo NRST ou pelo RTC.
 *                 O "acordar" é um RESET: o programa começa do zero.
 *
 * Para medir corrente: na Nucleo, remova o jumper IDD (JP6/JP5) e ligue um
 * amperímetro no lugar.
 */
#include "system.h"
#include "gpio.h"

#if defined(STM32F0)
#  define BTN_IRQn  EXTI4_15_IRQn
void EXTI4_15_IRQHandler(void)
#else
#  define BTN_IRQn  EXTI15_10_IRQn
void EXTI15_10_IRQHandler(void)
#endif
{
    EXTI->PR = (1U << BTN_PIN);        /* só limpa: o objetivo é acordar */
}

static void button_exti_init(void)
{
#if defined(STM32F0)
    RCC->AHBENR  |= RCC_AHBENR_GPIOCEN;
#else
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
#endif
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    gpio_mode(BTN_PORT, BTN_PIN, GPIO_MODE_INPUT);
    SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~(0xFU << 4)) | (2U << 4);
    EXTI->FTSR |= (1U << BTN_PIN);
    EXTI->IMR  |= (1U << BTN_PIN);
    NVIC_EnableIRQ(BTN_IRQn);
}

static void uart_flush(void)
{
#if defined(STM32F0)
    while (!(USART2->ISR & USART_ISR_TC)) { }
#else
    while (!(USART2->SR & USART_SR_TC)) { }
#endif
}

static void enter_sleep(void)
{
    kprintf("SLEEP: aperte B1 para acordar...\n");
    uart_flush();
    /* SysTick acordaria a CPU a cada 1 ms: desliga a interrupção dele.
     * Qualquer IRQ habilitada no NVIC acorda do Sleep (aqui só a EXTI). */
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT;
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP;    /* SLEEPDEEP = 0 -> Sleep */
    __WFI();
    SysTick->CTRL |= SysTick_CTRL_TICKINT;
    kprintf("acordou do SLEEP\n");
}

static void enter_stop(void)
{
    kprintf("STOP: aperte B1 para acordar...\n");
    uart_flush();
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT;

    EXTI->PR = 0xFFFFFFFFU;            /* pendências antigas impediriam o Stop */
    PWR->CR &= ~PWR_CR_PDDS;           /* PDDS = 0: Stop (não Standby)          */
    PWR->CR |=  PWR_CR_LPDS;           /* regulador em low-power durante o Stop */
    SCB->SCR |= SCB_SCR_SLEEPDEEP;
    __WFI();
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP;

    clock_init();                      /* voltou rodando do HSI: reconfigura PLL */
    SysTick->CTRL |= SysTick_CTRL_TICKINT;
    kprintf("acordou do STOP, clock restaurado para %u Hz\n", SystemCoreClock);
}

static void enter_standby(void)
{
    kprintf("STANDBY: encoste A0 (PA0) em 3V3 ou aperte RESET...\n");
    uart_flush();
    PWR->CR  |= PWR_CR_CWUF;           /* limpa WUF (senão acorda na hora) */
    PWR->CSR |= PWR_CSR_EWUP1;         /* habilita WKUP1 = PA0, borda de subida */
    PWR->CR  |= PWR_CR_PDDS;           /* PDDS = 1: Standby */
    SCB->SCR |= SCB_SCR_SLEEPDEEP;
    __WFI();
    /* nunca chega aqui */
}

int main(void)
{
    system_init();
    uart_init(115200);
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR;

#if defined(STM32F0)
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
#else
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
#endif
    gpio_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT);

    kprintf("\n=== Modulo 21: low-power ===\n");
    if (PWR->CSR & PWR_CSR_SBF) {      /* Standby Flag: viemos de um Standby */
        kprintf("*** acordou do STANDBY (SBF=1, WUF=%u) ***\n", (PWR->CSR & PWR_CSR_WUF) ? 1U : 0U);
        PWR->CR |= PWR_CR_CSBF | PWR_CR_CWUF;
    }
    PWR->CSR &= ~PWR_CSR_EWUP1;

    /* Permite depurar nos modos de baixo consumo (DBG_SLEEP/STOP/STANDBY) */
#if defined(STM32F0)
    RCC->APB2ENR |= RCC_APB2ENR_DBGMCUEN;
#endif
    DBGMCU->CR |= 7U;

    button_exti_init();

    for (;;) {
        kprintf("\n1 = sleep, 2 = stop, 3 = standby > ");
        int c;
        uint32_t t = 0;
        while ((c = uart_getc()) < 0) {          /* pisca enquanto espera */
            if (millis() - t > 250) { t = millis(); gpio_toggle(LED_PORT, LED_PIN); }
        }
        kprintf("%c\n", c);
        gpio_clear(LED_PORT, LED_PIN);
        if (c == '1') enter_sleep();
        else if (c == '2') enter_stop();
        else if (c == '3') enter_standby();
    }
}
