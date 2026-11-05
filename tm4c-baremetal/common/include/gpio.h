/*
 * gpio.h - helpers mínimos para o GPIO da TI (inline, sem esconder registradores).
 * O módulo 05 mostra o acesso cru; os demais usam estas funções.
 */
#ifndef GPIO_H
#define GPIO_H
#include "device.h"

/* Liga o clock da porta (0 = A, 1 = B, ...) e espera ela ficar pronta */
static inline void gpio_port_enable(uint32_t port)
{
    periph_enable(&SYSCTL_RCGCGPIO, &SYSCTL_PRGPIO, port);
}

/* Destrava pinos protegidos (PF0 e PD7 no 123; PD7 e PE7 no 129) */
static inline void gpio_unlock(GPIO_TypeDef *p, uint32_t pin)
{
    p->LOCK = GPIO_LOCK_KEY;
    p->CR  |= (1U << pin);
    p->LOCK = 0;
}

static inline void gpio_output(GPIO_TypeDef *p, uint32_t pin)
{
    p->AFSEL &= ~(1U << pin);
    p->AMSEL &= ~(1U << pin);
    p->DIR   |=  (1U << pin);
    p->DEN   |=  (1U << pin);       /* sem DEN o pino NÃO funciona digitalmente */
}

enum { GPIO_NOPULL = 0, GPIO_PULLUP = 1, GPIO_PULLDOWN = 2 };

static inline void gpio_input(GPIO_TypeDef *p, uint32_t pin, uint32_t pull)
{
    p->AFSEL &= ~(1U << pin);
    p->AMSEL &= ~(1U << pin);
    p->DIR   &= ~(1U << pin);
    if (pull == GPIO_PULLUP)   { p->PDR &= ~(1U << pin); p->PUR |= (1U << pin); }
    else if (pull == GPIO_PULLDOWN) { p->PUR &= ~(1U << pin); p->PDR |= (1U << pin); }
    else { p->PUR &= ~(1U << pin); p->PDR &= ~(1U << pin); }
    p->DEN   |=  (1U << pin);
}

/* Função alternativa: AFSEL = 1 e PCTL (4 bits por pino) = número da função
 * (coluna da tabela "GPIO Pins and Alternate Functions" do datasheet) */
static inline void gpio_af(GPIO_TypeDef *p, uint32_t pin, uint32_t pctl)
{
    p->AMSEL &= ~(1U << pin);
    p->PCTL   = (p->PCTL & ~(0xFU << (pin * 4U))) | (pctl << (pin * 4U));
    p->AFSEL |=  (1U << pin);
    p->DEN   |=  (1U << pin);
}

static inline void gpio_analog(GPIO_TypeDef *p, uint32_t pin)
{
    p->DIR   &= ~(1U << pin);
    p->AFSEL |=  (1U << pin);
    p->DEN   &= ~(1U << pin);
    p->AMSEL |=  (1U << pin);
}

/* Acesso mascarado ao DATA: só o pino escolhido é afetado, sem RMW */
static inline void gpio_set(GPIO_TypeDef *p, uint32_t pin)    { p->DATA_BITS[1U << pin] = 0xFFU; }
static inline void gpio_clear(GPIO_TypeDef *p, uint32_t pin)  { p->DATA_BITS[1U << pin] = 0x00U; }
static inline void gpio_toggle(GPIO_TypeDef *p, uint32_t pin) { p->DATA_BITS[1U << pin] ^= 0xFFU; }
static inline uint32_t gpio_read(GPIO_TypeDef *p, uint32_t pin)
{
    return p->DATA_BITS[1U << pin] ? 1U : 0U;
}

#endif
