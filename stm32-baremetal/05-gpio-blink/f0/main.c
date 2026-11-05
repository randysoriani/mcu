/*
 * Módulo 05 - Hello World: pisca o LED LD2 (PA5) da Nucleo-F072RB.
 *
 * Programa 100% autocontido: só usa o header de registradores.
 * Clock: o padrão de reset (HSI 8 MHz), sem PLL.
 */
#include "device.h"

/* Atraso "burro" por contagem. O volatile impede o compilador de
 * remover o laço (ele não tem efeito observável para o otimizador). */
static void delay(volatile uint32_t n)
{
    while (n--) {
        __asm volatile ("nop");
    }
}

int main(void)
{
    /* 1) Liga o clock do GPIOA. Sem clock o periférico ignora escritas. */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    /* 2) PA5 como saída: MODER5[11:10] = 01 */
    GPIOA->MODER &= ~(3U << (5 * 2));
    GPIOA->MODER |=  (1U << (5 * 2));

    /* 3) (opcional, já são os valores de reset) push-pull, velocidade baixa,
     *    sem pull-up/down */
    GPIOA->OTYPER  &= ~(1U << 5);
    GPIOA->OSPEEDR &= ~(3U << (5 * 2));
    GPIOA->PUPDR   &= ~(3U << (5 * 2));

    for (;;) {
        GPIOA->BSRR = (1U << 5);          /* LED aceso  (set PA5)   */
        delay(200000);
        GPIOA->BSRR = (1U << (5 + 16));   /* LED apagado (reset PA5) */
        delay(200000);
    }
}
