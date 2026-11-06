/*
 * Módulo 18 - Sleep e economia de energia (AT32UC3A)
 *
 *   Instrução "sleep N": 0 idle, 1 frozen, 2 standby, 3 stop, 5 deepstop, 6 static.
 *   Neste exemplo: IDLE (só a CPU para), acordado pela interrupção do botão
 *   do joystick (GPIO). As máscaras PBAMASK/PBBMASK/HSBMASK do PM desligam
 *   o clock de periféricos não usados.
 */
#include "system.h"

ISR_ATTR static void gpio_handler(void)
{
    GPIO_PORT(BTN_PIN)->IFR.CLR = GPIO_MASK(BTN_PIN);
}

int main(void)
{
    system_init();
    uart_init(115200);
    led_init();
    btn_init();

    GPIO_Port *p = GPIO_PORT(BTN_PIN);
    p->IMR0.CLR = GPIO_MASK(BTN_PIN);             /* borda de descida */
    p->IMR1.SET = GPIO_MASK(BTN_PIN);
    p->IFR.CLR  = GPIO_MASK(BTN_PIN);
    p->IER.SET  = GPIO_MASK(BTN_PIN);
    intc_register(IRQ_GRP_GPIO, gpio_handler, 1);

    kprintf("\n=== Modulo 18: sleep ===\nPBAMASK = 0x%08lx (1 = clock ligado)\n", (unsigned long)PM->PBAMASK);
    for (;;) {
        kprintf("1 = idle (acorda com o botao) > ");
        int c;
        uint32_t t = 0;
        while ((c = uart_getc()) < 0) {
            if (millis() - t > 250) { t = millis(); led_toggle(); }
        }
        kprintf("%c\n", c);
        if (c == '1') {
            led_off();
            while (!(USART0->CSR & USART_CSR_TXEMPTY)) { }
            mtsr(SYSREG_COMPARE, 0);              /* desliga o tick (COMPARE = 0) para não acordar */
            cpu_sleep(0);
            tick_init();                          /* religa o tick */
            kprintf("acordou do IDLE\n");
        }
    }
}
