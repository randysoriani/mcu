/*
 * Módulo 05 - Hello World: pisca o LED LD2 (PA5) da Nucleo-F446RE.
 *
 * Única diferença real para o F0: o GPIO está no barramento AHB1, então o
 * bit de clock fica em RCC->AHB1ENR (bit 0) e não em RCC->AHBENR (bit 17).
 * Clock: padrão de reset (HSI 16 MHz).
 */
#include "device.h"

static void delay(volatile uint32_t n)
{
    while (n--) {
        __asm volatile ("nop");
    }
}

int main(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;    /* errata: aguardar 2 ciclos após habilitar o clock */

    GPIOA->MODER &= ~(3U << (5 * 2));
    GPIOA->MODER |=  (1U << (5 * 2));

    for (;;) {
        GPIOA->ODR ^= (1U << 5);   /* alterna (read-modify-write no ODR) */
        delay(400000);
    }
}
