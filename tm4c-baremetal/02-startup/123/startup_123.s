/* =============================================================================
 *  startup_123.s - Startup em ASSEMBLY para o TM4C123GH6PM (Cortex-M4F / ARMv7E-M)
 *  Mesmo papel de common/123/startup.c. Explicado em 02-startup/README.md.
 * ========================================================================== */
    .syntax unified
    .cpu    cortex-m4
    .fpu    fpv4-sp-d16
    .thumb

    .global vector_table
    .global Reset_Handler
    .global Default_Handler

    .equ    SCB_CPACR, 0xE000ED88

    .section .text.Reset_Handler, "ax", %progbits
    .type    Reset_Handler, %function
Reset_Handler:
    ldr   sp, =_estack            /* Thumb-2 permite carregar SP direto       */

    /* ---- FPU: CPACR[23:20] = 0b1111 ---- */
    ldr   r0, =SCB_CPACR
    ldr   r1, [r0]
    orr   r1, r1, #(0xF << 20)
    str   r1, [r0]
    dsb
    isb

    /* ---- copia .data (FLASH -> RAM) ---- */
    ldr   r0, =_sdata
    ldr   r1, =_edata
    ldr   r2, =_sidata
1:  cmp   r0, r1
    ittt  lo                      /* bloco IT: 3 instruções condicionais     */
    ldrlo r3, [r2], #4            /* pós-incremento                          */
    strlo r3, [r0], #4
    blo   1b

    /* ---- zera .bss ---- */
    ldr   r0, =_sbss
    ldr   r1, =_ebss
    movs  r2, #0
2:  cmp   r0, r1
    itt   lo
    strlo r2, [r0], #4
    blo   2b

    bl    main
3:  b     3b
    .size Reset_Handler, . - Reset_Handler

    .section .text.Default_Handler, "ax", %progbits
    .type    Default_Handler, %function
Default_Handler:
    b     Default_Handler
    .size Default_Handler, . - Default_Handler

    .section .isr_vector, "a", %progbits
    .type    vector_table, %object
vector_table:
    .word _estack                         /* 0x00 SP inicial      */
    .word Reset_Handler                   /* 0x04 Reset           */
    .word NMI_Handler                     /* 0x08                 */
    .word HardFault_Handler               /* 0x0C                 */
    .word MemManage_Handler               /* 0x10                 */
    .word BusFault_Handler                /* 0x14                 */
    .word UsageFault_Handler              /* 0x18                 */
    .word 0, 0, 0, 0                      /* 0x1C-0x28 reservados */
    .word SVC_Handler                     /* 0x2C                 */
    .word DebugMon_Handler                /* 0x30                 */
    .word 0                               /* 0x34 reservado       */
    .word PendSV_Handler                  /* 0x38                 */
    .word SysTick_Handler                 /* 0x3C                 */
    .word GPIOA_Handler               /* IRQ 0 */
    .word GPIOB_Handler               /* IRQ 1 */
    .word GPIOC_Handler               /* IRQ 2 */
    .word GPIOD_Handler               /* IRQ 3 */
    .word GPIOE_Handler               /* IRQ 4 */
    .word UART0_Handler               /* IRQ 5 */
    .word UART1_Handler               /* IRQ 6 */
    .word SSI0_Handler                /* IRQ 7 */
    .word I2C0_Handler                /* IRQ 8 */
    .word PWM0_FAULT_Handler          /* IRQ 9 */
    .word PWM0_0_Handler              /* IRQ 10 */
    .word PWM0_1_Handler              /* IRQ 11 */
    .word PWM0_2_Handler              /* IRQ 12 */
    .word QEI0_Handler                /* IRQ 13 */
    .word ADC0SS0_Handler             /* IRQ 14 */
    .word ADC0SS1_Handler             /* IRQ 15 */
    .word ADC0SS2_Handler             /* IRQ 16 */
    .word ADC0SS3_Handler             /* IRQ 17 */
    .word WATCHDOG_Handler            /* IRQ 18 */
    .word TIMER0A_Handler             /* IRQ 19 */
    .word TIMER0B_Handler             /* IRQ 20 */
    .word TIMER1A_Handler             /* IRQ 21 */
    .word TIMER1B_Handler             /* IRQ 22 */
    .word TIMER2A_Handler             /* IRQ 23 */
    .word TIMER2B_Handler             /* IRQ 24 */
    .word COMP0_Handler               /* IRQ 25 */
    .word COMP1_Handler               /* IRQ 26 */
    .word 0                           /* IRQ 27 */
    .word SYSCTL_Handler              /* IRQ 28 */
    .word FLASH_Handler               /* IRQ 29 */
    .word GPIOF_Handler               /* IRQ 30 */
    .word 0                           /* IRQ 31 */
    .word 0                           /* IRQ 32 */
    .word UART2_Handler               /* IRQ 33 */
    .word SSI1_Handler                /* IRQ 34 */
    .word TIMER3A_Handler             /* IRQ 35 */
    .word TIMER3B_Handler             /* IRQ 36 */
    .word I2C1_Handler                /* IRQ 37 */
    .word QEI1_Handler                /* IRQ 38 */
    .word CAN0_Handler                /* IRQ 39 */
    .word CAN1_Handler                /* IRQ 40 */
    .word 0                           /* IRQ 41 */
    .word 0                           /* IRQ 42 */
    .word HIBERNATE_Handler           /* IRQ 43 */
    .word USB0_Handler                /* IRQ 44 */
    .word PWM0_3_Handler              /* IRQ 45 */
    .word UDMA_Handler                /* IRQ 46 */
    .word UDMAERR_Handler             /* IRQ 47 */
    .word ADC1SS0_Handler             /* IRQ 48 */
    .word ADC1SS1_Handler             /* IRQ 49 */
    .word ADC1SS2_Handler             /* IRQ 50 */
    .word ADC1SS3_Handler             /* IRQ 51 */
    .word 0                           /* IRQ 52 */
    .word 0                           /* IRQ 53 */
    .word 0                           /* IRQ 54 */
    .word 0                           /* IRQ 55 */
    .word 0                           /* IRQ 56 */
    .word SSI2_Handler                /* IRQ 57 */
    .word SSI3_Handler                /* IRQ 58 */
    .word UART3_Handler               /* IRQ 59 */
    .word UART4_Handler               /* IRQ 60 */
    .word UART5_Handler               /* IRQ 61 */
    .word UART6_Handler               /* IRQ 62 */
    .word UART7_Handler               /* IRQ 63 */
    .word 0                           /* IRQ 64 */
    .word 0                           /* IRQ 65 */
    .word 0                           /* IRQ 66 */
    .word 0                           /* IRQ 67 */
    .word I2C2_Handler                /* IRQ 68 */
    .word I2C3_Handler                /* IRQ 69 */
    .word TIMER4A_Handler             /* IRQ 70 */
    .word TIMER4B_Handler             /* IRQ 71 */
    .word 0                           /* IRQ 72 */
    .word 0                           /* IRQ 73 */
    .word 0                           /* IRQ 74 */
    .word 0                           /* IRQ 75 */
    .word 0                           /* IRQ 76 */
    .word 0                           /* IRQ 77 */
    .word 0                           /* IRQ 78 */
    .word 0                           /* IRQ 79 */
    .word 0                           /* IRQ 80 */
    .word 0                           /* IRQ 81 */
    .word 0                           /* IRQ 82 */
    .word 0                           /* IRQ 83 */
    .word 0                           /* IRQ 84 */
    .word 0                           /* IRQ 85 */
    .word 0                           /* IRQ 86 */
    .word 0                           /* IRQ 87 */
    .word 0                           /* IRQ 88 */
    .word 0                           /* IRQ 89 */
    .word 0                           /* IRQ 90 */
    .word 0                           /* IRQ 91 */
    .word TIMER5A_Handler             /* IRQ 92 */
    .word TIMER5B_Handler             /* IRQ 93 */
    .word WTIMER0A_Handler            /* IRQ 94 */
    .word WTIMER0B_Handler            /* IRQ 95 */
    .word WTIMER1A_Handler            /* IRQ 96 */
    .word WTIMER1B_Handler            /* IRQ 97 */
    .word WTIMER2A_Handler            /* IRQ 98 */
    .word WTIMER2B_Handler            /* IRQ 99 */
    .word WTIMER3A_Handler            /* IRQ 100 */
    .word WTIMER3B_Handler            /* IRQ 101 */
    .word WTIMER4A_Handler            /* IRQ 102 */
    .word WTIMER4B_Handler            /* IRQ 103 */
    .word WTIMER5A_Handler            /* IRQ 104 */
    .word WTIMER5B_Handler            /* IRQ 105 */
    .word FPU_Handler                 /* IRQ 106 */
    .word 0                           /* IRQ 107 */
    .word 0                           /* IRQ 108 */
    .word 0                           /* IRQ 109 */
    .word 0                           /* IRQ 110 */
    .word 0                           /* IRQ 111 */
    .word 0                           /* IRQ 112 */
    .word 0                           /* IRQ 113 */
    .word 0                           /* IRQ 114 */
    .word 0                           /* IRQ 115 */
    .word 0                           /* IRQ 116 */
    .word 0                           /* IRQ 117 */
    .word 0                           /* IRQ 118 */
    .word 0                           /* IRQ 119 */
    .word 0                           /* IRQ 120 */
    .word 0                           /* IRQ 121 */
    .word 0                           /* IRQ 122 */
    .word 0                           /* IRQ 123 */
    .word 0                           /* IRQ 124 */
    .word 0                           /* IRQ 125 */
    .word 0                           /* IRQ 126 */
    .word 0                           /* IRQ 127 */
    .word 0                           /* IRQ 128 */
    .word 0                           /* IRQ 129 */
    .word 0                           /* IRQ 130 */
    .word 0                           /* IRQ 131 */
    .word 0                           /* IRQ 132 */
    .word 0                           /* IRQ 133 */
    .word PWM1_0_Handler              /* IRQ 134 */
    .word PWM1_1_Handler              /* IRQ 135 */
    .word PWM1_2_Handler              /* IRQ 136 */
    .word PWM1_3_Handler              /* IRQ 137 */
    .word PWM1_FAULT_Handler          /* IRQ 138 */
    .size vector_table, . - vector_table

    .macro WEAK_IRQ name
    .weak      \name
    .thumb_set \name, Default_Handler
    .endm

    WEAK_IRQ NMI_Handler
    WEAK_IRQ HardFault_Handler
    WEAK_IRQ MemManage_Handler
    WEAK_IRQ BusFault_Handler
    WEAK_IRQ UsageFault_Handler
    WEAK_IRQ SVC_Handler
    WEAK_IRQ DebugMon_Handler
    WEAK_IRQ PendSV_Handler
    WEAK_IRQ SysTick_Handler
    WEAK_IRQ GPIOA_Handler
    WEAK_IRQ GPIOB_Handler
    WEAK_IRQ GPIOC_Handler
    WEAK_IRQ GPIOD_Handler
    WEAK_IRQ GPIOE_Handler
    WEAK_IRQ UART0_Handler
    WEAK_IRQ UART1_Handler
    WEAK_IRQ SSI0_Handler
    WEAK_IRQ I2C0_Handler
    WEAK_IRQ PWM0_FAULT_Handler
    WEAK_IRQ PWM0_0_Handler
    WEAK_IRQ PWM0_1_Handler
    WEAK_IRQ PWM0_2_Handler
    WEAK_IRQ QEI0_Handler
    WEAK_IRQ ADC0SS0_Handler
    WEAK_IRQ ADC0SS1_Handler
    WEAK_IRQ ADC0SS2_Handler
    WEAK_IRQ ADC0SS3_Handler
    WEAK_IRQ WATCHDOG_Handler
    WEAK_IRQ TIMER0A_Handler
    WEAK_IRQ TIMER0B_Handler
    WEAK_IRQ TIMER1A_Handler
    WEAK_IRQ TIMER1B_Handler
    WEAK_IRQ TIMER2A_Handler
    WEAK_IRQ TIMER2B_Handler
    WEAK_IRQ COMP0_Handler
    WEAK_IRQ COMP1_Handler
    WEAK_IRQ SYSCTL_Handler
    WEAK_IRQ FLASH_Handler
    WEAK_IRQ GPIOF_Handler
    WEAK_IRQ UART2_Handler
    WEAK_IRQ SSI1_Handler
    WEAK_IRQ TIMER3A_Handler
    WEAK_IRQ TIMER3B_Handler
    WEAK_IRQ I2C1_Handler
    WEAK_IRQ QEI1_Handler
    WEAK_IRQ CAN0_Handler
    WEAK_IRQ CAN1_Handler
    WEAK_IRQ HIBERNATE_Handler
    WEAK_IRQ USB0_Handler
    WEAK_IRQ PWM0_3_Handler
    WEAK_IRQ UDMA_Handler
    WEAK_IRQ UDMAERR_Handler
    WEAK_IRQ ADC1SS0_Handler
    WEAK_IRQ ADC1SS1_Handler
    WEAK_IRQ ADC1SS2_Handler
    WEAK_IRQ ADC1SS3_Handler
    WEAK_IRQ SSI2_Handler
    WEAK_IRQ SSI3_Handler
    WEAK_IRQ UART3_Handler
    WEAK_IRQ UART4_Handler
    WEAK_IRQ UART5_Handler
    WEAK_IRQ UART6_Handler
    WEAK_IRQ UART7_Handler
    WEAK_IRQ I2C2_Handler
    WEAK_IRQ I2C3_Handler
    WEAK_IRQ TIMER4A_Handler
    WEAK_IRQ TIMER4B_Handler
    WEAK_IRQ TIMER5A_Handler
    WEAK_IRQ TIMER5B_Handler
    WEAK_IRQ WTIMER0A_Handler
    WEAK_IRQ WTIMER0B_Handler
    WEAK_IRQ WTIMER1A_Handler
    WEAK_IRQ WTIMER1B_Handler
    WEAK_IRQ WTIMER2A_Handler
    WEAK_IRQ WTIMER2B_Handler
    WEAK_IRQ WTIMER3A_Handler
    WEAK_IRQ WTIMER3B_Handler
    WEAK_IRQ WTIMER4A_Handler
    WEAK_IRQ WTIMER4B_Handler
    WEAK_IRQ WTIMER5A_Handler
    WEAK_IRQ WTIMER5B_Handler
    WEAK_IRQ FPU_Handler
    WEAK_IRQ PWM1_0_Handler
    WEAK_IRQ PWM1_1_Handler
    WEAK_IRQ PWM1_2_Handler
    WEAK_IRQ PWM1_3_Handler
    WEAK_IRQ PWM1_FAULT_Handler
