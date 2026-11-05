/*
 * startup.c - código de inicialização do STM32F446xx (Cortex-M4F), em C.
 *
 * Igual ao do F0, com duas diferenças importantes:
 *   - tabela de vetores maior (exceções de falha do M4 + 97 IRQs)
 *   - a FPU precisa ser habilitada (CPACR) ANTES de qualquer código que use
 *     float, porque compilamos com -mfloat-abi=hard
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

/* Exceções do núcleo Cortex-M4 */
void NMI_Handler(void)        WEAK_DEFAULT;
void HardFault_Handler(void)  WEAK_DEFAULT;
void MemManage_Handler(void)  WEAK_DEFAULT;
void BusFault_Handler(void)   WEAK_DEFAULT;
void UsageFault_Handler(void) WEAK_DEFAULT;
void SVC_Handler(void)        WEAK_DEFAULT;
void DebugMon_Handler(void)   WEAK_DEFAULT;
void PendSV_Handler(void)     WEAK_DEFAULT;
void SysTick_Handler(void)    WEAK_DEFAULT;

/* Interrupções dos periféricos (RM0390, tabela 38) */
void WWDG_IRQHandler(void)               WEAK_DEFAULT;
void PVD_IRQHandler(void)                WEAK_DEFAULT;
void TAMP_STAMP_IRQHandler(void)         WEAK_DEFAULT;
void RTC_WKUP_IRQHandler(void)           WEAK_DEFAULT;
void FLASH_IRQHandler(void)              WEAK_DEFAULT;
void RCC_IRQHandler(void)                WEAK_DEFAULT;
void EXTI0_IRQHandler(void)              WEAK_DEFAULT;
void EXTI1_IRQHandler(void)              WEAK_DEFAULT;
void EXTI2_IRQHandler(void)              WEAK_DEFAULT;
void EXTI3_IRQHandler(void)              WEAK_DEFAULT;
void EXTI4_IRQHandler(void)              WEAK_DEFAULT;
void DMA1_Stream0_IRQHandler(void)       WEAK_DEFAULT;
void DMA1_Stream1_IRQHandler(void)       WEAK_DEFAULT;
void DMA1_Stream2_IRQHandler(void)       WEAK_DEFAULT;
void DMA1_Stream3_IRQHandler(void)       WEAK_DEFAULT;
void DMA1_Stream4_IRQHandler(void)       WEAK_DEFAULT;
void DMA1_Stream5_IRQHandler(void)       WEAK_DEFAULT;
void DMA1_Stream6_IRQHandler(void)       WEAK_DEFAULT;
void ADC_IRQHandler(void)                WEAK_DEFAULT;
void CAN1_TX_IRQHandler(void)            WEAK_DEFAULT;
void CAN1_RX0_IRQHandler(void)           WEAK_DEFAULT;
void CAN1_RX1_IRQHandler(void)           WEAK_DEFAULT;
void CAN1_SCE_IRQHandler(void)           WEAK_DEFAULT;
void EXTI9_5_IRQHandler(void)            WEAK_DEFAULT;
void TIM1_BRK_TIM9_IRQHandler(void)      WEAK_DEFAULT;
void TIM1_UP_TIM10_IRQHandler(void)      WEAK_DEFAULT;
void TIM1_TRG_COM_TIM11_IRQHandler(void) WEAK_DEFAULT;
void TIM1_CC_IRQHandler(void)            WEAK_DEFAULT;
void TIM2_IRQHandler(void)               WEAK_DEFAULT;
void TIM3_IRQHandler(void)               WEAK_DEFAULT;
void TIM4_IRQHandler(void)               WEAK_DEFAULT;
void I2C1_EV_IRQHandler(void)            WEAK_DEFAULT;
void I2C1_ER_IRQHandler(void)            WEAK_DEFAULT;
void I2C2_EV_IRQHandler(void)            WEAK_DEFAULT;
void I2C2_ER_IRQHandler(void)            WEAK_DEFAULT;
void SPI1_IRQHandler(void)               WEAK_DEFAULT;
void SPI2_IRQHandler(void)               WEAK_DEFAULT;
void USART1_IRQHandler(void)             WEAK_DEFAULT;
void USART2_IRQHandler(void)             WEAK_DEFAULT;
void USART3_IRQHandler(void)             WEAK_DEFAULT;
void EXTI15_10_IRQHandler(void)          WEAK_DEFAULT;
void RTC_Alarm_IRQHandler(void)          WEAK_DEFAULT;
void OTG_FS_WKUP_IRQHandler(void)        WEAK_DEFAULT;
void TIM8_BRK_TIM12_IRQHandler(void)     WEAK_DEFAULT;
void TIM8_UP_TIM13_IRQHandler(void)      WEAK_DEFAULT;
void TIM8_TRG_COM_TIM14_IRQHandler(void) WEAK_DEFAULT;
void TIM8_CC_IRQHandler(void)            WEAK_DEFAULT;
void DMA1_Stream7_IRQHandler(void)       WEAK_DEFAULT;
void FMC_IRQHandler(void)                WEAK_DEFAULT;
void SDIO_IRQHandler(void)               WEAK_DEFAULT;
void TIM5_IRQHandler(void)               WEAK_DEFAULT;
void SPI3_IRQHandler(void)               WEAK_DEFAULT;
void UART4_IRQHandler(void)              WEAK_DEFAULT;
void UART5_IRQHandler(void)              WEAK_DEFAULT;
void TIM6_DAC_IRQHandler(void)           WEAK_DEFAULT;
void TIM7_IRQHandler(void)               WEAK_DEFAULT;
void DMA2_Stream0_IRQHandler(void)       WEAK_DEFAULT;
void DMA2_Stream1_IRQHandler(void)       WEAK_DEFAULT;
void DMA2_Stream2_IRQHandler(void)       WEAK_DEFAULT;
void DMA2_Stream3_IRQHandler(void)       WEAK_DEFAULT;
void DMA2_Stream4_IRQHandler(void)       WEAK_DEFAULT;
void CAN2_TX_IRQHandler(void)            WEAK_DEFAULT;
void CAN2_RX0_IRQHandler(void)           WEAK_DEFAULT;
void CAN2_RX1_IRQHandler(void)           WEAK_DEFAULT;
void CAN2_SCE_IRQHandler(void)           WEAK_DEFAULT;
void OTG_FS_IRQHandler(void)             WEAK_DEFAULT;
void DMA2_Stream5_IRQHandler(void)       WEAK_DEFAULT;
void DMA2_Stream6_IRQHandler(void)       WEAK_DEFAULT;
void DMA2_Stream7_IRQHandler(void)       WEAK_DEFAULT;
void USART6_IRQHandler(void)             WEAK_DEFAULT;
void I2C3_EV_IRQHandler(void)            WEAK_DEFAULT;
void I2C3_ER_IRQHandler(void)            WEAK_DEFAULT;
void OTG_HS_EP1_OUT_IRQHandler(void)     WEAK_DEFAULT;
void OTG_HS_EP1_IN_IRQHandler(void)      WEAK_DEFAULT;
void OTG_HS_WKUP_IRQHandler(void)        WEAK_DEFAULT;
void OTG_HS_IRQHandler(void)             WEAK_DEFAULT;
void DCMI_IRQHandler(void)               WEAK_DEFAULT;
void FPU_IRQHandler(void)                WEAK_DEFAULT;
void SPI4_IRQHandler(void)               WEAK_DEFAULT;
void SAI1_IRQHandler(void)               WEAK_DEFAULT;
void SAI2_IRQHandler(void)               WEAK_DEFAULT;
void QUADSPI_IRQHandler(void)            WEAK_DEFAULT;
void CEC_IRQHandler(void)                WEAK_DEFAULT;
void SPDIF_RX_IRQHandler(void)           WEAK_DEFAULT;
void FMPI2C1_EV_IRQHandler(void)         WEAK_DEFAULT;
void FMPI2C1_ER_IRQHandler(void)         WEAK_DEFAULT;

typedef void (*vector_t)(void);

__attribute__((section(".isr_vector"), used))
const vector_t vector_table[16 + 97] = {
    (vector_t)&_estack,            /* 0x00 SP inicial */
    Reset_Handler,                 /* 0x04 */
    NMI_Handler,                   /* 0x08 */
    HardFault_Handler,             /* 0x0C */
    MemManage_Handler,             /* 0x10 */
    BusFault_Handler,              /* 0x14 */
    UsageFault_Handler,            /* 0x18 */
    0, 0, 0, 0,                    /* 0x1C-0x28 reservados */
    SVC_Handler,                   /* 0x2C */
    DebugMon_Handler,              /* 0x30 */
    0,                             /* 0x34 reservado */
    PendSV_Handler,                /* 0x38 */
    SysTick_Handler,               /* 0x3C */
    /* IRQ 0.. */
    WWDG_IRQHandler,               /* 0  */
    PVD_IRQHandler,                /* 1  */
    TAMP_STAMP_IRQHandler,         /* 2  */
    RTC_WKUP_IRQHandler,           /* 3  */
    FLASH_IRQHandler,              /* 4  */
    RCC_IRQHandler,                /* 5  */
    EXTI0_IRQHandler,              /* 6  */
    EXTI1_IRQHandler,              /* 7  */
    EXTI2_IRQHandler,              /* 8  */
    EXTI3_IRQHandler,              /* 9  */
    EXTI4_IRQHandler,              /* 10 */
    DMA1_Stream0_IRQHandler,       /* 11 */
    DMA1_Stream1_IRQHandler,       /* 12 */
    DMA1_Stream2_IRQHandler,       /* 13 */
    DMA1_Stream3_IRQHandler,       /* 14 */
    DMA1_Stream4_IRQHandler,       /* 15 */
    DMA1_Stream5_IRQHandler,       /* 16 */
    DMA1_Stream6_IRQHandler,       /* 17 */
    ADC_IRQHandler,                /* 18 */
    CAN1_TX_IRQHandler,            /* 19 */
    CAN1_RX0_IRQHandler,           /* 20 */
    CAN1_RX1_IRQHandler,           /* 21 */
    CAN1_SCE_IRQHandler,           /* 22 */
    EXTI9_5_IRQHandler,            /* 23 */
    TIM1_BRK_TIM9_IRQHandler,      /* 24 */
    TIM1_UP_TIM10_IRQHandler,      /* 25 */
    TIM1_TRG_COM_TIM11_IRQHandler, /* 26 */
    TIM1_CC_IRQHandler,            /* 27 */
    TIM2_IRQHandler,               /* 28 */
    TIM3_IRQHandler,               /* 29 */
    TIM4_IRQHandler,               /* 30 */
    I2C1_EV_IRQHandler,            /* 31 */
    I2C1_ER_IRQHandler,            /* 32 */
    I2C2_EV_IRQHandler,            /* 33 */
    I2C2_ER_IRQHandler,            /* 34 */
    SPI1_IRQHandler,               /* 35 */
    SPI2_IRQHandler,               /* 36 */
    USART1_IRQHandler,             /* 37 */
    USART2_IRQHandler,             /* 38 */
    USART3_IRQHandler,             /* 39 */
    EXTI15_10_IRQHandler,          /* 40 */
    RTC_Alarm_IRQHandler,          /* 41 */
    OTG_FS_WKUP_IRQHandler,        /* 42 */
    TIM8_BRK_TIM12_IRQHandler,     /* 43 */
    TIM8_UP_TIM13_IRQHandler,      /* 44 */
    TIM8_TRG_COM_TIM14_IRQHandler, /* 45 */
    TIM8_CC_IRQHandler,            /* 46 */
    DMA1_Stream7_IRQHandler,       /* 47 */
    FMC_IRQHandler,                /* 48 */
    SDIO_IRQHandler,               /* 49 */
    TIM5_IRQHandler,               /* 50 */
    SPI3_IRQHandler,               /* 51 */
    UART4_IRQHandler,              /* 52 */
    UART5_IRQHandler,              /* 53 */
    TIM6_DAC_IRQHandler,           /* 54 */
    TIM7_IRQHandler,               /* 55 */
    DMA2_Stream0_IRQHandler,       /* 56 */
    DMA2_Stream1_IRQHandler,       /* 57 */
    DMA2_Stream2_IRQHandler,       /* 58 */
    DMA2_Stream3_IRQHandler,       /* 59 */
    DMA2_Stream4_IRQHandler,       /* 60 */
    0,                             /* 61 reservado */
    0,                             /* 62 reservado */
    CAN2_TX_IRQHandler,            /* 63 */
    CAN2_RX0_IRQHandler,           /* 64 */
    CAN2_RX1_IRQHandler,           /* 65 */
    CAN2_SCE_IRQHandler,           /* 66 */
    OTG_FS_IRQHandler,             /* 67 */
    DMA2_Stream5_IRQHandler,       /* 68 */
    DMA2_Stream6_IRQHandler,       /* 69 */
    DMA2_Stream7_IRQHandler,       /* 70 */
    USART6_IRQHandler,             /* 71 */
    I2C3_EV_IRQHandler,            /* 72 */
    I2C3_ER_IRQHandler,            /* 73 */
    OTG_HS_EP1_OUT_IRQHandler,     /* 74 */
    OTG_HS_EP1_IN_IRQHandler,      /* 75 */
    OTG_HS_WKUP_IRQHandler,        /* 76 */
    OTG_HS_IRQHandler,             /* 77 */
    DCMI_IRQHandler,               /* 78 */
    0,                             /* 79 reservado */
    0,                             /* 80 reservado */
    FPU_IRQHandler,                /* 81 */
    0,                             /* 82 reservado */
    0,                             /* 83 reservado */
    SPI4_IRQHandler,               /* 84 */
    0,                             /* 85 reservado */
    0,                             /* 86 reservado */
    SAI1_IRQHandler,               /* 87 */
    0,                             /* 88 reservado */
    0,                             /* 89 reservado */
    0,                             /* 90 reservado */
    SAI2_IRQHandler,               /* 91 */
    QUADSPI_IRQHandler,            /* 92 */
    CEC_IRQHandler,                /* 93 */
    SPDIF_RX_IRQHandler,           /* 94 */
    FMPI2C1_EV_IRQHandler,         /* 95 */
    FMPI2C1_ER_IRQHandler,         /* 96 */
};

/* CPACR fica no SCB, endereço 0xE000ED88 */
#define SCB_CPACR (*(volatile uint32_t *)0xE000ED88UL)

/*
 * no-tree-loop-distribute-patterns: impede o GCC de trocar os laços abaixo
 * por chamadas a memcpy()/memset() da libc. Funcionaria, mas é melhor que o
 * startup não dependa de nenhuma biblioteca antes de a RAM estar pronta.
 */
__attribute__((optimize("no-tree-loop-distribute-patterns")))
void Reset_Handler(void)
{
    /* 0) Habilita a FPU: acesso total (0b11) aos coprocessadores CP10 e CP11.
     *    Sem isso, a primeira instrução VFP gera UsageFault (NOCP). */
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
