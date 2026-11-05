/*
 * tm4c129.h - TM4C1294NCPDT (EK-TM4C1294XL Connected LaunchPad)
 *   Cortex-M4F 120 MHz, 1 MB Flash em 0x00000000, 256 KB SRAM, 6 KB EEPROM,
 *   Ethernet MAC+PHY. Datasheet SPMS433B.
 */
#ifndef TM4C129_H
#define TM4C129_H

#include "tm4c_common.h"

#define FLASH_BASE   0x00000000UL
#define FLASH_SIZE   (1024UL * 1024UL)
#define SRAM_BASE    0x20000000UL
#define SRAM_SIZE    (256UL * 1024UL)
#define FLASH_ERASE_SIZE (16UL * 1024UL)   /* apagamento em setores de 16 KB */

/* ---- GPIO: no TM4C129 as portas ficam no AHB ---- */
#define GPIOA ((GPIO_TypeDef *) 0x40058000UL)
#define GPIOB ((GPIO_TypeDef *) 0x40059000UL)
#define GPIOC ((GPIO_TypeDef *) 0x4005A000UL)
#define GPIOD ((GPIO_TypeDef *) 0x4005B000UL)
#define GPIOE ((GPIO_TypeDef *) 0x4005C000UL)
#define GPIOF ((GPIO_TypeDef *) 0x4005D000UL)
#define GPIOG ((GPIO_TypeDef *) 0x4005E000UL)
#define GPIOH ((GPIO_TypeDef *) 0x4005F000UL)
#define GPIOJ ((GPIO_TypeDef *) 0x40060000UL)
#define GPIOK ((GPIO_TypeDef *) 0x40061000UL)
#define GPIOL ((GPIO_TypeDef *) 0x40062000UL)
#define GPIOM ((GPIO_TypeDef *) 0x40063000UL)
#define GPION ((GPIO_TypeDef *) 0x40064000UL)
#define GPIOP ((GPIO_TypeDef *) 0x40065000UL)
#define GPIOQ ((GPIO_TypeDef *) 0x40066000UL)
enum { PORTA = 0, PORTB, PORTC, PORTD, PORTE, PORTF, PORTG, PORTH,
       PORTJ, PORTK, PORTL, PORTM, PORTN, PORTP, PORTQ };

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
#define QEI0   ((QEI_TypeDef *)   0x4002C000UL)
#define TIMER0 ((TIMER_TypeDef *) 0x40030000UL)
#define TIMER1 ((TIMER_TypeDef *) 0x40031000UL)
#define TIMER2 ((TIMER_TypeDef *) 0x40032000UL)
#define TIMER3 ((TIMER_TypeDef *) 0x40033000UL)
#define TIMER4 ((TIMER_TypeDef *) 0x40034000UL)
#define TIMER5 ((TIMER_TypeDef *) 0x40035000UL)
#define ADC0   ((ADC_TypeDef *)   0x40038000UL)
#define ADC1   ((ADC_TypeDef *)   0x40039000UL)
#define CAN0   ((CAN_TypeDef *)   0x40040000UL)
#define CAN1   ((CAN_TypeDef *)   0x40041000UL)
#define HIB    ((HIB_TypeDef *)   0x400FC000UL)

/* ---- System control específico do 129 ---- */
#define SYSCTL_RSCLKCFG  REG32(SYSCTL_BASE + 0x0B0)
#define SYSCTL_MEMTIM0   REG32(SYSCTL_BASE + 0x0C0)
#define SYSCTL_ALTCLKCFG REG32(SYSCTL_BASE + 0x138)
#define SYSCTL_DIVSCLK   REG32(SYSCTL_BASE + 0x148)
#define SYSCTL_RIS_MOSCPUPRIS (1U << 8)
#define SYSCTL_MOSCCTL_NOXTAL (1U << 2)
#define SYSCTL_MOSCCTL_PWRDN  (1U << 3)
#define SYSCTL_MOSCCTL_OSCRNG (1U << 4)
#define SYSCTL_RSCLKCFG_PSYSDIV_Pos 0
#define SYSCTL_RSCLKCFG_OSCSRC_MOSC (3U << 20)
#define SYSCTL_RSCLKCFG_PLLSRC_MOSC (3U << 24)
#define SYSCTL_RSCLKCFG_USEPLL  (1U << 28)
#define SYSCTL_RSCLKCFG_NEWFREQ (1U << 30)
#define SYSCTL_RSCLKCFG_MEMTIMU (1U << 31)
#define SYSCTL_PLLFREQ0_PLLPWR  (1U << 23)
#define SYSCTL_PLLSTAT_LOCK     (1U << 0)

/* ---- Números de IRQ (datasheet, tabela "Interrupts") ---- */
enum {
    GPIOA_IRQn = 0, GPIOB_IRQn = 1, GPIOC_IRQn = 2, GPIOD_IRQn = 3, GPIOE_IRQn = 4,
    UART0_IRQn = 5, UART1_IRQn = 6, SSI0_IRQn = 7, I2C0_IRQn = 8,
    PWM0_FAULT_IRQn = 9, PWM0_0_IRQn = 10, PWM0_1_IRQn = 11, PWM0_2_IRQn = 12,
    QEI0_IRQn = 13, ADC0SS0_IRQn = 14, ADC0SS1_IRQn = 15, ADC0SS2_IRQn = 16, ADC0SS3_IRQn = 17,
    WATCHDOG_IRQn = 18, TIMER0A_IRQn = 19, TIMER0B_IRQn = 20, TIMER1A_IRQn = 21,
    TIMER1B_IRQn = 22, TIMER2A_IRQn = 23, TIMER2B_IRQn = 24,
    SYSCTL_IRQn = 28, FLASH_IRQn = 29, GPIOF_IRQn = 30, GPIOG_IRQn = 31, GPIOH_IRQn = 32,
    UART2_IRQn = 33, SSI1_IRQn = 34, TIMER3A_IRQn = 35, TIMER3B_IRQn = 36, I2C1_IRQn = 37,
    CAN0_IRQn = 38, CAN1_IRQn = 39, ETH_IRQn = 40, HIBERNATE_IRQn = 41, USB0_IRQn = 42,
    PWM0_3_IRQn = 43, UDMA_IRQn = 44, UDMAERR_IRQn = 45, ADC1SS0_IRQn = 46, ADC1SS3_IRQn = 49,
    GPIOJ_IRQn = 51, GPIOK_IRQn = 52, GPIOL_IRQn = 53, SSI2_IRQn = 54, SSI3_IRQn = 55,
    UART3_IRQn = 56, I2C2_IRQn = 61, I2C3_IRQn = 62, TIMER4A_IRQn = 63, TIMER5A_IRQn = 65,
    FPU_IRQn = 67, GPIOM_IRQn = 72, GPION_IRQn = 73, TIMER6A_IRQn = 98, TIMER7A_IRQn = 100,
};

#endif
