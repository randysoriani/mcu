/*
 * gpio.h - helpers mínimos para GPIO (inline, sem esconder os registradores).
 *
 * O bloco GPIO é praticamente idêntico no F0 e no F4, por isso estas funções
 * servem para os dois. Elas existem só para reduzir repetição nos módulos
 * mais avançados; o módulo 05 mostra o acesso "cru" aos registradores.
 */
#ifndef GPIO_H
#define GPIO_H

#include "device.h"

enum { GPIO_MODE_INPUT = 0, GPIO_MODE_OUTPUT = 1, GPIO_MODE_AF = 2, GPIO_MODE_ANALOG = 3 };
enum { GPIO_PUSHPULL = 0, GPIO_OPENDRAIN = 1 };
enum { GPIO_SPEED_LOW = 0, GPIO_SPEED_MEDIUM = 1, GPIO_SPEED_HIGH = 2, GPIO_SPEED_VERYHIGH = 3 };
enum { GPIO_NOPULL = 0, GPIO_PULLUP = 1, GPIO_PULLDOWN = 2 };

/* Cada pino ocupa 2 bits em MODER/OSPEEDR/PUPDR e 1 bit em OTYPER. */
static inline void gpio_mode(GPIO_TypeDef *p, uint32_t pin, uint32_t mode)
{
    p->MODER = (p->MODER & ~(3U << (pin * 2U))) | (mode << (pin * 2U));
}

static inline void gpio_otype(GPIO_TypeDef *p, uint32_t pin, uint32_t type)
{
    p->OTYPER = (p->OTYPER & ~(1U << pin)) | (type << pin);
}

static inline void gpio_speed(GPIO_TypeDef *p, uint32_t pin, uint32_t speed)
{
    p->OSPEEDR = (p->OSPEEDR & ~(3U << (pin * 2U))) | (speed << (pin * 2U));
}

static inline void gpio_pull(GPIO_TypeDef *p, uint32_t pin, uint32_t pull)
{
    p->PUPDR = (p->PUPDR & ~(3U << (pin * 2U))) | (pull << (pin * 2U));
}

/* Função alternativa: 4 bits por pino; AFR[0] = pinos 0..7, AFR[1] = 8..15 */
static inline void gpio_af(GPIO_TypeDef *p, uint32_t pin, uint32_t af)
{
    uint32_t idx = pin >> 3, sh = (pin & 7U) * 4U;
    p->AFR[idx] = (p->AFR[idx] & ~(0xFU << sh)) | (af << sh);
    gpio_mode(p, pin, GPIO_MODE_AF);
}

/* BSRR: bits 0..15 setam, bits 16..31 resetam. Escrita atômica, sem RMW. */
static inline void gpio_set(GPIO_TypeDef *p, uint32_t pin)    { p->BSRR = 1U << pin; }
static inline void gpio_clear(GPIO_TypeDef *p, uint32_t pin)  { p->BSRR = 1U << (pin + 16U); }
static inline void gpio_toggle(GPIO_TypeDef *p, uint32_t pin) { p->ODR ^= 1U << pin; }
static inline uint32_t gpio_read(GPIO_TypeDef *p, uint32_t pin) { return (p->IDR >> pin) & 1U; }

#endif
