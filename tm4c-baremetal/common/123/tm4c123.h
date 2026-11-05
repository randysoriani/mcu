/*
 * tm4c123.h - TM4C123GH6PM (EK-TM4C123GXL LaunchPad)
 *   Cortex-M4F 80 MHz, 256 KB Flash em 0x00000000, 32 KB SRAM, 2 KB EEPROM
 *   Datasheet SPMS376E.
 */
#ifndef TM4C123_H
#define TM4C123_H

#include "tm4c_common.h"

#define FLASH_BASE   0x00000000UL
#define FLASH_SIZE   (256UL * 1024UL)
#define SRAM_BASE    0x20000000UL
#define SRAM_SIZE    (32UL * 1024UL)
#define FLASH_ERASE_SIZE 1024UL       /* bloco mínimo de apagamento */

/* ---- GPIO: abertura APB (padrão após reset). Há também a abertura AHB
 *      (0x40058000...), habilitada por porta em SYSCTL_GPIOHBCTL. ---- */
#define GPIOA ((GPIO_TypeDef *) 0x40004000UL)
#define GPIOB ((GPIO_TypeDef *) 0x40005000UL)
#define GPIOC ((GPIO_TypeDef *) 0x40006000UL)
#define GPIOD ((GPIO_TypeDef *) 0x40007000UL)
#define GPIOE ((GPIO_TypeDef *) 0x40024000UL)
#define GPIOF ((GPIO_TypeDef *) 0x40025000UL)
enum { PORTA = 0, PORTB, PORTC, PORTD, PORTE, PORTF };

#define WDT0   ((WDT_TypeDef *)   0x40000000UL)
#define WDT1   ((WDT_TypeDef *)   0x40001000UL)
#define SSI0   ((SSI_TypeDef *)   0x40008000UL)
#define SSI1   ((SSI_TypeDef *)   0x40009000UL)
#define SSI2   ((SSI_TypeDef *)   0x4000A000UL)
#define SSI3   ((SSI_TypeDef *)   0x4000B000UL)
#define UART0  ((UART_TypeDef *)  0x4000C000UL)
#define UART1  ((UART_TypeDef *)  0x4000D000UL)
#define I2C0   ((I2C_TypeDef *)   0x40020000UL)
#define I2C1   ((I2C_TypeDef *)   0x40021000UL)
#define PWM0   ((PWM_TypeDef *)   0x40028000UL)
#define PWM1   ((PWM_TypeDef *)   0x40029000UL)
#define QEI0   ((QEI_TypeDef *)   0x4002C000UL)
#define QEI1   ((QEI_TypeDef *)   0x4002D000UL)
#define TIMER0 ((TIMER_TypeDef *) 0x40030000UL)
#define TIMER1 ((TIMER_TypeDef *) 0x40031000UL)
#define TIMER2 ((TIMER_TypeDef *) 0x40032000UL)
#define TIMER3 ((TIMER_TypeDef *) 0x40033000UL)
#define TIMER4 ((TIMER_TypeDef *) 0x40034000UL)
#define TIMER5 ((TIMER_TypeDef *) 0x40035000UL)
#define WTIMER0 ((TIMER_TypeDef *) 0x40036000UL)  /* "wide": 32/64 bits */
#define WTIMER1 ((TIMER_TypeDef *) 0x40037000UL)
#define ADC0   ((ADC_TypeDef *)   0x40038000UL)
#define ADC1   ((ADC_TypeDef *)   0x40039000UL)
#define CAN0   ((CAN_TypeDef *)   0x40040000UL)
#define CAN1   ((CAN_TypeDef *)   0x40041000UL)
#define HIB    ((HIB_TypeDef *)   0x400FC000UL)

/* ---- System control específico do 123 ---- */
#define SYSCTL_RCC       REG32(SYSCTL_BASE + 0x060)
#define SYSCTL_GPIOHBCTL REG32(SYSCTL_BASE + 0x06C)
#define SYSCTL_RCC2      REG32(SYSCTL_BASE + 0x070)
#define SYSCTL_RIS_PLLLRIS (1U << 6)
/* RCC */
#define SYSCTL_RCC_MOSCDIS    (1U << 0)
#define SYSCTL_RCC_OSCSRC_Msk (3U << 4)
#define SYSCTL_RCC_XTAL_Pos   6
#define SYSCTL_RCC_XTAL_Msk   (0x1FU << 6)
#define SYSCTL_RCC_XTAL_16MHZ (0x15U << 6)
#define SYSCTL_RCC_BYPASS     (1U << 11)
#define SYSCTL_RCC_PWRDN      (1U << 13)
#define SYSCTL_RCC_PWMDIV_Pos 17
#define SYSCTL_RCC_USEPWMDIV  (1U << 20)
#define SYSCTL_RCC_USESYSDIV  (1U << 22)
/* RCC2 */
#define SYSCTL_RCC2_OSCSRC2_Msk (7U << 4)
#define SYSCTL_RCC2_BYPASS2   (1U << 11)
#define SYSCTL_RCC2_PWRDN2    (1U << 13)
#define SYSCTL_RCC2_SYSDIV2_Msk (0x7FU << 22)   /* SYSDIV2 [28:23] + SYSDIV2LSB [22] */
#define SYSCTL_RCC2_SYSDIV2_Pos 22
#define SYSCTL_RCC2_DIV400    (1U << 30)
#define SYSCTL_RCC2_USERCC2   (1U << 31)

/* ---- Números de IRQ (datasheet, tabela "Interrupts") ---- */
enum {
    GPIOA_IRQn = 0, GPIOB_IRQn = 1, GPIOC_IRQn = 2, GPIOD_IRQn = 3, GPIOE_IRQn = 4,
    UART0_IRQn = 5, UART1_IRQn = 6, SSI0_IRQn = 7, I2C0_IRQn = 8,
    PWM0_FAULT_IRQn = 9, PWM0_0_IRQn = 10, PWM0_1_IRQn = 11, PWM0_2_IRQn = 12,
    QEI0_IRQn = 13, ADC0SS0_IRQn = 14, ADC0SS1_IRQn = 15, ADC0SS2_IRQn = 16, ADC0SS3_IRQn = 17,
    WATCHDOG_IRQn = 18, TIMER0A_IRQn = 19, TIMER0B_IRQn = 20, TIMER1A_IRQn = 21,
    TIMER1B_IRQn = 22, TIMER2A_IRQn = 23, TIMER2B_IRQn = 24, COMP0_IRQn = 25, COMP1_IRQn = 26,
    SYSCTL_IRQn = 28, FLASH_IRQn = 29, GPIOF_IRQn = 30, UART2_IRQn = 33, SSI1_IRQn = 34,
    TIMER3A_IRQn = 35, TIMER3B_IRQn = 36, I2C1_IRQn = 37, QEI1_IRQn = 38,
    CAN0_IRQn = 39, CAN1_IRQn = 40, HIBERNATE_IRQn = 43, USB0_IRQn = 44, PWM0_3_IRQn = 45,
    UDMA_IRQn = 46, UDMAERR_IRQn = 47, ADC1SS0_IRQn = 48, ADC1SS3_IRQn = 51,
    SSI2_IRQn = 57, SSI3_IRQn = 58, UART3_IRQn = 59, I2C2_IRQn = 68, I2C3_IRQn = 69,
    TIMER4A_IRQn = 70, TIMER5A_IRQn = 92, WTIMER0A_IRQn = 94, WTIMER0B_IRQn = 95,
    FPU_IRQn = 106, PWM1_0_IRQn = 134, PWM1_1_IRQn = 135, PWM1_2_IRQn = 136,
    PWM1_3_IRQn = 137, PWM1_FAULT_IRQn = 138,
};

#endif
