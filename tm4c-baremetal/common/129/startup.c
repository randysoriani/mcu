/*
 * startup.c - inicialização do TM4C1294NCPDT (Cortex-M4F), em C.
 * Explicado em 02-startup/README.md.
 *
 * Handlers seguem o padrão <PERIFERICO>_Handler (ex.: TIMER0A_Handler,
 * UART0_Handler, GPIOF_Handler). Todos são "weak": defina uma função com o
 * mesmo nome no seu código para substituir o Default_Handler.
 */
#include <stdint.h>

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;
extern void (*__init_array_start[])(void);
extern void (*__init_array_end[])(void);

int main(void);
void Reset_Handler(void);

void Default_Handler(void)
{
    for (;;) { }
}

#define WEAK_DEFAULT __attribute__((weak, alias("Default_Handler")))

void NMI_Handler(void)        WEAK_DEFAULT;
void HardFault_Handler(void)  WEAK_DEFAULT;
void MemManage_Handler(void)  WEAK_DEFAULT;
void BusFault_Handler(void)   WEAK_DEFAULT;
void UsageFault_Handler(void) WEAK_DEFAULT;
void SVC_Handler(void)        WEAK_DEFAULT;
void DebugMon_Handler(void)   WEAK_DEFAULT;
void PendSV_Handler(void)     WEAK_DEFAULT;
void SysTick_Handler(void)    WEAK_DEFAULT;

void GPIOA_Handler(void) WEAK_DEFAULT;
void GPIOB_Handler(void) WEAK_DEFAULT;
void GPIOC_Handler(void) WEAK_DEFAULT;
void GPIOD_Handler(void) WEAK_DEFAULT;
void GPIOE_Handler(void) WEAK_DEFAULT;
void UART0_Handler(void) WEAK_DEFAULT;
void UART1_Handler(void) WEAK_DEFAULT;
void SSI0_Handler(void) WEAK_DEFAULT;
void I2C0_Handler(void) WEAK_DEFAULT;
void PWM0_FAULT_Handler(void) WEAK_DEFAULT;
void PWM0_0_Handler(void) WEAK_DEFAULT;
void PWM0_1_Handler(void) WEAK_DEFAULT;
void PWM0_2_Handler(void) WEAK_DEFAULT;
void QEI0_Handler(void) WEAK_DEFAULT;
void ADC0SS0_Handler(void) WEAK_DEFAULT;
void ADC0SS1_Handler(void) WEAK_DEFAULT;
void ADC0SS2_Handler(void) WEAK_DEFAULT;
void ADC0SS3_Handler(void) WEAK_DEFAULT;
void WATCHDOG_Handler(void) WEAK_DEFAULT;
void TIMER0A_Handler(void) WEAK_DEFAULT;
void TIMER0B_Handler(void) WEAK_DEFAULT;
void TIMER1A_Handler(void) WEAK_DEFAULT;
void TIMER1B_Handler(void) WEAK_DEFAULT;
void TIMER2A_Handler(void) WEAK_DEFAULT;
void TIMER2B_Handler(void) WEAK_DEFAULT;
void COMP0_Handler(void) WEAK_DEFAULT;
void COMP1_Handler(void) WEAK_DEFAULT;
void COMP2_Handler(void) WEAK_DEFAULT;
void SYSCTL_Handler(void) WEAK_DEFAULT;
void FLASH_Handler(void) WEAK_DEFAULT;
void GPIOF_Handler(void) WEAK_DEFAULT;
void GPIOG_Handler(void) WEAK_DEFAULT;
void GPIOH_Handler(void) WEAK_DEFAULT;
void UART2_Handler(void) WEAK_DEFAULT;
void SSI1_Handler(void) WEAK_DEFAULT;
void TIMER3A_Handler(void) WEAK_DEFAULT;
void TIMER3B_Handler(void) WEAK_DEFAULT;
void I2C1_Handler(void) WEAK_DEFAULT;
void CAN0_Handler(void) WEAK_DEFAULT;
void CAN1_Handler(void) WEAK_DEFAULT;
void ETH_Handler(void) WEAK_DEFAULT;
void HIBERNATE_Handler(void) WEAK_DEFAULT;
void USB0_Handler(void) WEAK_DEFAULT;
void PWM0_3_Handler(void) WEAK_DEFAULT;
void UDMA_Handler(void) WEAK_DEFAULT;
void UDMAERR_Handler(void) WEAK_DEFAULT;
void ADC1SS0_Handler(void) WEAK_DEFAULT;
void ADC1SS1_Handler(void) WEAK_DEFAULT;
void ADC1SS2_Handler(void) WEAK_DEFAULT;
void ADC1SS3_Handler(void) WEAK_DEFAULT;
void EPI0_Handler(void) WEAK_DEFAULT;
void GPIOJ_Handler(void) WEAK_DEFAULT;
void GPIOK_Handler(void) WEAK_DEFAULT;
void GPIOL_Handler(void) WEAK_DEFAULT;
void SSI2_Handler(void) WEAK_DEFAULT;
void SSI3_Handler(void) WEAK_DEFAULT;
void UART3_Handler(void) WEAK_DEFAULT;
void UART4_Handler(void) WEAK_DEFAULT;
void UART5_Handler(void) WEAK_DEFAULT;
void UART6_Handler(void) WEAK_DEFAULT;
void UART7_Handler(void) WEAK_DEFAULT;
void I2C2_Handler(void) WEAK_DEFAULT;
void I2C3_Handler(void) WEAK_DEFAULT;
void TIMER4A_Handler(void) WEAK_DEFAULT;
void TIMER4B_Handler(void) WEAK_DEFAULT;
void TIMER5A_Handler(void) WEAK_DEFAULT;
void TIMER5B_Handler(void) WEAK_DEFAULT;
void FPU_Handler(void) WEAK_DEFAULT;
void I2C4_Handler(void) WEAK_DEFAULT;
void I2C5_Handler(void) WEAK_DEFAULT;
void GPIOM_Handler(void) WEAK_DEFAULT;
void GPION_Handler(void) WEAK_DEFAULT;
void TAMPER_Handler(void) WEAK_DEFAULT;
void GPIOP0_Handler(void) WEAK_DEFAULT;
void GPIOP1_Handler(void) WEAK_DEFAULT;
void GPIOP2_Handler(void) WEAK_DEFAULT;
void GPIOP3_Handler(void) WEAK_DEFAULT;
void GPIOP4_Handler(void) WEAK_DEFAULT;
void GPIOP5_Handler(void) WEAK_DEFAULT;
void GPIOP6_Handler(void) WEAK_DEFAULT;
void GPIOP7_Handler(void) WEAK_DEFAULT;
void GPIOQ0_Handler(void) WEAK_DEFAULT;
void GPIOQ1_Handler(void) WEAK_DEFAULT;
void GPIOQ2_Handler(void) WEAK_DEFAULT;
void GPIOQ3_Handler(void) WEAK_DEFAULT;
void GPIOQ4_Handler(void) WEAK_DEFAULT;
void GPIOQ5_Handler(void) WEAK_DEFAULT;
void GPIOQ6_Handler(void) WEAK_DEFAULT;
void GPIOQ7_Handler(void) WEAK_DEFAULT;
void TIMER6A_Handler(void) WEAK_DEFAULT;
void TIMER6B_Handler(void) WEAK_DEFAULT;
void TIMER7A_Handler(void) WEAK_DEFAULT;
void TIMER7B_Handler(void) WEAK_DEFAULT;
void I2C6_Handler(void) WEAK_DEFAULT;
void I2C7_Handler(void) WEAK_DEFAULT;
void I2C8_Handler(void) WEAK_DEFAULT;
void I2C9_Handler(void) WEAK_DEFAULT;

typedef void (*vector_t)(void);

__attribute__((section(".isr_vector"), used))
const vector_t vector_table[16 + 114] = {
    (vector_t)&_estack,       /* 0x00 SP inicial */
    Reset_Handler,            /* 0x04 */
    NMI_Handler,              /* 0x08 */
    HardFault_Handler,        /* 0x0C */
    MemManage_Handler,        /* 0x10 */
    BusFault_Handler,         /* 0x14 */
    UsageFault_Handler,       /* 0x18 */
    0, 0, 0, 0,               /* 0x1C-0x28 reservados */
    SVC_Handler,              /* 0x2C */
    DebugMon_Handler,         /* 0x30 */
    0,                        /* 0x34 reservado */
    PendSV_Handler,           /* 0x38 */
    SysTick_Handler,          /* 0x3C */
    GPIOA_Handler,            /* IRQ 0 */
    GPIOB_Handler,            /* IRQ 1 */
    GPIOC_Handler,            /* IRQ 2 */
    GPIOD_Handler,            /* IRQ 3 */
    GPIOE_Handler,            /* IRQ 4 */
    UART0_Handler,            /* IRQ 5 */
    UART1_Handler,            /* IRQ 6 */
    SSI0_Handler,             /* IRQ 7 */
    I2C0_Handler,             /* IRQ 8 */
    PWM0_FAULT_Handler,       /* IRQ 9 */
    PWM0_0_Handler,           /* IRQ 10 */
    PWM0_1_Handler,           /* IRQ 11 */
    PWM0_2_Handler,           /* IRQ 12 */
    QEI0_Handler,             /* IRQ 13 */
    ADC0SS0_Handler,          /* IRQ 14 */
    ADC0SS1_Handler,          /* IRQ 15 */
    ADC0SS2_Handler,          /* IRQ 16 */
    ADC0SS3_Handler,          /* IRQ 17 */
    WATCHDOG_Handler,         /* IRQ 18 */
    TIMER0A_Handler,          /* IRQ 19 */
    TIMER0B_Handler,          /* IRQ 20 */
    TIMER1A_Handler,          /* IRQ 21 */
    TIMER1B_Handler,          /* IRQ 22 */
    TIMER2A_Handler,          /* IRQ 23 */
    TIMER2B_Handler,          /* IRQ 24 */
    COMP0_Handler,            /* IRQ 25 */
    COMP1_Handler,            /* IRQ 26 */
    COMP2_Handler,            /* IRQ 27 */
    SYSCTL_Handler,           /* IRQ 28 */
    FLASH_Handler,            /* IRQ 29 */
    GPIOF_Handler,            /* IRQ 30 */
    GPIOG_Handler,            /* IRQ 31 */
    GPIOH_Handler,            /* IRQ 32 */
    UART2_Handler,            /* IRQ 33 */
    SSI1_Handler,             /* IRQ 34 */
    TIMER3A_Handler,          /* IRQ 35 */
    TIMER3B_Handler,          /* IRQ 36 */
    I2C1_Handler,             /* IRQ 37 */
    CAN0_Handler,             /* IRQ 38 */
    CAN1_Handler,             /* IRQ 39 */
    ETH_Handler,              /* IRQ 40 */
    HIBERNATE_Handler,        /* IRQ 41 */
    USB0_Handler,             /* IRQ 42 */
    PWM0_3_Handler,           /* IRQ 43 */
    UDMA_Handler,             /* IRQ 44 */
    UDMAERR_Handler,          /* IRQ 45 */
    ADC1SS0_Handler,          /* IRQ 46 */
    ADC1SS1_Handler,          /* IRQ 47 */
    ADC1SS2_Handler,          /* IRQ 48 */
    ADC1SS3_Handler,          /* IRQ 49 */
    EPI0_Handler,             /* IRQ 50 */
    GPIOJ_Handler,            /* IRQ 51 */
    GPIOK_Handler,            /* IRQ 52 */
    GPIOL_Handler,            /* IRQ 53 */
    SSI2_Handler,             /* IRQ 54 */
    SSI3_Handler,             /* IRQ 55 */
    UART3_Handler,            /* IRQ 56 */
    UART4_Handler,            /* IRQ 57 */
    UART5_Handler,            /* IRQ 58 */
    UART6_Handler,            /* IRQ 59 */
    UART7_Handler,            /* IRQ 60 */
    I2C2_Handler,             /* IRQ 61 */
    I2C3_Handler,             /* IRQ 62 */
    TIMER4A_Handler,          /* IRQ 63 */
    TIMER4B_Handler,          /* IRQ 64 */
    TIMER5A_Handler,          /* IRQ 65 */
    TIMER5B_Handler,          /* IRQ 66 */
    FPU_Handler,              /* IRQ 67 */
    0,                        /* IRQ 68 reservado */
    0,                        /* IRQ 69 reservado */
    I2C4_Handler,             /* IRQ 70 */
    I2C5_Handler,             /* IRQ 71 */
    GPIOM_Handler,            /* IRQ 72 */
    GPION_Handler,            /* IRQ 73 */
    0,                        /* IRQ 74 reservado */
    TAMPER_Handler,           /* IRQ 75 */
    GPIOP0_Handler,           /* IRQ 76 */
    GPIOP1_Handler,           /* IRQ 77 */
    GPIOP2_Handler,           /* IRQ 78 */
    GPIOP3_Handler,           /* IRQ 79 */
    GPIOP4_Handler,           /* IRQ 80 */
    GPIOP5_Handler,           /* IRQ 81 */
    GPIOP6_Handler,           /* IRQ 82 */
    GPIOP7_Handler,           /* IRQ 83 */
    GPIOQ0_Handler,           /* IRQ 84 */
    GPIOQ1_Handler,           /* IRQ 85 */
    GPIOQ2_Handler,           /* IRQ 86 */
    GPIOQ3_Handler,           /* IRQ 87 */
    GPIOQ4_Handler,           /* IRQ 88 */
    GPIOQ5_Handler,           /* IRQ 89 */
    GPIOQ6_Handler,           /* IRQ 90 */
    GPIOQ7_Handler,           /* IRQ 91 */
    0,                        /* IRQ 92 reservado */
    0,                        /* IRQ 93 reservado */
    0,                        /* IRQ 94 reservado */
    0,                        /* IRQ 95 reservado */
    0,                        /* IRQ 96 reservado */
    0,                        /* IRQ 97 reservado */
    TIMER6A_Handler,          /* IRQ 98 */
    TIMER6B_Handler,          /* IRQ 99 */
    TIMER7A_Handler,          /* IRQ 100 */
    TIMER7B_Handler,          /* IRQ 101 */
    I2C6_Handler,             /* IRQ 102 */
    I2C7_Handler,             /* IRQ 103 */
    0,                        /* IRQ 104 reservado */
    0,                        /* IRQ 105 reservado */
    0,                        /* IRQ 106 reservado */
    0,                        /* IRQ 107 reservado */
    0,                        /* IRQ 108 reservado */
    I2C8_Handler,             /* IRQ 109 */
    I2C9_Handler,             /* IRQ 110 */
    0,                        /* IRQ 111 reservado */
    0,                        /* IRQ 112 reservado */
    0,                        /* IRQ 113 reservado */
};

#define SCB_CPACR (*(volatile uint32_t *)0xE000ED88UL)

/* Impede o GCC de trocar os laços por memcpy/memset (startup sem libc) */
__attribute__((optimize("no-tree-loop-distribute-patterns")))
void Reset_Handler(void)
{
    /* 0) FPU: acesso total a CP10/CP11 antes de qualquer código float */
    SCB_CPACR |= (0xFU << 20);
    __asm volatile ("dsb\n\tisb" ::: "memory");

    /* 1) .data: FLASH -> RAM */
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    /* 2) .bss = 0 */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    /* 3) construtores */
    for (void (**f)(void) = __init_array_start; f < __init_array_end; f++) {
        (*f)();
    }

    main();
    for (;;) { }
}
