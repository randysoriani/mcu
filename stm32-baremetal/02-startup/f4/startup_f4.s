/* =============================================================================
 *  startup_f4.s - Startup em ASSEMBLY para STM32F446xx (Cortex-M4F / ARMv7E-M)
 *
 *  Mesmo papel de common/f4/startup.c. Diferenças para o do F0:
 *    - ARMv7-M tem Thumb-2 completo: LDR com pós-incremento, CBZ, IT...
 *    - habilita a FPU (CPACR) antes de qualquer coisa
 *    - tabela de vetores com MemManage/BusFault/UsageFault/DebugMon + 97 IRQs
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

    /* ---- FPU: CPACR[23:20] = 0b1111 (CP10 e CP11 com acesso total) ---- */
    ldr   r0, =SCB_CPACR
    ldr   r1, [r0]
    orr   r1, r1, #(0xF << 20)
    str   r1, [r0]
    dsb                           /* garante que a escrita terminou          */
    isb                           /* descarta o pipeline                     */

    /* ---- copia .data ---- */
    ldr   r0, =_sdata
    ldr   r1, =_edata
    ldr   r2, =_sidata
1:  cmp   r0, r1
    ittt  lo                      /* bloco IT: próximas 3 instr. só se r0<r1 */
    ldrlo r3, [r2], #4            /* r3 = *r2; r2 += 4  (pós-incremento)     */
    strlo r3, [r0], #4            /* *r0 = r3; r0 += 4                       */
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
    .word WWDG_IRQHandler                 /* IRQ 0  */
    .word PVD_IRQHandler                  /* IRQ 1  */
    .word TAMP_STAMP_IRQHandler           /* IRQ 2  */
    .word RTC_WKUP_IRQHandler             /* IRQ 3  */
    .word FLASH_IRQHandler                /* IRQ 4  */
    .word RCC_IRQHandler                  /* IRQ 5  */
    .word EXTI0_IRQHandler                /* IRQ 6  */
    .word EXTI1_IRQHandler                /* IRQ 7  */
    .word EXTI2_IRQHandler                /* IRQ 8  */
    .word EXTI3_IRQHandler                /* IRQ 9  */
    .word EXTI4_IRQHandler                /* IRQ 10 */
    .word DMA1_Stream0_IRQHandler         /* IRQ 11 */
    .word DMA1_Stream1_IRQHandler         /* IRQ 12 */
    .word DMA1_Stream2_IRQHandler         /* IRQ 13 */
    .word DMA1_Stream3_IRQHandler         /* IRQ 14 */
    .word DMA1_Stream4_IRQHandler         /* IRQ 15 */
    .word DMA1_Stream5_IRQHandler         /* IRQ 16 */
    .word DMA1_Stream6_IRQHandler         /* IRQ 17 */
    .word ADC_IRQHandler                  /* IRQ 18 */
    .word CAN1_TX_IRQHandler              /* IRQ 19 */
    .word CAN1_RX0_IRQHandler             /* IRQ 20 */
    .word CAN1_RX1_IRQHandler             /* IRQ 21 */
    .word CAN1_SCE_IRQHandler             /* IRQ 22 */
    .word EXTI9_5_IRQHandler              /* IRQ 23 */
    .word TIM1_BRK_TIM9_IRQHandler        /* IRQ 24 */
    .word TIM1_UP_TIM10_IRQHandler        /* IRQ 25 */
    .word TIM1_TRG_COM_TIM11_IRQHandler   /* IRQ 26 */
    .word TIM1_CC_IRQHandler              /* IRQ 27 */
    .word TIM2_IRQHandler                 /* IRQ 28 */
    .word TIM3_IRQHandler                 /* IRQ 29 */
    .word TIM4_IRQHandler                 /* IRQ 30 */
    .word I2C1_EV_IRQHandler              /* IRQ 31 */
    .word I2C1_ER_IRQHandler              /* IRQ 32 */
    .word I2C2_EV_IRQHandler              /* IRQ 33 */
    .word I2C2_ER_IRQHandler              /* IRQ 34 */
    .word SPI1_IRQHandler                 /* IRQ 35 */
    .word SPI2_IRQHandler                 /* IRQ 36 */
    .word USART1_IRQHandler               /* IRQ 37 */
    .word USART2_IRQHandler               /* IRQ 38 */
    .word USART3_IRQHandler               /* IRQ 39 */
    .word EXTI15_10_IRQHandler            /* IRQ 40 */
    .word RTC_Alarm_IRQHandler            /* IRQ 41 */
    .word OTG_FS_WKUP_IRQHandler          /* IRQ 42 */
    .word TIM8_BRK_TIM12_IRQHandler       /* IRQ 43 */
    .word TIM8_UP_TIM13_IRQHandler        /* IRQ 44 */
    .word TIM8_TRG_COM_TIM14_IRQHandler   /* IRQ 45 */
    .word TIM8_CC_IRQHandler              /* IRQ 46 */
    .word DMA1_Stream7_IRQHandler         /* IRQ 47 */
    .word FMC_IRQHandler                  /* IRQ 48 */
    .word SDIO_IRQHandler                 /* IRQ 49 */
    .word TIM5_IRQHandler                 /* IRQ 50 */
    .word SPI3_IRQHandler                 /* IRQ 51 */
    .word UART4_IRQHandler                /* IRQ 52 */
    .word UART5_IRQHandler                /* IRQ 53 */
    .word TIM6_DAC_IRQHandler             /* IRQ 54 */
    .word TIM7_IRQHandler                 /* IRQ 55 */
    .word DMA2_Stream0_IRQHandler         /* IRQ 56 */
    .word DMA2_Stream1_IRQHandler         /* IRQ 57 */
    .word DMA2_Stream2_IRQHandler         /* IRQ 58 */
    .word DMA2_Stream3_IRQHandler         /* IRQ 59 */
    .word DMA2_Stream4_IRQHandler         /* IRQ 60 */
    .word 0                               /* IRQ 61 reservado */
    .word 0                               /* IRQ 62 reservado */
    .word CAN2_TX_IRQHandler              /* IRQ 63 */
    .word CAN2_RX0_IRQHandler             /* IRQ 64 */
    .word CAN2_RX1_IRQHandler             /* IRQ 65 */
    .word CAN2_SCE_IRQHandler             /* IRQ 66 */
    .word OTG_FS_IRQHandler               /* IRQ 67 */
    .word DMA2_Stream5_IRQHandler         /* IRQ 68 */
    .word DMA2_Stream6_IRQHandler         /* IRQ 69 */
    .word DMA2_Stream7_IRQHandler         /* IRQ 70 */
    .word USART6_IRQHandler               /* IRQ 71 */
    .word I2C3_EV_IRQHandler              /* IRQ 72 */
    .word I2C3_ER_IRQHandler              /* IRQ 73 */
    .word OTG_HS_EP1_OUT_IRQHandler       /* IRQ 74 */
    .word OTG_HS_EP1_IN_IRQHandler        /* IRQ 75 */
    .word OTG_HS_WKUP_IRQHandler          /* IRQ 76 */
    .word OTG_HS_IRQHandler               /* IRQ 77 */
    .word DCMI_IRQHandler                 /* IRQ 78 */
    .word 0                               /* IRQ 79 reservado */
    .word 0                               /* IRQ 80 reservado */
    .word FPU_IRQHandler                  /* IRQ 81 */
    .word 0                               /* IRQ 82 reservado */
    .word 0                               /* IRQ 83 reservado */
    .word SPI4_IRQHandler                 /* IRQ 84 */
    .word 0                               /* IRQ 85 reservado */
    .word 0                               /* IRQ 86 reservado */
    .word SAI1_IRQHandler                 /* IRQ 87 */
    .word 0                               /* IRQ 88 reservado */
    .word 0                               /* IRQ 89 reservado */
    .word 0                               /* IRQ 90 reservado */
    .word SAI2_IRQHandler                 /* IRQ 91 */
    .word QUADSPI_IRQHandler              /* IRQ 92 */
    .word CEC_IRQHandler                  /* IRQ 93 */
    .word SPDIF_RX_IRQHandler             /* IRQ 94 */
    .word FMPI2C1_EV_IRQHandler           /* IRQ 95 */
    .word FMPI2C1_ER_IRQHandler           /* IRQ 96 */
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
    WEAK_IRQ WWDG_IRQHandler
    WEAK_IRQ PVD_IRQHandler
    WEAK_IRQ TAMP_STAMP_IRQHandler
    WEAK_IRQ RTC_WKUP_IRQHandler
    WEAK_IRQ FLASH_IRQHandler
    WEAK_IRQ RCC_IRQHandler
    WEAK_IRQ EXTI0_IRQHandler
    WEAK_IRQ EXTI1_IRQHandler
    WEAK_IRQ EXTI2_IRQHandler
    WEAK_IRQ EXTI3_IRQHandler
    WEAK_IRQ EXTI4_IRQHandler
    WEAK_IRQ DMA1_Stream0_IRQHandler
    WEAK_IRQ DMA1_Stream1_IRQHandler
    WEAK_IRQ DMA1_Stream2_IRQHandler
    WEAK_IRQ DMA1_Stream3_IRQHandler
    WEAK_IRQ DMA1_Stream4_IRQHandler
    WEAK_IRQ DMA1_Stream5_IRQHandler
    WEAK_IRQ DMA1_Stream6_IRQHandler
    WEAK_IRQ ADC_IRQHandler
    WEAK_IRQ CAN1_TX_IRQHandler
    WEAK_IRQ CAN1_RX0_IRQHandler
    WEAK_IRQ CAN1_RX1_IRQHandler
    WEAK_IRQ CAN1_SCE_IRQHandler
    WEAK_IRQ EXTI9_5_IRQHandler
    WEAK_IRQ TIM1_BRK_TIM9_IRQHandler
    WEAK_IRQ TIM1_UP_TIM10_IRQHandler
    WEAK_IRQ TIM1_TRG_COM_TIM11_IRQHandler
    WEAK_IRQ TIM1_CC_IRQHandler
    WEAK_IRQ TIM2_IRQHandler
    WEAK_IRQ TIM3_IRQHandler
    WEAK_IRQ TIM4_IRQHandler
    WEAK_IRQ I2C1_EV_IRQHandler
    WEAK_IRQ I2C1_ER_IRQHandler
    WEAK_IRQ I2C2_EV_IRQHandler
    WEAK_IRQ I2C2_ER_IRQHandler
    WEAK_IRQ SPI1_IRQHandler
    WEAK_IRQ SPI2_IRQHandler
    WEAK_IRQ USART1_IRQHandler
    WEAK_IRQ USART2_IRQHandler
    WEAK_IRQ USART3_IRQHandler
    WEAK_IRQ EXTI15_10_IRQHandler
    WEAK_IRQ RTC_Alarm_IRQHandler
    WEAK_IRQ OTG_FS_WKUP_IRQHandler
    WEAK_IRQ TIM8_BRK_TIM12_IRQHandler
    WEAK_IRQ TIM8_UP_TIM13_IRQHandler
    WEAK_IRQ TIM8_TRG_COM_TIM14_IRQHandler
    WEAK_IRQ TIM8_CC_IRQHandler
    WEAK_IRQ DMA1_Stream7_IRQHandler
    WEAK_IRQ FMC_IRQHandler
    WEAK_IRQ SDIO_IRQHandler
    WEAK_IRQ TIM5_IRQHandler
    WEAK_IRQ SPI3_IRQHandler
    WEAK_IRQ UART4_IRQHandler
    WEAK_IRQ UART5_IRQHandler
    WEAK_IRQ TIM6_DAC_IRQHandler
    WEAK_IRQ TIM7_IRQHandler
    WEAK_IRQ DMA2_Stream0_IRQHandler
    WEAK_IRQ DMA2_Stream1_IRQHandler
    WEAK_IRQ DMA2_Stream2_IRQHandler
    WEAK_IRQ DMA2_Stream3_IRQHandler
    WEAK_IRQ DMA2_Stream4_IRQHandler
    WEAK_IRQ CAN2_TX_IRQHandler
    WEAK_IRQ CAN2_RX0_IRQHandler
    WEAK_IRQ CAN2_RX1_IRQHandler
    WEAK_IRQ CAN2_SCE_IRQHandler
    WEAK_IRQ OTG_FS_IRQHandler
    WEAK_IRQ DMA2_Stream5_IRQHandler
    WEAK_IRQ DMA2_Stream6_IRQHandler
    WEAK_IRQ DMA2_Stream7_IRQHandler
    WEAK_IRQ USART6_IRQHandler
    WEAK_IRQ I2C3_EV_IRQHandler
    WEAK_IRQ I2C3_ER_IRQHandler
    WEAK_IRQ OTG_HS_EP1_OUT_IRQHandler
    WEAK_IRQ OTG_HS_EP1_IN_IRQHandler
    WEAK_IRQ OTG_HS_WKUP_IRQHandler
    WEAK_IRQ OTG_HS_IRQHandler
    WEAK_IRQ DCMI_IRQHandler
    WEAK_IRQ FPU_IRQHandler
    WEAK_IRQ SPI4_IRQHandler
    WEAK_IRQ SAI1_IRQHandler
    WEAK_IRQ SAI2_IRQHandler
    WEAK_IRQ QUADSPI_IRQHandler
    WEAK_IRQ CEC_IRQHandler
    WEAK_IRQ SPDIF_RX_IRQHandler
    WEAK_IRQ FMPI2C1_EV_IRQHandler
    WEAK_IRQ FMPI2C1_ER_IRQHandler
