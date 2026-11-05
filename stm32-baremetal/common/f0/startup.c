/*
 * startup.c - código de inicialização do STM32F072xB (Cortex-M0), em C.
 *
 * Explicado em detalhes em 02-startup/README.md. Responsabilidades:
 *   1. Definir a tabela de vetores (pilha inicial + endereços dos handlers)
 *   2. Reset_Handler: copiar .data da Flash para a RAM, zerar .bss,
 *      chamar construtores e então main()
 *   3. Fornecer um Default_Handler para interrupções não tratadas
 *
 * Todos os handlers são "weak": se o seu código definir uma função com o
 * mesmo nome (ex.: void TIM2_IRQHandler(void)), ela substitui a padrão.
 */
#include <stdint.h>

/* Símbolos criados pelo linker script (não são variáveis: só o ENDEREÇO importa) */
extern uint32_t _sidata;  /* início da imagem de .data na FLASH */
extern uint32_t _sdata;   /* início de .data na RAM             */
extern uint32_t _edata;   /* fim de .data na RAM                */
extern uint32_t _sbss;    /* início de .bss                     */
extern uint32_t _ebss;    /* fim de .bss                        */
extern uint32_t _estack;  /* topo da pilha (fim da RAM)         */
extern void (*__init_array_start[])(void);
extern void (*__init_array_end[])(void);

int main(void);
void Reset_Handler(void);

/* Handler padrão: trava num laço. Com o debugger dá para ver em qual IRQ
 * caiu lendo o registrador IPSR (bits [5:0] de xPSR). */
void Default_Handler(void)
{
    for (;;) { }
}

#define WEAK_DEFAULT __attribute__((weak, alias("Default_Handler")))

/* Exceções do núcleo Cortex-M0 */
void NMI_Handler(void)        WEAK_DEFAULT;
void HardFault_Handler(void)  WEAK_DEFAULT;
void SVC_Handler(void)        WEAK_DEFAULT;
void PendSV_Handler(void)     WEAK_DEFAULT;
void SysTick_Handler(void)    WEAK_DEFAULT;

/* Interrupções dos periféricos (RM0091, tabela 37) */
void WWDG_IRQHandler(void)                  WEAK_DEFAULT;
void PVD_VDDIO2_IRQHandler(void)            WEAK_DEFAULT;
void RTC_IRQHandler(void)                   WEAK_DEFAULT;
void FLASH_IRQHandler(void)                 WEAK_DEFAULT;
void RCC_CRS_IRQHandler(void)               WEAK_DEFAULT;
void EXTI0_1_IRQHandler(void)               WEAK_DEFAULT;
void EXTI2_3_IRQHandler(void)               WEAK_DEFAULT;
void EXTI4_15_IRQHandler(void)              WEAK_DEFAULT;
void TSC_IRQHandler(void)                   WEAK_DEFAULT;
void DMA1_Channel1_IRQHandler(void)         WEAK_DEFAULT;
void DMA1_Channel2_3_IRQHandler(void)       WEAK_DEFAULT;
void DMA1_Channel4_5_6_7_IRQHandler(void)   WEAK_DEFAULT;
void ADC1_COMP_IRQHandler(void)             WEAK_DEFAULT;
void TIM1_BRK_UP_TRG_COM_IRQHandler(void)   WEAK_DEFAULT;
void TIM1_CC_IRQHandler(void)               WEAK_DEFAULT;
void TIM2_IRQHandler(void)                  WEAK_DEFAULT;
void TIM3_IRQHandler(void)                  WEAK_DEFAULT;
void TIM6_DAC_IRQHandler(void)              WEAK_DEFAULT;
void TIM7_IRQHandler(void)                  WEAK_DEFAULT;
void TIM14_IRQHandler(void)                 WEAK_DEFAULT;
void TIM15_IRQHandler(void)                 WEAK_DEFAULT;
void TIM16_IRQHandler(void)                 WEAK_DEFAULT;
void TIM17_IRQHandler(void)                 WEAK_DEFAULT;
void I2C1_IRQHandler(void)                  WEAK_DEFAULT;
void I2C2_IRQHandler(void)                  WEAK_DEFAULT;
void SPI1_IRQHandler(void)                  WEAK_DEFAULT;
void SPI2_IRQHandler(void)                  WEAK_DEFAULT;
void USART1_IRQHandler(void)                WEAK_DEFAULT;
void USART2_IRQHandler(void)                WEAK_DEFAULT;
void USART3_4_IRQHandler(void)              WEAK_DEFAULT;
void CEC_CAN_IRQHandler(void)               WEAK_DEFAULT;
void USB_IRQHandler(void)                   WEAK_DEFAULT;

/*
 * Tabela de vetores. O atributo section coloca o array na seção .isr_vector,
 * que o linker script posiciona no endereço 0x08000000.
 * Entrada 0 = valor inicial do SP; entrada 1 = endereço do Reset_Handler.
 */
typedef void (*vector_t)(void);

__attribute__((section(".isr_vector"), used))
const vector_t vector_table[16 + 32] = {
    (vector_t)&_estack,             /* 0x00 SP inicial            */
    Reset_Handler,                  /* 0x04 Reset                 */
    NMI_Handler,                    /* 0x08 NMI                   */
    HardFault_Handler,              /* 0x0C HardFault             */
    0, 0, 0, 0, 0, 0, 0,            /* 0x10-0x28 reservados (M0)  */
    SVC_Handler,                    /* 0x2C SVCall                */
    0, 0,                           /* 0x30-0x34 reservados       */
    PendSV_Handler,                 /* 0x38 PendSV                */
    SysTick_Handler,                /* 0x3C SysTick               */
    /* IRQ 0.. */
    WWDG_IRQHandler,                /* 0  */
    PVD_VDDIO2_IRQHandler,          /* 1  */
    RTC_IRQHandler,                 /* 2  */
    FLASH_IRQHandler,               /* 3  */
    RCC_CRS_IRQHandler,             /* 4  */
    EXTI0_1_IRQHandler,             /* 5  */
    EXTI2_3_IRQHandler,             /* 6  */
    EXTI4_15_IRQHandler,            /* 7  */
    TSC_IRQHandler,                 /* 8  */
    DMA1_Channel1_IRQHandler,       /* 9  */
    DMA1_Channel2_3_IRQHandler,     /* 10 */
    DMA1_Channel4_5_6_7_IRQHandler, /* 11 */
    ADC1_COMP_IRQHandler,           /* 12 */
    TIM1_BRK_UP_TRG_COM_IRQHandler, /* 13 */
    TIM1_CC_IRQHandler,             /* 14 */
    TIM2_IRQHandler,                /* 15 */
    TIM3_IRQHandler,                /* 16 */
    TIM6_DAC_IRQHandler,            /* 17 */
    TIM7_IRQHandler,                /* 18 */
    TIM14_IRQHandler,               /* 19 */
    TIM15_IRQHandler,               /* 20 */
    TIM16_IRQHandler,               /* 21 */
    TIM17_IRQHandler,               /* 22 */
    I2C1_IRQHandler,                /* 23 */
    I2C2_IRQHandler,                /* 24 */
    SPI1_IRQHandler,                /* 25 */
    SPI2_IRQHandler,                /* 26 */
    USART1_IRQHandler,              /* 27 */
    USART2_IRQHandler,              /* 28 */
    USART3_4_IRQHandler,            /* 29 */
    CEC_CAN_IRQHandler,             /* 30 */
    USB_IRQHandler,                 /* 31 */
};

/*
 * no-tree-loop-distribute-patterns: impede o GCC de trocar os laços abaixo
 * por chamadas a memcpy()/memset() da libc. Funcionaria, mas é melhor que o
 * startup não dependa de nenhuma biblioteca antes de a RAM estar pronta.
 */
__attribute__((optimize("no-tree-loop-distribute-patterns")))
void Reset_Handler(void)
{
    /* 1) Copia os valores iniciais de .data (FLASH -> RAM) */
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    /* 2) Zera .bss (o padrão C exige que globais sem inicializador valham 0) */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    /* 3) Construtores (__attribute__((constructor)) ou objetos globais C++) */
    for (void (**f)(void) = __init_array_start; f < __init_array_end; f++) {
        (*f)();
    }

    /* 4) Programa do usuário */
    main();

    /* main() não deveria retornar num sistema embarcado */
    for (;;) { }
}
