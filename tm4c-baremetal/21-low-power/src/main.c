/*
 * Módulo 21 - Modos de baixo consumo: Sleep, Deep-Sleep e Hibernação
 *
 * Menu pela serial (115200):
 *   1 - SLEEP      : CPU para (WFI), clocks seguem. Acorda pelo botão (IRQ de GPIO).
 *   2 - DEEP-SLEEP : SLEEPDEEP = 1. O clock vai para o configurado em DSLPCLKCFG
 *                    (aqui PIOSC) e só os periféricos habilitados em DCGCxxx recebem
 *                    clock. Ao acordar o hardware VOLTA SOZINHO ao clock de run
 *                    (diferente do STM32, onde é preciso reconfigurar o PLL).
 *   3 - HIBERNAÇÃO : desliga quase tudo, exceto o módulo HIB (RTC + 64 bytes).
 *                    Acorda pelo alarme do RTC após 5 s; o "acordar" é um RESET.
 *
 * Para medir corrente: 123 -> jumper JP4 (I_MCU); 129 -> JP2 (corrente do MCU).
 */
#include "system.h"
#include "gpio.h"

#define SYSCTL_DCGCGPIO REG32(SYSCTL_BASE + 0x808)   /* deep-sleep clock gating GPIO */
#define SYSCTL_DCGCUART REG32(SYSCTL_BASE + 0x818)
#define HIB_WAKE_MAGIC  0xB0A7D1AAU

void BTN_IRQHandler(void)
{
    BTN_PORT->ICR = (1U << BTN_PIN);     /* só limpa: o objetivo é acordar */
}

static void hib_wait(void) { while (!(HIB->CTL & HIB_CTL_WRC)) { } }

static void button_irq_init(void)
{
    gpio_port_enable(BTN_PORT_NUM);
    gpio_input(BTN_PORT, BTN_PIN, GPIO_PULLUP);
    BTN_PORT->IS  &= ~(1U << BTN_PIN);
    BTN_PORT->IBE &= ~(1U << BTN_PIN);
    BTN_PORT->IEV &= ~(1U << BTN_PIN);
    BTN_PORT->ICR  =  (1U << BTN_PIN);
    BTN_PORT->IM  |=  (1U << BTN_PIN);
    NVIC_EnableIRQ(BTN_IRQn);
}

static void uart_flush(void) { while (UART0->FR & UART_FR_BUSY) { } }

static void enter_sleep(void)
{
    kprintf("SLEEP: aperte o botao para acordar...\n");
    uart_flush();
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT;      /* senão o tick acorda a cada 1 ms */
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP;
    __WFI();
    SysTick->CTRL |= SysTick_CTRL_TICKINT;
    kprintf("acordou do SLEEP\n");
}

static void enter_deep_sleep(void)
{
    kprintf("DEEP-SLEEP: aperte o botao para acordar...\n");
    uart_flush();
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT;
    SYSCTL_DSLPCLKCFG = (1U << 4);               /* DSOSCSRC = PIOSC (16 MHz), sem divisor */
    SYSCTL_DCGCGPIO   = (1U << BTN_PORT_NUM);    /* só a porta do botão recebe clock */
    SCB->SCR |= SCB_SCR_SLEEPDEEP;
    __WFI();
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP;
    SysTick->CTRL |= SysTick_CTRL_TICKINT;
    kprintf("acordou do DEEP-SLEEP (clock de volta a %u Hz automaticamente)\n", SystemCoreClock);
}

static void enter_hibernate(void)
{
    kprintf("HIBERNACAO: acordando pelo RTC em 5 s...\n");
    uart_flush();
    periph_enable(&SYSCTL_RCGCHIB, &SYSCTL_PRHIB, 0);
    if (!(HIB->CTL & HIB_CTL_CLK32EN)) {
        hib_wait(); HIB->CTL = HIB_CTL_CLK32EN; hib_wait();
        delay_ms(1500);
    }
    hib_wait(); HIB->CTL |= HIB_CTL_RTCEN;  hib_wait();
    hib_wait(); HIB->DATA[1] = HIB_WAKE_MAGIC;
    hib_wait(); HIB->RTCM0 = HIB->RTCC + 5U;
    hib_wait(); HIB->IC = HIB_INT_RTCALT0;
    hib_wait(); HIB->CTL |= HIB_CTL_RTCWEN;      /* acorda no match do RTC */
    hib_wait(); HIB->CTL |= HIB_CTL_HIBREQ;      /* pede a hibernação      */
    for (;;) { }                                 /* desliga em instantes   */
}

int main(void)
{
    system_init();
    uart_init(115200);
    gpio_port_enable(LED_PORT_NUM);
    gpio_output(LED_PORT, LED_PIN);

    kprintf("\n=== Modulo 21: low-power ===\n");
    periph_enable(&SYSCTL_RCGCHIB, &SYSCTL_PRHIB, 0);
    if (HIB->DATA[1] == HIB_WAKE_MAGIC) {
        kprintf("*** acordou da HIBERNACAO (HIB RIS = 0x%02X) ***\n", HIB->RIS);
        hib_wait(); HIB->DATA[1] = 0;
        hib_wait(); HIB->IC = 0x1FU; hib_wait();
    }
    button_irq_init();

    for (;;) {
        kprintf("\n1 = sleep, 2 = deep-sleep, 3 = hibernacao > ");
        int c;
        uint32_t t = 0;
        while ((c = uart_getc()) < 0) {
            if (millis() - t > 250) { t = millis(); gpio_toggle(LED_PORT, LED_PIN); }
        }
        kprintf("%c\n", c);
        gpio_clear(LED_PORT, LED_PIN);
        if (c == '1') enter_sleep();
        else if (c == '2') enter_deep_sleep();
        else if (c == '3') enter_hibernate();
    }
}
