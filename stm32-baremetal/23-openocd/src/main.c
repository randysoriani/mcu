/*
 * Módulo 23 - OpenOCD: programa "alvo" para os exercícios
 *
 * Tudo aqui foi pensado para ser observado e controlado PELO DEBUGGER, com
 * o programa rodando (sem parar a CPU):
 *
 *   periodo_ms        -> mude o ritmo do LED escrevendo na RAM (mww)
 *   contador          -> leia com mdw e veja ele subir
 *   comando           -> escreva 1, 2 ou 3 para o programa executar algo
 *   usar_semihosting  -> 1 = mensagens saem no console do OpenOCD (semihosting)
 *
 * Os endereços são impressos na UART e podem ser vistos com:
 *   make MCU=f4 symbols
 */
#include "system.h"
#include "gpio.h"

volatile uint32_t periodo_ms       = 500;
volatile uint32_t contador         = 0;
volatile uint32_t comando          = 0;
volatile uint32_t usar_semihosting = 0;

/*
 * Semihosting: a instrução "bkpt 0xAB" para a CPU; o OpenOCD percebe que é
 * um pedido de semihosting, executa a operação no PC (aqui: imprimir uma
 * string) e retoma o programa. r0 = operação, r1 = argumento.
 *   0x04 = SYS_WRITE0 (string terminada em zero)
 * SEM debugger conectado, bkpt gera HardFault -> por isso só é usado quando
 * usar_semihosting = 1 (você liga isso pelo OpenOCD depois de habilitar
 * "arm semihosting enable").
 */
static void semi_puts(const char *s)
{
    if (!usar_semihosting) return;
    register uint32_t    r0 __asm("r0") = 0x04;
    register const char *r1 __asm("r1") = s;
    __asm volatile ("bkpt 0xAB" : "+r"(r0) : "r"(r1) : "memory");
}

static void led_init(void)
{
#if defined(STM32F0)
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
#else
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
#endif
    gpio_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT);
}

int main(void)
{
    system_init();
    uart_init(115200);
    led_init();

    kprintf("\n=== Modulo 23: OpenOCD ===\n");
    kprintf("periodo_ms       @ %p\n", (void *)&periodo_ms);
    kprintf("contador         @ %p\n", (void *)&contador);
    kprintf("comando          @ %p   (1 = LED, 2 = semihosting, 3 = reset)\n", (void *)&comando);
    kprintf("usar_semihosting @ %p\n", (void *)&usar_semihosting);
    kprintf("LED: ODR do GPIOA @ %p, bit %u\n", (void *)&LED_PORT->ODR, LED_PIN);

    uint32_t t = 0;
    for (;;) {
        if (millis() - t >= periodo_ms) {
            t = millis();
            gpio_toggle(LED_PORT, LED_PIN);
            contador++;
            if (contador % 10 == 0) semi_puts("[alvo] mais 10 piscadas\n");
        }

        switch (comando) {
        case 0: break;
        case 1: gpio_toggle(LED_PORT, LED_PIN); kprintf("comando 1: LED invertido\n"); break;
        case 2: semi_puts("[alvo] ola pelo semihosting!\n");
                kprintf("comando 2: mensagem semihosting %s\n", usar_semihosting ? "enviada" : "ignorada (usar_semihosting = 0)");
                break;
        case 3: kprintf("comando 3: reset\n"); delay_ms(10); NVIC_SystemReset(); break;
        default: kprintf("comando %u desconhecido\n", comando); break;
        }
        comando = 0;
        __WFI();
    }
}
