/* Blink mínimo para o exemplo de Makefile do módulo 00 (Nucleo-F446RE) */
#include "device.h"

int main(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;           /* clock do GPIOA */
    GPIOA->MODER  = (GPIOA->MODER & ~(3U << 10)) | (1U << 10);   /* PA5 saída */
    for (;;) {
        GPIOA->ODR ^= (1U << 5);
        for (volatile uint32_t i = 0; i < 500000; i++) { }
    }
}
