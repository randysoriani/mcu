/*
 * startup.c - inicialização do TM4C123GH6PM (Cortex-M4F), em C.
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
void SYSCTL_Handler(void) WEAK_DEFAULT;
void FLASH_Handler(void) WEAK_DEFAULT;
void GPIOF_Handler(void) WEAK_DEFAULT;
void UART2_Handler(void) WEAK_DEFAULT;
void SSI1_Handler(void) WEAK_DEFAULT;
void TIMER3A_Handler(void) WEAK_DEFAULT;
void TIMER3B_Handler(void) WEAK_DEFAULT;
void I2C1_Handler(void) WEAK_DEFAULT;
void QEI1_Handler(void) WEAK_DEFAULT;
void CAN0_Handler(void) WEAK_DEFAULT;
void CAN1_Handler(void) WEAK_DEFAULT;
void HIBERNATE_Handler(void) WEAK_DEFAULT;
void USB0_Handler(void) WEAK_DEFAULT;
void PWM0_3_Handler(void) WEAK_DEFAULT;
void UDMA_Handler(void) WEAK_DEFAULT;
void UDMAERR_Handler(void) WEAK_DEFAULT;
void ADC1SS0_Handler(void) WEAK_DEFAULT;
void ADC1SS1_Handler(void) WEAK_DEFAULT;
void ADC1SS2_Handler(void) WEAK_DEFAULT;
void ADC1SS3_Handler(void) WEAK_DEFAULT;
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
void WTIMER0A_Handler(void) WEAK_DEFAULT;
void WTIMER0B_Handler(void) WEAK_DEFAULT;
void WTIMER1A_Handler(void) WEAK_DEFAULT;
void WTIMER1B_Handler(void) WEAK_DEFAULT;
void WTIMER2A_Handler(void) WEAK_DEFAULT;
void WTIMER2B_Handler(void) WEAK_DEFAULT;
void WTIMER3A_Handler(void) WEAK_DEFAULT;
void WTIMER3B_Handler(void) WEAK_DEFAULT;
void WTIMER4A_Handler(void) WEAK_DEFAULT;
void WTIMER4B_Handler(void) WEAK_DEFAULT;
void WTIMER5A_Handler(void) WEAK_DEFAULT;
void WTIMER5B_Handler(void) WEAK_DEFAULT;
void FPU_Handler(void) WEAK_DEFAULT;
void PWM1_0_Handler(void) WEAK_DEFAULT;
void PWM1_1_Handler(void) WEAK_DEFAULT;
void PWM1_2_Handler(void) WEAK_DEFAULT;
void PWM1_3_Handler(void) WEAK_DEFAULT;
void PWM1_FAULT_Handler(void) WEAK_DEFAULT;

typedef void (*vector_t)(void);

__attribute__((section(".isr_vector"), used))
const vector_t vector_table[16 + 139] = {
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
    0,                        /* IRQ 27 reservado */
    SYSCTL_Handler,           /* IRQ 28 */
    FLASH_Handler,            /* IRQ 29 */
    GPIOF_Handler,            /* IRQ 30 */
    0,                        /* IRQ 31 reservado */
    0,                        /* IRQ 32 reservado */
    UART2_Handler,            /* IRQ 33 */
    SSI1_Handler,             /* IRQ 34 */
    TIMER3A_Handler,          /* IRQ 35 */
    TIMER3B_Handler,          /* IRQ 36 */
    I2C1_Handler,             /* IRQ 37 */
    QEI1_Handler,             /* IRQ 38 */
    CAN0_Handler,             /* IRQ 39 */
    CAN1_Handler,             /* IRQ 40 */
    0,                        /* IRQ 41 reservado */
    0,                        /* IRQ 42 reservado */
    HIBERNATE_Handler,        /* IRQ 43 */
    USB0_Handler,             /* IRQ 44 */
    PWM0_3_Handler,           /* IRQ 45 */
    UDMA_Handler,             /* IRQ 46 */
    UDMAERR_Handler,          /* IRQ 47 */
    ADC1SS0_Handler,          /* IRQ 48 */
    ADC1SS1_Handler,          /* IRQ 49 */
    ADC1SS2_Handler,          /* IRQ 50 */
    ADC1SS3_Handler,          /* IRQ 51 */
    0,                        /* IRQ 52 reservado */
    0,                        /* IRQ 53 reservado */
    0,                        /* IRQ 54 reservado */
    0,                        /* IRQ 55 reservado */
    0,                        /* IRQ 56 reservado */
    SSI2_Handler,             /* IRQ 57 */
    SSI3_Handler,             /* IRQ 58 */
    UART3_Handler,            /* IRQ 59 */
    UART4_Handler,            /* IRQ 60 */
    UART5_Handler,            /* IRQ 61 */
    UART6_Handler,            /* IRQ 62 */
    UART7_Handler,            /* IRQ 63 */
    0,                        /* IRQ 64 reservado */
    0,                        /* IRQ 65 reservado */
    0,                        /* IRQ 66 reservado */
    0,                        /* IRQ 67 reservado */
    I2C2_Handler,             /* IRQ 68 */
    I2C3_Handler,             /* IRQ 69 */
    TIMER4A_Handler,          /* IRQ 70 */
    TIMER4B_Handler,          /* IRQ 71 */
    0,                        /* IRQ 72 reservado */
    0,                        /* IRQ 73 reservado */
    0,                        /* IRQ 74 reservado */
    0,                        /* IRQ 75 reservado */
    0,                        /* IRQ 76 reservado */
    0,                        /* IRQ 77 reservado */
    0,                        /* IRQ 78 reservado */
    0,                        /* IRQ 79 reservado */
    0,                        /* IRQ 80 reservado */
    0,                        /* IRQ 81 reservado */
    0,                        /* IRQ 82 reservado */
    0,                        /* IRQ 83 reservado */
    0,                        /* IRQ 84 reservado */
    0,                        /* IRQ 85 reservado */
    0,                        /* IRQ 86 reservado */
    0,                        /* IRQ 87 reservado */
    0,                        /* IRQ 88 reservado */
    0,                        /* IRQ 89 reservado */
    0,                        /* IRQ 90 reservado */
    0,                        /* IRQ 91 reservado */
    TIMER5A_Handler,          /* IRQ 92 */
    TIMER5B_Handler,          /* IRQ 93 */
    WTIMER0A_Handler,         /* IRQ 94 */
    WTIMER0B_Handler,         /* IRQ 95 */
    WTIMER1A_Handler,         /* IRQ 96 */
    WTIMER1B_Handler,         /* IRQ 97 */
    WTIMER2A_Handler,         /* IRQ 98 */
    WTIMER2B_Handler,         /* IRQ 99 */
    WTIMER3A_Handler,         /* IRQ 100 */
    WTIMER3B_Handler,         /* IRQ 101 */
    WTIMER4A_Handler,         /* IRQ 102 */
    WTIMER4B_Handler,         /* IRQ 103 */
    WTIMER5A_Handler,         /* IRQ 104 */
    WTIMER5B_Handler,         /* IRQ 105 */
    FPU_Handler,              /* IRQ 106 */
    0,                        /* IRQ 107 reservado */
    0,                        /* IRQ 108 reservado */
    0,                        /* IRQ 109 reservado */
    0,                        /* IRQ 110 reservado */
    0,                        /* IRQ 111 reservado */
    0,                        /* IRQ 112 reservado */
    0,                        /* IRQ 113 reservado */
    0,                        /* IRQ 114 reservado */
    0,                        /* IRQ 115 reservado */
    0,                        /* IRQ 116 reservado */
    0,                        /* IRQ 117 reservado */
    0,                        /* IRQ 118 reservado */
    0,                        /* IRQ 119 reservado */
    0,                        /* IRQ 120 reservado */
    0,                        /* IRQ 121 reservado */
    0,                        /* IRQ 122 reservado */
    0,                        /* IRQ 123 reservado */
    0,                        /* IRQ 124 reservado */
    0,                        /* IRQ 125 reservado */
    0,                        /* IRQ 126 reservado */
    0,                        /* IRQ 127 reservado */
    0,                        /* IRQ 128 reservado */
    0,                        /* IRQ 129 reservado */
    0,                        /* IRQ 130 reservado */
    0,                        /* IRQ 131 reservado */
    0,                        /* IRQ 132 reservado */
    0,                        /* IRQ 133 reservado */
    PWM1_0_Handler,           /* IRQ 134 */
    PWM1_1_Handler,           /* IRQ 135 */
    PWM1_2_Handler,           /* IRQ 136 */
    PWM1_3_Handler,           /* IRQ 137 */
    PWM1_FAULT_Handler,       /* IRQ 138 */
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
