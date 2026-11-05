/*
 * stm32f0.h - Definições de registradores do STM32F072xB (Cortex-M0)
 *
 * Escrito à mão para o curso, a partir do Reference Manual RM0091 e do
 * datasheet DS9826. NÃO usa CMSIS, HAL ou LL.
 *
 * Convenções:
 *   - Cada periférico é uma struct cujos campos estão na MESMA ordem e com o
 *     MESMO offset dos registradores no RM. Os comentários mostram o offset.
 *   - "volatile" é obrigatório: o hardware muda o conteúdo dos registradores
 *     "por fora" do programa, e o compilador não pode otimizar os acessos.
 *   - Macros de bits seguem o padrão PERIFERICO_REGISTRADOR_CAMPO.
 */
#ifndef STM32F0_H
#define STM32F0_H

#include <stdint.h>

#define __IO volatile
#define __I  volatile const

/* ===========================================================================
 *  Mapa de memória (RM0091, seção 2.2)
 * ======================================================================== */
#define FLASH_BASE      0x08000000UL   /* 128 KB de Flash                     */
#define SRAM_BASE       0x20000000UL   /* 16 KB de SRAM                       */
#define SYSMEM_BASE     0x1FFFC800UL   /* bootloader de sistema (ROM da ST)   */
#define PERIPH_BASE     0x40000000UL

#define APBPERIPH_BASE  PERIPH_BASE
#define AHBPERIPH_BASE  0x40020000UL
#define AHB2PERIPH_BASE 0x48000000UL

/* APB */
#define TIM2_BASE       0x40000000UL
#define TIM3_BASE       0x40000400UL
#define TIM6_BASE       0x40001000UL
#define TIM7_BASE       0x40001400UL
#define TIM14_BASE      0x40002000UL
#define RTC_BASE        0x40002800UL
#define WWDG_BASE       0x40002C00UL
#define IWDG_BASE       0x40003000UL
#define SPI2_BASE       0x40003800UL
#define USART2_BASE     0x40004400UL
#define USART3_BASE     0x40004800UL
#define USART4_BASE     0x40004C00UL
#define I2C1_BASE       0x40005400UL
#define I2C2_BASE       0x40005800UL
#define USB_BASE        0x40005C00UL
#define CAN_BASE        0x40006400UL
#define CRS_BASE        0x40006C00UL
#define PWR_BASE        0x40007000UL
#define DAC_BASE        0x40007400UL
#define SYSCFG_BASE     0x40010000UL
#define EXTI_BASE       0x40010400UL
#define ADC1_BASE       0x40012400UL
#define ADC_COMMON_BASE 0x40012708UL
#define TIM1_BASE       0x40012C00UL
#define SPI1_BASE       0x40013000UL
#define USART1_BASE     0x40013800UL
#define TIM15_BASE      0x40014000UL
#define TIM16_BASE      0x40014400UL
#define TIM17_BASE      0x40014800UL
#define DBGMCU_BASE     0x40015800UL

/* AHB1 */
#define DMA1_BASE       0x40020000UL
#define RCC_BASE        0x40021000UL
#define FLASH_R_BASE    0x40022000UL
#define CRC_BASE        0x40023000UL

/* AHB2 (GPIO ligado direto ao barramento AHB -> 1 ciclo por acesso) */
#define GPIOA_BASE      0x48000000UL
#define GPIOB_BASE      0x48000400UL
#define GPIOC_BASE      0x48000800UL
#define GPIOD_BASE      0x48000C00UL
#define GPIOE_BASE      0x48001000UL
#define GPIOF_BASE      0x48001400UL

/* Núcleo Cortex-M0 (Private Peripheral Bus) */
#define SCS_BASE        0xE000E000UL
#define SYSTICK_BASE    0xE000E010UL
#define NVIC_BASE       0xE000E100UL
#define SCB_BASE        0xE000ED00UL

/* Valores de calibração gravados na fábrica (System Memory) */
#define TS_CAL1_ADDR    ((const uint16_t *)0x1FFFF7B8UL) /* sensor temp @30C, 3.3V  */
#define VREFINT_CAL_ADDR ((const uint16_t *)0x1FFFF7BAUL)/* VREFINT @30C, 3.3V      */
#define TS_CAL2_ADDR    ((const uint16_t *)0x1FFFF7C2UL) /* sensor temp @110C, 3.3V */
#define UID_BASE        0x1FFFF7ACUL                     /* Unique ID 96 bits       */
#define FLASHSIZE_BASE  0x1FFFF7CCUL                     /* tamanho da flash em KB  */

/* ===========================================================================
 *  Núcleo: SysTick, NVIC, SCB
 * ======================================================================== */
typedef struct {
    __IO uint32_t CTRL;   /* 0x00 */
    __IO uint32_t LOAD;   /* 0x04 */
    __IO uint32_t VAL;    /* 0x08 */
    __I  uint32_t CALIB;  /* 0x0C */
} SysTick_TypeDef;

#define SysTick_CTRL_ENABLE     (1U << 0)
#define SysTick_CTRL_TICKINT    (1U << 1)
#define SysTick_CTRL_CLKSOURCE  (1U << 2)  /* 1 = HCLK, 0 = HCLK/8 */
#define SysTick_CTRL_COUNTFLAG  (1U << 16)

typedef struct {
    __IO uint32_t ISER[1];      /* 0x000 Interrupt Set-Enable     */
    uint32_t      RES0[31];
    __IO uint32_t ICER[1];      /* 0x080 Interrupt Clear-Enable   */
    uint32_t      RES1[31];
    __IO uint32_t ISPR[1];      /* 0x100 Interrupt Set-Pending    */
    uint32_t      RES2[31];
    __IO uint32_t ICPR[1];      /* 0x180 Interrupt Clear-Pending  */
    uint32_t      RES3[95];
    __IO uint32_t IPR[8];       /* 0x300 Priority (acesso SÓ por word no M0) */
} NVIC_TypeDef;

typedef struct {
    __I  uint32_t CPUID;  /* 0x00 */
    __IO uint32_t ICSR;   /* 0x04 */
    uint32_t      RES0;   /* 0x08 (M0 NÃO possui VTOR) */
    __IO uint32_t AIRCR;  /* 0x0C */
    __IO uint32_t SCR;    /* 0x10 */
    __IO uint32_t CCR;    /* 0x14 */
    uint32_t      RES1;   /* 0x18 */
    __IO uint32_t SHPR[2];/* 0x1C, 0x20 */
} SCB_TypeDef;

#define SCB_SCR_SLEEPONEXIT  (1U << 1)
#define SCB_SCR_SLEEPDEEP    (1U << 2)
#define SCB_AIRCR_VECTKEY    (0x05FAU << 16)
#define SCB_AIRCR_SYSRESETREQ (1U << 2)

#define SysTick ((SysTick_TypeDef *) SYSTICK_BASE)
#define NVIC    ((NVIC_TypeDef *)    NVIC_BASE)
#define SCB     ((SCB_TypeDef *)     SCB_BASE)

/* ===========================================================================
 *  Números de interrupção (posição na tabela de vetores - 16)
 * ======================================================================== */
typedef enum {
    WWDG_IRQn               = 0,
    PVD_VDDIO2_IRQn         = 1,
    RTC_IRQn                = 2,
    FLASH_IRQn              = 3,
    RCC_CRS_IRQn            = 4,
    EXTI0_1_IRQn            = 5,
    EXTI2_3_IRQn            = 6,
    EXTI4_15_IRQn           = 7,
    TSC_IRQn                = 8,
    DMA1_Channel1_IRQn      = 9,
    DMA1_Channel2_3_IRQn    = 10,
    DMA1_Channel4_5_6_7_IRQn= 11,
    ADC1_COMP_IRQn          = 12,
    TIM1_BRK_UP_TRG_COM_IRQn= 13,
    TIM1_CC_IRQn            = 14,
    TIM2_IRQn               = 15,
    TIM3_IRQn               = 16,
    TIM6_DAC_IRQn           = 17,
    TIM7_IRQn               = 18,
    TIM14_IRQn              = 19,
    TIM15_IRQn              = 20,
    TIM16_IRQn              = 21,
    TIM17_IRQn              = 22,
    I2C1_IRQn               = 23,
    I2C2_IRQn               = 24,
    SPI1_IRQn               = 25,
    SPI2_IRQn               = 26,
    USART1_IRQn             = 27,
    USART2_IRQn             = 28,
    USART3_4_IRQn           = 29,
    CEC_CAN_IRQn            = 30,
    USB_IRQn                = 31,
} IRQn_Type;

#define NVIC_PRIO_BITS 2   /* M0: apenas 4 níveis de prioridade (bits [7:6]) */

/* ===========================================================================
 *  RCC - Reset and Clock Control (RM0091 seção 6)
 * ======================================================================== */
typedef struct {
    __IO uint32_t CR;        /* 0x00 Clock control                    */
    __IO uint32_t CFGR;      /* 0x04 Clock configuration              */
    __IO uint32_t CIR;       /* 0x08 Clock interrupt                  */
    __IO uint32_t APB2RSTR;  /* 0x0C APB2 peripheral reset            */
    __IO uint32_t APB1RSTR;  /* 0x10 APB1 peripheral reset            */
    __IO uint32_t AHBENR;    /* 0x14 AHB peripheral clock enable      */
    __IO uint32_t APB2ENR;   /* 0x18 APB2 peripheral clock enable     */
    __IO uint32_t APB1ENR;   /* 0x1C APB1 peripheral clock enable     */
    __IO uint32_t BDCR;      /* 0x20 Backup domain control            */
    __IO uint32_t CSR;       /* 0x24 Control/status                   */
    __IO uint32_t AHBRSTR;   /* 0x28 AHB peripheral reset             */
    __IO uint32_t CFGR2;     /* 0x2C Clock configuration 2 (PREDIV)   */
    __IO uint32_t CFGR3;     /* 0x30 Clock configuration 3 (mux USART/I2C) */
    __IO uint32_t CR2;       /* 0x34 Clock control 2 (HSI14, HSI48)   */
} RCC_TypeDef;

/* RCC_CR */
#define RCC_CR_HSION        (1U << 0)
#define RCC_CR_HSIRDY       (1U << 1)
#define RCC_CR_HSEON        (1U << 16)
#define RCC_CR_HSERDY       (1U << 17)
#define RCC_CR_HSEBYP       (1U << 18)
#define RCC_CR_CSSON        (1U << 19)
#define RCC_CR_PLLON        (1U << 24)
#define RCC_CR_PLLRDY       (1U << 25)
/* RCC_CR2 */
#define RCC_CR2_HSI14ON     (1U << 0)
#define RCC_CR2_HSI14RDY    (1U << 1)
#define RCC_CR2_HSI48ON     (1U << 16)
#define RCC_CR2_HSI48RDY    (1U << 17)
/* RCC_CFGR */
#define RCC_CFGR_SW_Pos     0
#define RCC_CFGR_SW_Msk     (3U << 0)
#define RCC_CFGR_SW_HSI     (0U << 0)
#define RCC_CFGR_SW_HSE     (1U << 0)
#define RCC_CFGR_SW_PLL     (2U << 0)
#define RCC_CFGR_SW_HSI48   (3U << 0)
#define RCC_CFGR_SWS_Msk    (3U << 2)
#define RCC_CFGR_SWS_PLL    (2U << 2)
#define RCC_CFGR_HPRE_Msk   (0xFU << 4)
#define RCC_CFGR_PPRE_Msk   (7U << 8)
#define RCC_CFGR_PLLSRC_Msk (3U << 15)
#define RCC_CFGR_PLLSRC_HSI_DIV2   (0U << 15)
#define RCC_CFGR_PLLSRC_HSI_PREDIV (1U << 15)
#define RCC_CFGR_PLLSRC_HSE_PREDIV (2U << 15)
#define RCC_CFGR_PLLSRC_HSI48_PREDIV (3U << 15)
#define RCC_CFGR_PLLMUL_Pos 18
#define RCC_CFGR_PLLMUL_Msk (0xFU << 18)
#define RCC_CFGR_MCO_Pos    24
#define RCC_CFGR_MCO_Msk    (0xFU << 24)
/* RCC_AHBENR */
#define RCC_AHBENR_DMAEN    (1U << 0)
#define RCC_AHBENR_SRAMEN   (1U << 2)
#define RCC_AHBENR_FLITFEN  (1U << 4)
#define RCC_AHBENR_CRCEN    (1U << 6)
#define RCC_AHBENR_GPIOAEN  (1U << 17)
#define RCC_AHBENR_GPIOBEN  (1U << 18)
#define RCC_AHBENR_GPIOCEN  (1U << 19)
#define RCC_AHBENR_GPIODEN  (1U << 20)
#define RCC_AHBENR_GPIOEEN  (1U << 21)
#define RCC_AHBENR_GPIOFEN  (1U << 22)
/* RCC_APB2ENR */
#define RCC_APB2ENR_SYSCFGEN (1U << 0)
#define RCC_APB2ENR_ADCEN    (1U << 9)
#define RCC_APB2ENR_TIM1EN   (1U << 11)
#define RCC_APB2ENR_SPI1EN   (1U << 12)
#define RCC_APB2ENR_USART1EN (1U << 14)
#define RCC_APB2ENR_TIM15EN  (1U << 16)
#define RCC_APB2ENR_TIM16EN  (1U << 17)
#define RCC_APB2ENR_TIM17EN  (1U << 18)
#define RCC_APB2ENR_DBGMCUEN (1U << 22)
/* RCC_APB1ENR */
#define RCC_APB1ENR_TIM2EN   (1U << 0)
#define RCC_APB1ENR_TIM3EN   (1U << 1)
#define RCC_APB1ENR_TIM6EN   (1U << 4)
#define RCC_APB1ENR_TIM7EN   (1U << 5)
#define RCC_APB1ENR_TIM14EN  (1U << 8)
#define RCC_APB1ENR_WWDGEN   (1U << 11)
#define RCC_APB1ENR_SPI2EN   (1U << 14)
#define RCC_APB1ENR_USART2EN (1U << 17)
#define RCC_APB1ENR_USART3EN (1U << 18)
#define RCC_APB1ENR_USART4EN (1U << 19)
#define RCC_APB1ENR_I2C1EN   (1U << 21)
#define RCC_APB1ENR_I2C2EN   (1U << 22)
#define RCC_APB1ENR_USBEN    (1U << 23)
#define RCC_APB1ENR_CANEN    (1U << 25)
#define RCC_APB1ENR_CRSEN    (1U << 27)
#define RCC_APB1ENR_PWREN    (1U << 28)
#define RCC_APB1ENR_DACEN    (1U << 29)
/* RCC_BDCR */
#define RCC_BDCR_LSEON      (1U << 0)
#define RCC_BDCR_LSERDY     (1U << 1)
#define RCC_BDCR_LSEBYP     (1U << 2)
#define RCC_BDCR_LSEDRV_Msk (3U << 3)
#define RCC_BDCR_RTCSEL_Msk (3U << 8)
#define RCC_BDCR_RTCSEL_LSE (1U << 8)
#define RCC_BDCR_RTCSEL_LSI (2U << 8)
#define RCC_BDCR_RTCEN      (1U << 15)
#define RCC_BDCR_BDRST      (1U << 16)
/* RCC_CSR */
#define RCC_CSR_LSION       (1U << 0)
#define RCC_CSR_LSIRDY      (1U << 1)
#define RCC_CSR_RMVF        (1U << 24)
#define RCC_CSR_OBLRSTF     (1U << 25)
#define RCC_CSR_PINRSTF     (1U << 26)
#define RCC_CSR_PORRSTF     (1U << 27)
#define RCC_CSR_SFTRSTF     (1U << 28)
#define RCC_CSR_IWDGRSTF    (1U << 29)
#define RCC_CSR_WWDGRSTF    (1U << 30)
#define RCC_CSR_LPWRRSTF    (1U << 31)

#define RCC ((RCC_TypeDef *) RCC_BASE)

/* ===========================================================================
 *  FLASH (interface) - RM0091 seção 3
 * ======================================================================== */
typedef struct {
    __IO uint32_t ACR;      /* 0x00 Access control (latência)  */
    __IO uint32_t KEYR;     /* 0x04 */
    __IO uint32_t OPTKEYR;  /* 0x08 */
    __IO uint32_t SR;       /* 0x0C */
    __IO uint32_t CR;       /* 0x10 */
    __IO uint32_t AR;       /* 0x14 Address (para apagar página) */
    uint32_t      RES;      /* 0x18 */
    __I  uint32_t OBR;      /* 0x1C */
    __I  uint32_t WRPR;     /* 0x20 */
} FLASH_TypeDef;

#define FLASH_ACR_LATENCY   (1U << 0)   /* 0: 0<SYSCLK<=24MHz, 1: 24<SYSCLK<=48MHz */
#define FLASH_ACR_PRFTBE    (1U << 4)
#define FLASH_ACR_PRFTBS    (1U << 5)
#define FLASH_SR_BSY        (1U << 0)
#define FLASH_SR_PGERR      (1U << 2)
#define FLASH_SR_WRPRTERR   (1U << 4)
#define FLASH_SR_EOP        (1U << 5)
#define FLASH_CR_PG         (1U << 0)
#define FLASH_CR_PER        (1U << 1)
#define FLASH_CR_MER        (1U << 2)
#define FLASH_CR_STRT       (1U << 6)
#define FLASH_CR_LOCK       (1U << 7)
#define FLASH_KEY1          0x45670123UL
#define FLASH_KEY2          0xCDEF89ABUL
#define FLASH_PAGE_SIZE     2048U

#define FLASH ((FLASH_TypeDef *) FLASH_R_BASE)

/* ===========================================================================
 *  PWR - RM0091 seção 5
 * ======================================================================== */
typedef struct {
    __IO uint32_t CR;   /* 0x00 */
    __IO uint32_t CSR;  /* 0x04 */
} PWR_TypeDef;

#define PWR_CR_LPDS     (1U << 0)  /* regulador em low-power no Stop */
#define PWR_CR_PDDS     (1U << 1)  /* 1 = Standby ao entrar em deepsleep */
#define PWR_CR_CWUF     (1U << 2)
#define PWR_CR_CSBF     (1U << 3)
#define PWR_CR_DBP      (1U << 8)  /* libera escrita no backup domain */
#define PWR_CSR_WUF     (1U << 0)
#define PWR_CSR_SBF     (1U << 1)
#define PWR_CSR_EWUP1   (1U << 8)  /* WKUP1 = PA0  */
#define PWR_CSR_EWUP2   (1U << 9)  /* WKUP2 = PC13 */

#define PWR ((PWR_TypeDef *) PWR_BASE)

/* ===========================================================================
 *  GPIO - RM0091 seção 8
 * ======================================================================== */
typedef struct {
    __IO uint32_t MODER;    /* 0x00 modo: 00 in, 01 out, 10 AF, 11 analog */
    __IO uint32_t OTYPER;   /* 0x04 0 push-pull, 1 open-drain             */
    __IO uint32_t OSPEEDR;  /* 0x08 velocidade (slew rate)                */
    __IO uint32_t PUPDR;    /* 0x0C 00 nada, 01 pull-up, 10 pull-down     */
    __I  uint32_t IDR;      /* 0x10 input data                            */
    __IO uint32_t ODR;      /* 0x14 output data                           */
    __IO uint32_t BSRR;     /* 0x18 bit set/reset (atômico)               */
    __IO uint32_t LCKR;     /* 0x1C lock                                  */
    __IO uint32_t AFR[2];   /* 0x20 AFRL (pinos 0-7), 0x24 AFRH (8-15)    */
    __IO uint32_t BRR;      /* 0x28 bit reset                             */
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD ((GPIO_TypeDef *) GPIOD_BASE)
#define GPIOF ((GPIO_TypeDef *) GPIOF_BASE)

/* ===========================================================================
 *  SYSCFG e EXTI - RM0091 seções 9 e 11
 * ======================================================================== */
typedef struct {
    __IO uint32_t CFGR1;     /* 0x00 (MEM_MODE, remapeamento DMA) */
    uint32_t      RES;       /* 0x04 */
    __IO uint32_t EXTICR[4]; /* 0x08..0x14 seleção de porta da EXTI */
    __IO uint32_t CFGR2;     /* 0x18 */
} SYSCFG_TypeDef;

typedef struct {
    __IO uint32_t IMR;   /* 0x00 Interrupt mask      */
    __IO uint32_t EMR;   /* 0x04 Event mask          */
    __IO uint32_t RTSR;  /* 0x08 Rising trigger      */
    __IO uint32_t FTSR;  /* 0x0C Falling trigger     */
    __IO uint32_t SWIER; /* 0x10 Software interrupt  */
    __IO uint32_t PR;    /* 0x14 Pending (escreve 1 p/ limpar) */
} EXTI_TypeDef;

#define SYSCFG ((SYSCFG_TypeDef *) SYSCFG_BASE)
#define EXTI   ((EXTI_TypeDef *)   EXTI_BASE)

/* ===========================================================================
 *  USART (IP "nova", com ISR/ICR/TDR/RDR) - RM0091 seção 27
 * ======================================================================== */
typedef struct {
    __IO uint32_t CR1;   /* 0x00 */
    __IO uint32_t CR2;   /* 0x04 */
    __IO uint32_t CR3;   /* 0x08 */
    __IO uint32_t BRR;   /* 0x0C */
    __IO uint32_t GTPR;  /* 0x10 */
    __IO uint32_t RTOR;  /* 0x14 */
    __IO uint32_t RQR;   /* 0x18 */
    __I  uint32_t ISR;   /* 0x1C */
    __IO uint32_t ICR;   /* 0x20 */
    __I  uint32_t RDR;   /* 0x24 */
    __IO uint32_t TDR;   /* 0x28 */
} USART_TypeDef;

#define USART_CR1_UE      (1U << 0)
#define USART_CR1_RE      (1U << 2)
#define USART_CR1_TE      (1U << 3)
#define USART_CR1_IDLEIE  (1U << 4)
#define USART_CR1_RXNEIE  (1U << 5)
#define USART_CR1_TCIE    (1U << 6)
#define USART_CR1_TXEIE   (1U << 7)
#define USART_CR1_PEIE    (1U << 8)
#define USART_CR1_PS      (1U << 9)
#define USART_CR1_PCE     (1U << 10)
#define USART_CR1_M0      (1U << 12)
#define USART_CR1_OVER8   (1U << 15)
#define USART_CR1_M1      (1U << 28)
#define USART_CR2_STOP_Msk (3U << 12)
#define USART_CR3_EIE     (1U << 0)
#define USART_CR3_DMAR    (1U << 6)
#define USART_CR3_DMAT    (1U << 7)
#define USART_ISR_PE      (1U << 0)
#define USART_ISR_FE      (1U << 1)
#define USART_ISR_NE      (1U << 2)
#define USART_ISR_ORE     (1U << 3)
#define USART_ISR_IDLE    (1U << 4)
#define USART_ISR_RXNE    (1U << 5)
#define USART_ISR_TC      (1U << 6)
#define USART_ISR_TXE     (1U << 7)
#define USART_ICR_PECF    (1U << 0)
#define USART_ICR_FECF    (1U << 1)
#define USART_ICR_NCF     (1U << 2)
#define USART_ICR_ORECF   (1U << 3)
#define USART_ICR_IDLECF  (1U << 4)
#define USART_ICR_TCCF    (1U << 6)

#define USART1 ((USART_TypeDef *) USART1_BASE)
#define USART2 ((USART_TypeDef *) USART2_BASE)
#define USART3 ((USART_TypeDef *) USART3_BASE)
#define USART4 ((USART_TypeDef *) USART4_BASE)

/* ===========================================================================
 *  TIM - timers (TIM1/2/3/6/7/14/15/16/17 compartilham o mesmo layout)
 *  RM0091 seções 17 a 21
 * ======================================================================== */
typedef struct {
    __IO uint32_t CR1;    /* 0x00 */
    __IO uint32_t CR2;    /* 0x04 */
    __IO uint32_t SMCR;   /* 0x08 slave mode control */
    __IO uint32_t DIER;   /* 0x0C DMA/interrupt enable */
    __IO uint32_t SR;     /* 0x10 status */
    __IO uint32_t EGR;    /* 0x14 event generation */
    __IO uint32_t CCMR1;  /* 0x18 capture/compare mode 1 (CH1, CH2) */
    __IO uint32_t CCMR2;  /* 0x1C capture/compare mode 2 (CH3, CH4) */
    __IO uint32_t CCER;   /* 0x20 capture/compare enable */
    __IO uint32_t CNT;    /* 0x24 counter */
    __IO uint32_t PSC;    /* 0x28 prescaler */
    __IO uint32_t ARR;    /* 0x2C auto-reload */
    __IO uint32_t RCR;    /* 0x30 repetition counter (TIM1/15/16/17) */
    __IO uint32_t CCR1;   /* 0x34 */
    __IO uint32_t CCR2;   /* 0x38 */
    __IO uint32_t CCR3;   /* 0x3C */
    __IO uint32_t CCR4;   /* 0x40 */
    __IO uint32_t BDTR;   /* 0x44 break/dead-time (avançados) */
    __IO uint32_t DCR;    /* 0x48 */
    __IO uint32_t DMAR;   /* 0x4C */
    __IO uint32_t OR;     /* 0x50 */
} TIM_TypeDef;

#define TIM_CR1_CEN    (1U << 0)
#define TIM_CR1_UDIS   (1U << 1)
#define TIM_CR1_URS    (1U << 2)
#define TIM_CR1_OPM    (1U << 3)
#define TIM_CR1_DIR    (1U << 4)
#define TIM_CR1_ARPE   (1U << 7)
#define TIM_CR2_MMS_Pos 4
#define TIM_DIER_UIE   (1U << 0)
#define TIM_DIER_CC1IE (1U << 1)
#define TIM_DIER_CC2IE (1U << 2)
#define TIM_DIER_UDE   (1U << 8)
#define TIM_SR_UIF     (1U << 0)
#define TIM_SR_CC1IF   (1U << 1)
#define TIM_SR_CC2IF   (1U << 2)
#define TIM_SR_CC1OF   (1U << 9)
#define TIM_EGR_UG     (1U << 0)
#define TIM_CCER_CC1E  (1U << 0)
#define TIM_CCER_CC1P  (1U << 1)
#define TIM_CCER_CC2E  (1U << 4)
#define TIM_CCER_CC2P  (1U << 5)
#define TIM_BDTR_MOE   (1U << 15)

#define TIM1  ((TIM_TypeDef *) TIM1_BASE)
#define TIM2  ((TIM_TypeDef *) TIM2_BASE)
#define TIM3  ((TIM_TypeDef *) TIM3_BASE)
#define TIM6  ((TIM_TypeDef *) TIM6_BASE)
#define TIM7  ((TIM_TypeDef *) TIM7_BASE)
#define TIM14 ((TIM_TypeDef *) TIM14_BASE)
#define TIM15 ((TIM_TypeDef *) TIM15_BASE)
#define TIM16 ((TIM_TypeDef *) TIM16_BASE)
#define TIM17 ((TIM_TypeDef *) TIM17_BASE)

/* ===========================================================================
 *  SPI / I2S - RM0091 seção 28
 * ======================================================================== */
typedef struct {
    __IO uint32_t CR1;     /* 0x00 */
    __IO uint32_t CR2;     /* 0x04 */
    __IO uint32_t SR;      /* 0x08 */
    __IO uint32_t DR;      /* 0x0C */
    __IO uint32_t CRCPR;   /* 0x10 */
    __I  uint32_t RXCRCR;  /* 0x14 */
    __I  uint32_t TXCRCR;  /* 0x18 */
    __IO uint32_t I2SCFGR; /* 0x1C */
    __IO uint32_t I2SPR;   /* 0x20 */
} SPI_TypeDef;

#define SPI_CR1_CPHA      (1U << 0)
#define SPI_CR1_CPOL      (1U << 1)
#define SPI_CR1_MSTR      (1U << 2)
#define SPI_CR1_BR_Pos    3
#define SPI_CR1_SPE       (1U << 6)
#define SPI_CR1_LSBFIRST  (1U << 7)
#define SPI_CR1_SSI       (1U << 8)
#define SPI_CR1_SSM       (1U << 9)
#define SPI_CR1_RXONLY    (1U << 10)
#define SPI_CR1_CRCL      (1U << 11)
#define SPI_CR1_BIDIOE    (1U << 14)
#define SPI_CR1_BIDIMODE  (1U << 15)
#define SPI_CR2_RXDMAEN   (1U << 0)
#define SPI_CR2_TXDMAEN   (1U << 1)
#define SPI_CR2_SSOE      (1U << 2)
#define SPI_CR2_NSSP      (1U << 3)
#define SPI_CR2_DS_Pos    8          /* tamanho do dado: DS = bits-1 */
#define SPI_CR2_DS_Msk    (0xFU << 8)
#define SPI_CR2_FRXTH     (1U << 12) /* RXNE com 8 bits na FIFO */
#define SPI_SR_RXNE       (1U << 0)
#define SPI_SR_TXE        (1U << 1)
#define SPI_SR_CHSIDE     (1U << 2)
#define SPI_SR_UDR        (1U << 3)
#define SPI_SR_MODF       (1U << 5)
#define SPI_SR_OVR        (1U << 6)
#define SPI_SR_BSY        (1U << 7)
#define SPI_SR_FRLVL_Msk  (3U << 9)
#define SPI_SR_FTLVL_Msk  (3U << 11)
#define SPI_I2SCFGR_CHLEN    (1U << 0)
#define SPI_I2SCFGR_DATLEN_Pos 1
#define SPI_I2SCFGR_CKPOL    (1U << 3)
#define SPI_I2SCFGR_I2SSTD_Pos 4
#define SPI_I2SCFGR_I2SCFG_Pos 8
#define SPI_I2SCFGR_I2SE     (1U << 10)
#define SPI_I2SCFGR_I2SMOD   (1U << 11)
#define SPI_I2SPR_ODD        (1U << 8)
#define SPI_I2SPR_MCKOE      (1U << 9)

#define SPI1 ((SPI_TypeDef *) SPI1_BASE)
#define SPI2 ((SPI_TypeDef *) SPI2_BASE)

/* ===========================================================================
 *  I2C (IP "nova", com TIMINGR) - RM0091 seção 26
 * ======================================================================== */
typedef struct {
    __IO uint32_t CR1;      /* 0x00 */
    __IO uint32_t CR2;      /* 0x04 */
    __IO uint32_t OAR1;     /* 0x08 */
    __IO uint32_t OAR2;     /* 0x0C */
    __IO uint32_t TIMINGR;  /* 0x10 */
    __IO uint32_t TIMEOUTR; /* 0x14 */
    __I  uint32_t ISR;      /* 0x18 */
    __IO uint32_t ICR;      /* 0x1C */
    __I  uint32_t PECR;     /* 0x20 */
    __I  uint32_t RXDR;     /* 0x24 */
    __IO uint32_t TXDR;     /* 0x28 */
} I2C_TypeDef;

#define I2C_CR1_PE          (1U << 0)
#define I2C_CR1_ANFOFF      (1U << 12)
#define I2C_CR2_SADD_Pos    0
#define I2C_CR2_RD_WRN      (1U << 10)
#define I2C_CR2_START       (1U << 13)
#define I2C_CR2_STOP        (1U << 14)
#define I2C_CR2_NACK        (1U << 15)
#define I2C_CR2_NBYTES_Pos  16
#define I2C_CR2_RELOAD      (1U << 24)
#define I2C_CR2_AUTOEND     (1U << 25)
#define I2C_ISR_TXE         (1U << 0)
#define I2C_ISR_TXIS        (1U << 1)
#define I2C_ISR_RXNE        (1U << 2)
#define I2C_ISR_NACKF       (1U << 4)
#define I2C_ISR_STOPF       (1U << 5)
#define I2C_ISR_TC          (1U << 6)
#define I2C_ISR_BERR        (1U << 8)
#define I2C_ISR_ARLO        (1U << 9)
#define I2C_ISR_BUSY        (1U << 15)
#define I2C_ICR_NACKCF      (1U << 4)
#define I2C_ICR_STOPCF      (1U << 5)
#define I2C_ICR_BERRCF      (1U << 8)
#define I2C_ICR_ARLOCF      (1U << 9)

#define I2C1 ((I2C_TypeDef *) I2C1_BASE)
#define I2C2 ((I2C_TypeDef *) I2C2_BASE)

/* ===========================================================================
 *  ADC - RM0091 seção 13
 * ======================================================================== */
typedef struct {
    __IO uint32_t ISR;     /* 0x00 */
    __IO uint32_t IER;     /* 0x04 */
    __IO uint32_t CR;      /* 0x08 */
    __IO uint32_t CFGR1;   /* 0x0C */
    __IO uint32_t CFGR2;   /* 0x10 */
    __IO uint32_t SMPR;    /* 0x14 */
    uint32_t      RES1[2]; /* 0x18, 0x1C */
    __IO uint32_t TR;      /* 0x20 watchdog analógico */
    uint32_t      RES2;    /* 0x24 */
    __IO uint32_t CHSELR;  /* 0x28 seleção de canais */
    uint32_t      RES3[5]; /* 0x2C..0x3C */
    __I  uint32_t DR;      /* 0x40 */
} ADC_TypeDef;

typedef struct {
    __IO uint32_t CCR;     /* 0x308 (a partir de ADC1_BASE) */
} ADC_Common_TypeDef;

#define ADC_ISR_ADRDY    (1U << 0)
#define ADC_ISR_EOSMP    (1U << 1)
#define ADC_ISR_EOC      (1U << 2)
#define ADC_ISR_EOSEQ    (1U << 3)
#define ADC_ISR_OVR      (1U << 4)
#define ADC_CR_ADEN      (1U << 0)
#define ADC_CR_ADDIS     (1U << 1)
#define ADC_CR_ADSTART   (1U << 2)
#define ADC_CR_ADSTP     (1U << 4)
#define ADC_CR_ADCAL     (1U << 31)
#define ADC_CFGR1_DMAEN  (1U << 0)
#define ADC_CFGR1_DMACFG (1U << 1)  /* 1 = DMA circular */
#define ADC_CFGR1_SCANDIR (1U << 2)
#define ADC_CFGR1_RES_Pos 3
#define ADC_CFGR1_ALIGN  (1U << 5)
#define ADC_CFGR1_OVRMOD (1U << 12)
#define ADC_CFGR1_CONT   (1U << 13)
#define ADC_CFGR2_CKMODE_Pos 30
#define ADC_CCR_VREFEN   (1U << 22)
#define ADC_CCR_TSEN     (1U << 23)

#define ADC1       ((ADC_TypeDef *) ADC1_BASE)
#define ADC_COMMON ((ADC_Common_TypeDef *) ADC_COMMON_BASE)

/* ===========================================================================
 *  DAC - RM0091 seção 14
 * ======================================================================== */
typedef struct {
    __IO uint32_t CR;       /* 0x00 */
    __IO uint32_t SWTRIGR;  /* 0x04 */
    __IO uint32_t DHR12R1;  /* 0x08 */
    __IO uint32_t DHR12L1;  /* 0x0C */
    __IO uint32_t DHR8R1;   /* 0x10 */
    __IO uint32_t DHR12R2;  /* 0x14 */
    __IO uint32_t DHR12L2;  /* 0x18 */
    __IO uint32_t DHR8R2;   /* 0x1C */
    __IO uint32_t DHR12RD;  /* 0x20 */
    __IO uint32_t DHR12LD;  /* 0x24 */
    __IO uint32_t DHR8RD;   /* 0x28 */
    __I  uint32_t DOR1;     /* 0x2C */
    __I  uint32_t DOR2;     /* 0x30 */
    __IO uint32_t SR;       /* 0x34 */
} DAC_TypeDef;

#define DAC_CR_EN1        (1U << 0)
#define DAC_CR_BOFF1      (1U << 1)
#define DAC_CR_TEN1       (1U << 2)
#define DAC_CR_TSEL1_Pos  3
#define DAC_CR_WAVE1_Pos  6
#define DAC_CR_MAMP1_Pos  8
#define DAC_CR_DMAEN1     (1U << 12)
#define DAC_SR_DMAUDR1    (1U << 13)

#define DAC ((DAC_TypeDef *) DAC_BASE)

/* ===========================================================================
 *  DMA (controlador por CANAIS) - RM0091 seção 10
 * ======================================================================== */
typedef struct {
    __IO uint32_t CCR;    /* configuração do canal */
    __IO uint32_t CNDTR;  /* número de transferências */
    __IO uint32_t CPAR;   /* endereço do periférico */
    __IO uint32_t CMAR;   /* endereço da memória */
    uint32_t      RES;
} DMA_Channel_TypeDef;

typedef struct {
    __I  uint32_t ISR;    /* 0x00 */
    __IO uint32_t IFCR;   /* 0x04 */
    DMA_Channel_TypeDef CH[7]; /* CH[0] = canal 1 (0x08), CH[1] = canal 2 (0x1C)... */
} DMA_TypeDef;

#define DMA_CCR_EN        (1U << 0)
#define DMA_CCR_TCIE      (1U << 1)
#define DMA_CCR_HTIE      (1U << 2)
#define DMA_CCR_TEIE      (1U << 3)
#define DMA_CCR_DIR       (1U << 4)  /* 1 = memória -> periférico */
#define DMA_CCR_CIRC      (1U << 5)
#define DMA_CCR_PINC      (1U << 6)
#define DMA_CCR_MINC      (1U << 7)
#define DMA_CCR_PSIZE_Pos 8
#define DMA_CCR_MSIZE_Pos 10
#define DMA_CCR_PL_Pos    12
#define DMA_CCR_MEM2MEM   (1U << 14)
/* flags no ISR/IFCR: canal n (1..7) ocupa os bits 4*(n-1) .. 4*(n-1)+3 */
#define DMA_GIF(n)   (1U << (4U * ((n) - 1U) + 0U))
#define DMA_TCIF(n)  (1U << (4U * ((n) - 1U) + 1U))
#define DMA_HTIF(n)  (1U << (4U * ((n) - 1U) + 2U))
#define DMA_TEIF(n)  (1U << (4U * ((n) - 1U) + 3U))

#define DMA1 ((DMA_TypeDef *) DMA1_BASE)

/* ===========================================================================
 *  bxCAN - RM0091 seção 29
 * ======================================================================== */
typedef struct {
    __IO uint32_t TIR;   /* identificador + TXRQ */
    __IO uint32_t TDTR;  /* DLC                  */
    __IO uint32_t TDLR;  /* dados bytes 0..3     */
    __IO uint32_t TDHR;  /* dados bytes 4..7     */
} CAN_TxMailBox_TypeDef;

typedef struct {
    __I  uint32_t RIR;
    __I  uint32_t RDTR;
    __I  uint32_t RDLR;
    __I  uint32_t RDHR;
} CAN_FIFOMailBox_TypeDef;

typedef struct {
    __IO uint32_t FR1;
    __IO uint32_t FR2;
} CAN_FilterRegister_TypeDef;

typedef struct {
    __IO uint32_t MCR;     /* 0x000 */
    __IO uint32_t MSR;     /* 0x004 */
    __IO uint32_t TSR;     /* 0x008 */
    __IO uint32_t RF0R;    /* 0x00C */
    __IO uint32_t RF1R;    /* 0x010 */
    __IO uint32_t IER;     /* 0x014 */
    __IO uint32_t ESR;     /* 0x018 */
    __IO uint32_t BTR;     /* 0x01C */
    uint32_t      RES0[88];/* 0x020..0x17F */
    CAN_TxMailBox_TypeDef   sTxMailBox[3];  /* 0x180 */
    CAN_FIFOMailBox_TypeDef sFIFOMailBox[2];/* 0x1B0 */
    uint32_t      RES1[12];/* 0x1D0..0x1FF */
    __IO uint32_t FMR;     /* 0x200 */
    __IO uint32_t FM1R;    /* 0x204 */
    uint32_t      RES2;    /* 0x208 */
    __IO uint32_t FS1R;    /* 0x20C */
    uint32_t      RES3;    /* 0x210 */
    __IO uint32_t FFA1R;   /* 0x214 */
    uint32_t      RES4;    /* 0x218 */
    __IO uint32_t FA1R;    /* 0x21C */
    uint32_t      RES5[8]; /* 0x220..0x23F */
    CAN_FilterRegister_TypeDef sFilterRegister[14]; /* 0x240 */
} CAN_TypeDef;

#define CAN ((CAN_TypeDef *) CAN_BASE)

/* ===========================================================================
 *  IWDG / WWDG - RM0091 seções 24 e 25
 * ======================================================================== */
typedef struct {
    __IO uint32_t KR;    /* 0x00 key        */
    __IO uint32_t PR;    /* 0x04 prescaler  */
    __IO uint32_t RLR;   /* 0x08 reload     */
    __I  uint32_t SR;    /* 0x0C status     */
    __IO uint32_t WINR;  /* 0x10 window     */
} IWDG_TypeDef;

typedef struct {
    __IO uint32_t CR;    /* 0x00 */
    __IO uint32_t CFR;   /* 0x04 */
    __IO uint32_t SR;    /* 0x08 */
} WWDG_TypeDef;

#define IWDG ((IWDG_TypeDef *) IWDG_BASE)
#define WWDG ((WWDG_TypeDef *) WWDG_BASE)

/* ===========================================================================
 *  RTC - RM0091 seção 23
 * ======================================================================== */
typedef struct {
    __IO uint32_t TR;       /* 0x00 time (BCD)       */
    __IO uint32_t DR;       /* 0x04 date (BCD)       */
    __IO uint32_t CR;       /* 0x08 */
    __IO uint32_t ISR;      /* 0x0C */
    __IO uint32_t PRER;     /* 0x10 prescalers       */
    __IO uint32_t WUTR;     /* 0x14 wakeup timer     */
    uint32_t      RES0;     /* 0x18 */
    __IO uint32_t ALRMAR;   /* 0x1C alarm A          */
    uint32_t      RES1;     /* 0x20 */
    __IO uint32_t WPR;      /* 0x24 write protection */
    __I  uint32_t SSR;      /* 0x28 sub-second       */
    __IO uint32_t SHIFTR;   /* 0x2C */
    __I  uint32_t TSTR;     /* 0x30 */
    __I  uint32_t TSDR;     /* 0x34 */
    __I  uint32_t TSSSR;    /* 0x38 */
    __IO uint32_t CALR;     /* 0x3C */
    __IO uint32_t TAFCR;    /* 0x40 */
    __IO uint32_t ALRMASSR; /* 0x44 */
    uint32_t      RES2[2];  /* 0x48, 0x4C */
    __IO uint32_t BKPR[5];  /* 0x50 backup registers (20 bytes) */
} RTC_TypeDef;

#define RTC_CR_FMT      (1U << 6)
#define RTC_CR_ALRAE    (1U << 8)
#define RTC_CR_WUTE     (1U << 10)
#define RTC_CR_ALRAIE   (1U << 12)
#define RTC_CR_WUTIE    (1U << 14)
#define RTC_ISR_ALRAWF  (1U << 0)
#define RTC_ISR_WUTWF   (1U << 2)
#define RTC_ISR_INITS   (1U << 4)
#define RTC_ISR_RSF     (1U << 5)
#define RTC_ISR_INITF   (1U << 6)
#define RTC_ISR_INIT    (1U << 7)
#define RTC_ISR_ALRAF   (1U << 8)
#define RTC_ISR_WUTF    (1U << 10)

#define RTC ((RTC_TypeDef *) RTC_BASE)

/* ===========================================================================
 *  DBGMCU
 * ======================================================================== */
typedef struct {
    __I  uint32_t IDCODE;   /* 0x00 */
    __IO uint32_t CR;       /* 0x04 */
    __IO uint32_t APB1FZ;   /* 0x08 */
    __IO uint32_t APB2FZ;   /* 0x0C */
} DBGMCU_TypeDef;

#define DBGMCU ((DBGMCU_TypeDef *) DBGMCU_BASE)

/* ===========================================================================
 *  Funções inline utilitárias do núcleo (equivalentes às do CMSIS)
 * ======================================================================== */
static inline void __enable_irq(void)  { __asm volatile ("cpsie i" ::: "memory"); }
static inline void __disable_irq(void) { __asm volatile ("cpsid i" ::: "memory"); }
static inline void __WFI(void)         { __asm volatile ("wfi"); }
static inline void __WFE(void)         { __asm volatile ("wfe"); }
static inline void __NOP(void)         { __asm volatile ("nop"); }
static inline void __DSB(void)         { __asm volatile ("dsb 0xF" ::: "memory"); }
static inline void __ISB(void)         { __asm volatile ("isb 0xF" ::: "memory"); }

static inline void NVIC_EnableIRQ(IRQn_Type irq)
{
    NVIC->ISER[0] = 1U << ((uint32_t)irq & 0x1FU);
}

static inline void NVIC_DisableIRQ(IRQn_Type irq)
{
    NVIC->ICER[0] = 1U << ((uint32_t)irq & 0x1FU);
    __DSB();
    __ISB();
}

static inline void NVIC_ClearPendingIRQ(IRQn_Type irq)
{
    NVIC->ICPR[0] = 1U << ((uint32_t)irq & 0x1FU);
}

/* prio: 0 (mais alta) .. 3 (mais baixa). No M0 o IPR só aceita acesso de 32 bits. */
static inline void NVIC_SetPriority(IRQn_Type irq, uint32_t prio)
{
    uint32_t n     = (uint32_t)irq >> 2;          /* qual registrador IPR   */
    uint32_t shift = ((uint32_t)irq & 3U) * 8U;   /* qual byte dentro dele  */
    uint32_t v     = NVIC->IPR[n];
    v &= ~(0xFFU << shift);
    v |= ((prio << (8U - NVIC_PRIO_BITS)) & 0xFFU) << shift;
    NVIC->IPR[n] = v;
}

static inline void NVIC_SystemReset(void)
{
    __DSB();
    SCB->AIRCR = SCB_AIRCR_VECTKEY | SCB_AIRCR_SYSRESETREQ;
    __DSB();
    for (;;) { }
}

#endif /* STM32F0_H */
