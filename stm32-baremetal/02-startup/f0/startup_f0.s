/* =============================================================================
 *  startup_f0.s - Startup em ASSEMBLY para STM32F072xB (Cortex-M0 / ARMv6-M)
 *
 *  Faz exatamente o mesmo que common/f0/startup.c, instrução por instrução.
 *  ARMv6-M só tem o conjunto Thumb-1 (+ algumas de 32 bits): sem IT, sem
 *  CBZ, sem LDR/STR com pós-incremento, imediatos pequenos...
 * ========================================================================== */

    .syntax unified          /* sintaxe UAL (unificada ARM/Thumb)           */
    .cpu    cortex-m0
    .fpu    softvfp
    .thumb                   /* Cortex-M só executa Thumb                   */

    .global vector_table
    .global Reset_Handler
    .global Default_Handler

/* -----------------------------------------------------------------------------
 *  Reset_Handler: primeiro código executado após o reset.
 *  O hardware já carregou SP = vector_table[0] e PC = vector_table[1].
 * -------------------------------------------------------------------------- */
    .section .text.Reset_Handler, "ax", %progbits
    .type    Reset_Handler, %function
Reset_Handler:
    ldr   r0, =_estack       /* (re)define SP: útil se o boot veio de um     */
    mov   sp, r0             /* bootloader. No M0 não existe "ldr sp, =x".  */

    /* ---- copia .data: FLASH (_sidata) -> RAM (_sdata.._edata) ---- */
    ldr   r0, =_sdata        /* r0 = destino                                 */
    ldr   r1, =_edata        /* r1 = fim do destino                          */
    ldr   r2, =_sidata       /* r2 = origem                                  */
    movs  r3, #0             /* r3 = offset                                  */
    b     2f
1:  ldr   r4, [r2, r3]       /* r4 = *(origem + offset)                      */
    str   r4, [r0, r3]       /* *(destino + offset) = r4                     */
    adds  r3, r3, #4
2:  adds  r4, r0, r3         /* r4 = destino + offset                        */
    cmp   r4, r1
    bcc   1b                 /* enquanto r4 < fim (sem sinal)                */

    /* ---- zera .bss ---- */
    ldr   r2, =_sbss
    ldr   r4, =_ebss
    movs  r3, #0
    b     4f
3:  str   r3, [r2]
    adds  r2, r2, #4
4:  cmp   r2, r4
    bcc   3b

    /* ---- chama main() ---- */
    bl    main

    /* main não deve retornar; se retornar, trava aqui */
5:  b     5b
    .size Reset_Handler, . - Reset_Handler

/* -----------------------------------------------------------------------------
 *  Default_Handler: destino de toda interrupção sem handler definido.
 * -------------------------------------------------------------------------- */
    .section .text.Default_Handler, "ax", %progbits
    .type    Default_Handler, %function
Default_Handler:
    b     Default_Handler
    .size Default_Handler, . - Default_Handler

/* -----------------------------------------------------------------------------
 *  Tabela de vetores. Cada entrada é uma word (.word) com um endereço.
 *  Como os símbolos são %function Thumb, o assembler/linker põe o bit 0 = 1.
 * -------------------------------------------------------------------------- */
    .section .isr_vector, "a", %progbits
    .type    vector_table, %object
vector_table:
    .word _estack                         /* 0x00 SP inicial      */
    .word Reset_Handler                   /* 0x04 Reset           */
    .word NMI_Handler                     /* 0x08                 */
    .word HardFault_Handler               /* 0x0C                 */
    .word 0, 0, 0, 0, 0, 0, 0             /* 0x10-0x28 reservados */
    .word SVC_Handler                     /* 0x2C                 */
    .word 0, 0                            /* 0x30-0x34 reservados */
    .word PendSV_Handler                  /* 0x38                 */
    .word SysTick_Handler                 /* 0x3C                 */
    .word WWDG_IRQHandler                 /* IRQ 0  */
    .word PVD_VDDIO2_IRQHandler           /* IRQ 1  */
    .word RTC_IRQHandler                  /* IRQ 2  */
    .word FLASH_IRQHandler                /* IRQ 3  */
    .word RCC_CRS_IRQHandler              /* IRQ 4  */
    .word EXTI0_1_IRQHandler              /* IRQ 5  */
    .word EXTI2_3_IRQHandler              /* IRQ 6  */
    .word EXTI4_15_IRQHandler             /* IRQ 7  */
    .word TSC_IRQHandler                  /* IRQ 8  */
    .word DMA1_Channel1_IRQHandler        /* IRQ 9  */
    .word DMA1_Channel2_3_IRQHandler      /* IRQ 10 */
    .word DMA1_Channel4_5_6_7_IRQHandler  /* IRQ 11 */
    .word ADC1_COMP_IRQHandler            /* IRQ 12 */
    .word TIM1_BRK_UP_TRG_COM_IRQHandler  /* IRQ 13 */
    .word TIM1_CC_IRQHandler              /* IRQ 14 */
    .word TIM2_IRQHandler                 /* IRQ 15 */
    .word TIM3_IRQHandler                 /* IRQ 16 */
    .word TIM6_DAC_IRQHandler             /* IRQ 17 */
    .word TIM7_IRQHandler                 /* IRQ 18 */
    .word TIM14_IRQHandler                /* IRQ 19 */
    .word TIM15_IRQHandler                /* IRQ 20 */
    .word TIM16_IRQHandler                /* IRQ 21 */
    .word TIM17_IRQHandler                /* IRQ 22 */
    .word I2C1_IRQHandler                 /* IRQ 23 */
    .word I2C2_IRQHandler                 /* IRQ 24 */
    .word SPI1_IRQHandler                 /* IRQ 25 */
    .word SPI2_IRQHandler                 /* IRQ 26 */
    .word USART1_IRQHandler               /* IRQ 27 */
    .word USART2_IRQHandler               /* IRQ 28 */
    .word USART3_4_IRQHandler             /* IRQ 29 */
    .word CEC_CAN_IRQHandler              /* IRQ 30 */
    .word USB_IRQHandler                  /* IRQ 31 */
    .size vector_table, . - vector_table

/* -----------------------------------------------------------------------------
 *  Aliases fracos: cada handler aponta para Default_Handler, a menos que
 *  o programa defina uma função com o mesmo nome (símbolo forte).
 * -------------------------------------------------------------------------- */
    .macro WEAK_IRQ name
    .weak      \name
    .thumb_set \name, Default_Handler
    .endm

    WEAK_IRQ NMI_Handler
    WEAK_IRQ HardFault_Handler
    WEAK_IRQ SVC_Handler
    WEAK_IRQ PendSV_Handler
    WEAK_IRQ SysTick_Handler
    WEAK_IRQ WWDG_IRQHandler
    WEAK_IRQ PVD_VDDIO2_IRQHandler
    WEAK_IRQ RTC_IRQHandler
    WEAK_IRQ FLASH_IRQHandler
    WEAK_IRQ RCC_CRS_IRQHandler
    WEAK_IRQ EXTI0_1_IRQHandler
    WEAK_IRQ EXTI2_3_IRQHandler
    WEAK_IRQ EXTI4_15_IRQHandler
    WEAK_IRQ TSC_IRQHandler
    WEAK_IRQ DMA1_Channel1_IRQHandler
    WEAK_IRQ DMA1_Channel2_3_IRQHandler
    WEAK_IRQ DMA1_Channel4_5_6_7_IRQHandler
    WEAK_IRQ ADC1_COMP_IRQHandler
    WEAK_IRQ TIM1_BRK_UP_TRG_COM_IRQHandler
    WEAK_IRQ TIM1_CC_IRQHandler
    WEAK_IRQ TIM2_IRQHandler
    WEAK_IRQ TIM3_IRQHandler
    WEAK_IRQ TIM6_DAC_IRQHandler
    WEAK_IRQ TIM7_IRQHandler
    WEAK_IRQ TIM14_IRQHandler
    WEAK_IRQ TIM15_IRQHandler
    WEAK_IRQ TIM16_IRQHandler
    WEAK_IRQ TIM17_IRQHandler
    WEAK_IRQ I2C1_IRQHandler
    WEAK_IRQ I2C2_IRQHandler
    WEAK_IRQ SPI1_IRQHandler
    WEAK_IRQ SPI2_IRQHandler
    WEAK_IRQ USART1_IRQHandler
    WEAK_IRQ USART2_IRQHandler
    WEAK_IRQ USART3_4_IRQHandler
    WEAK_IRQ CEC_CAN_IRQHandler
    WEAK_IRQ USB_IRQHandler
