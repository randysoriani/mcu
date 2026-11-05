/*
 * stm32f4.h - Definições de registradores do STM32F446xE (Cortex-M4F)
 *
 * Escrito à mão para o curso, a partir do Reference Manual RM0390 e do
 * datasheet DS10693. NÃO usa CMSIS, HAL ou LL.
 *
 * A maior parte dos periféricos é idêntica nos outros F4 (F401/F411/F407/F429).
 * Diferenças relevantes estão anotadas nos README de cada módulo.
 */
#ifndef STM32F4_H
#define STM32F4_H

#include <stdint.h>

#define __IO volatile
#define __I  volatile const

/* ===========================================================================
 *  Mapa de memória (RM0390, seção 2.2)
 * ======================================================================== */
#define FLASH_BASE      0x08000000UL   /* 512 KB de Flash                       */
#define SRAM1_BASE      0x20000000UL   /* 112 KB                                */
#define SRAM2_BASE      0x2001C000UL   /* 16 KB (contíguo à SRAM1 -> 128 KB)    */
#define BKPSRAM_BASE    0x40024000UL   /* 4 KB de SRAM no backup domain         */
#define SYSMEM_BASE     0x1FFF0000UL   /* bootloader de sistema (ROM da ST)     */
#define PERIPH_BASE     0x40000000UL
#define PERIPH_BB_BASE  0x42000000UL   /* alias de bit-banding dos periféricos  */
#define SRAM_BB_BASE    0x22000000UL   /* alias de bit-banding da SRAM          */

/* APB1 (máx. 45 MHz) */
#define TIM2_BASE       0x40000000UL
#define TIM3_BASE       0x40000400UL
#define TIM4_BASE       0x40000800UL
#define TIM5_BASE       0x40000C00UL
#define TIM6_BASE       0x40001000UL
#define TIM7_BASE       0x40001400UL
#define TIM12_BASE      0x40001800UL
#define TIM13_BASE      0x40001C00UL
#define TIM14_BASE      0x40002000UL
#define RTC_BASE        0x40002800UL
#define WWDG_BASE       0x40002C00UL
#define IWDG_BASE       0x40003000UL
#define SPI2_BASE       0x40003800UL
#define SPI3_BASE       0x40003C00UL
#define USART2_BASE     0x40004400UL
#define USART3_BASE     0x40004800UL
#define UART4_BASE      0x40004C00UL
#define UART5_BASE      0x40005000UL
#define I2C1_BASE       0x40005400UL
#define I2C2_BASE       0x40005800UL
#define I2C3_BASE       0x40005C00UL
#define CAN1_BASE       0x40006400UL
#define CAN2_BASE       0x40006800UL
#define PWR_BASE        0x40007000UL
#define DAC_BASE        0x40007400UL

/* APB2 (máx. 90 MHz) */
#define TIM1_BASE       0x40010000UL
#define TIM8_BASE       0x40010400UL
#define USART1_BASE     0x40011000UL
#define USART6_BASE     0x40011400UL
#define ADC1_BASE       0x40012000UL
#define ADC2_BASE       0x40012100UL
#define ADC3_BASE       0x40012200UL
#define ADC_COMMON_BASE 0x40012300UL
#define SPI1_BASE       0x40013000UL
#define SPI4_BASE       0x40013400UL
#define SYSCFG_BASE     0x40013800UL
#define EXTI_BASE       0x40013C00UL
#define TIM9_BASE       0x40014000UL
#define TIM10_BASE      0x40014400UL
#define TIM11_BASE      0x40014800UL

/* AHB1 (máx. 180 MHz) */
#define GPIOA_BASE      0x40020000UL
#define GPIOB_BASE      0x40020400UL
#define GPIOC_BASE      0x40020800UL
#define GPIOD_BASE      0x40020C00UL
#define GPIOE_BASE      0x40021000UL
#define GPIOF_BASE      0x40021400UL
#define GPIOG_BASE      0x40021800UL
#define GPIOH_BASE      0x40021C00UL
#define CRC_BASE        0x40023000UL
#define RCC_BASE        0x40023800UL
#define FLASH_R_BASE    0x40023C00UL
#define DMA1_BASE       0x40026000UL
#define DMA2_BASE       0x40026400UL

#define DBGMCU_BASE     0xE0042000UL

/* Núcleo Cortex-M4 */
#define SCS_BASE        0xE000E000UL
#define SYSTICK_BASE    0xE000E010UL
#define NVIC_BASE       0xE000E100UL
#define SCB_BASE        0xE000ED00UL

/* Valores de calibração gravados na fábrica */
#define UID_BASE         0x1FFF7A10UL
#define FLASHSIZE_BASE   0x1FFF7A22UL
#define VREFINT_CAL_ADDR ((const uint16_t *)0x1FFF7A2AUL) /* @30C, VDDA=3.3V */
#define TS_CAL1_ADDR     ((const uint16_t *)0x1FFF7A2CUL) /* @30C            */
#define TS_CAL2_ADDR     ((const uint16_t *)0x1FFF7A2EUL) /* @110C           */

/* ===========================================================================
 *  Núcleo: SysTick, NVIC, SCB
 * ======================================================================== */
typedef struct {
    __IO uint32_t CTRL;
    __IO uint32_t LOAD;
    __IO uint32_t VAL;
    __I  uint32_t CALIB;
} SysTick_TypeDef;

#define SysTick_CTRL_ENABLE     (1U << 0)
#define SysTick_CTRL_TICKINT    (1U << 1)
#define SysTick_CTRL_CLKSOURCE  (1U << 2)
#define SysTick_CTRL_COUNTFLAG  (1U << 16)

typedef struct {
    __IO uint32_t ISER[8];   /* 0x000 */
    uint32_t      RES0[24];
    __IO uint32_t ICER[8];   /* 0x080 */
    uint32_t      RES1[24];
    __IO uint32_t ISPR[8];   /* 0x100 */
    uint32_t      RES2[24];
    __IO uint32_t ICPR[8];   /* 0x180 */
    uint32_t      RES3[24];
    __IO uint32_t IABR[8];   /* 0x200 active bit */
    uint32_t      RES4[56];
    __IO uint8_t  IP[240];   /* 0x300 prioridade (1 byte por IRQ; M4 aceita byte) */
    uint32_t      RES5[644];
    __IO uint32_t STIR;      /* 0xE00 software trigger */
} NVIC_TypeDef;

typedef struct {
    __I  uint32_t CPUID;   /* 0x00 */
    __IO uint32_t ICSR;    /* 0x04 */
    __IO uint32_t VTOR;    /* 0x08 vector table offset */
    __IO uint32_t AIRCR;   /* 0x0C */
    __IO uint32_t SCR;     /* 0x10 */
    __IO uint32_t CCR;     /* 0x14 */
    __IO uint8_t  SHP[12]; /* 0x18 prioridades das exceções do sistema */
    __IO uint32_t SHCSR;   /* 0x24 */
    __IO uint32_t CFSR;    /* 0x28 configurable fault status */
    __IO uint32_t HFSR;    /* 0x2C hard fault status */
    __IO uint32_t DFSR;    /* 0x30 */
    __IO uint32_t MMFAR;   /* 0x34 */
    __IO uint32_t BFAR;    /* 0x38 */
    __IO uint32_t AFSR;    /* 0x3C */
    __I  uint32_t PFR[2];  /* 0x40 */
    __I  uint32_t DFR;     /* 0x48 */
    __I  uint32_t ADR;     /* 0x4C */
    __I  uint32_t MMFR[4]; /* 0x50 */
    __I  uint32_t ISAR[5]; /* 0x60 */
    uint32_t      RES0[5]; /* 0x74 */
    __IO uint32_t CPACR;   /* 0x88 coprocessor access control (FPU) */
} SCB_TypeDef;

#define SCB_SCR_SLEEPONEXIT   (1U << 1)
#define SCB_SCR_SLEEPDEEP     (1U << 2)
#define SCB_AIRCR_VECTKEY     (0x05FAU << 16)
#define SCB_AIRCR_SYSRESETREQ (1U << 2)
#define SCB_SHCSR_MEMFAULTENA (1U << 16)
#define SCB_SHCSR_BUSFAULTENA (1U << 17)
#define SCB_SHCSR_USGFAULTENA (1U << 18)

#define SysTick ((SysTick_TypeDef *) SYSTICK_BASE)
#define NVIC    ((NVIC_TypeDef *)    NVIC_BASE)
#define SCB     ((SCB_TypeDef *)     SCB_BASE)

/* ===========================================================================
 *  Números de interrupção (RM0390, tabela 38)
 * ======================================================================== */
typedef enum {
    WWDG_IRQn              = 0,
    PVD_IRQn               = 1,
    TAMP_STAMP_IRQn        = 2,
    RTC_WKUP_IRQn          = 3,
    FLASH_IRQn             = 4,
    RCC_IRQn               = 5,
    EXTI0_IRQn             = 6,
    EXTI1_IRQn             = 7,
    EXTI2_IRQn             = 8,
    EXTI3_IRQn             = 9,
    EXTI4_IRQn             = 10,
    DMA1_Stream0_IRQn      = 11,
    DMA1_Stream1_IRQn      = 12,
    DMA1_Stream2_IRQn      = 13,
    DMA1_Stream3_IRQn      = 14,
    DMA1_Stream4_IRQn      = 15,
    DMA1_Stream5_IRQn      = 16,
    DMA1_Stream6_IRQn      = 17,
    ADC_IRQn               = 18,
    CAN1_TX_IRQn           = 19,
    CAN1_RX0_IRQn          = 20,
    CAN1_RX1_IRQn          = 21,
    CAN1_SCE_IRQn          = 22,
    EXTI9_5_IRQn           = 23,
    TIM1_BRK_TIM9_IRQn     = 24,
    TIM1_UP_TIM10_IRQn     = 25,
    TIM1_TRG_COM_TIM11_IRQn= 26,
    TIM1_CC_IRQn           = 27,
    TIM2_IRQn              = 28,
    TIM3_IRQn              = 29,
    TIM4_IRQn              = 30,
    I2C1_EV_IRQn           = 31,
    I2C1_ER_IRQn           = 32,
    I2C2_EV_IRQn           = 33,
    I2C2_ER_IRQn           = 34,
    SPI1_IRQn              = 35,
    SPI2_IRQn              = 36,
    USART1_IRQn            = 37,
    USART2_IRQn            = 38,
    USART3_IRQn            = 39,
    EXTI15_10_IRQn         = 40,
    RTC_Alarm_IRQn         = 41,
    OTG_FS_WKUP_IRQn       = 42,
    TIM8_BRK_TIM12_IRQn    = 43,
    TIM8_UP_TIM13_IRQn     = 44,
    TIM8_TRG_COM_TIM14_IRQn= 45,
    TIM8_CC_IRQn           = 46,
    DMA1_Stream7_IRQn      = 47,
    FMC_IRQn               = 48,
    SDIO_IRQn              = 49,
    TIM5_IRQn              = 50,
    SPI3_IRQn              = 51,
    UART4_IRQn             = 52,
    UART5_IRQn             = 53,
    TIM6_DAC_IRQn          = 54,
    TIM7_IRQn              = 55,
    DMA2_Stream0_IRQn      = 56,
    DMA2_Stream1_IRQn      = 57,
    DMA2_Stream2_IRQn      = 58,
    DMA2_Stream3_IRQn      = 59,
    DMA2_Stream4_IRQn      = 60,
    CAN2_TX_IRQn           = 63,
    CAN2_RX0_IRQn          = 64,
    CAN2_RX1_IRQn          = 65,
    CAN2_SCE_IRQn          = 66,
    OTG_FS_IRQn            = 67,
    DMA2_Stream5_IRQn      = 68,
    DMA2_Stream6_IRQn      = 69,
    DMA2_Stream7_IRQn      = 70,
    USART6_IRQn            = 71,
    I2C3_EV_IRQn           = 72,
    I2C3_ER_IRQn           = 73,
    FPU_IRQn               = 81,
    SPI4_IRQn              = 84,
    SAI1_IRQn              = 87,
    SAI2_IRQn              = 91,
    QUADSPI_IRQn           = 92,
    CEC_IRQn               = 93,
    SPDIF_RX_IRQn          = 94,
    FMPI2C1_EV_IRQn        = 95,
    FMPI2C1_ER_IRQn        = 96,
} IRQn_Type;

#define NVIC_PRIO_BITS 4   /* STM32F4: 16 níveis de prioridade (bits [7:4]) */

/* ===========================================================================
 *  RCC - RM0390 seção 6
 * ======================================================================== */
typedef struct {
    __IO uint32_t CR;          /* 0x00 */
    __IO uint32_t PLLCFGR;     /* 0x04 */
    __IO uint32_t CFGR;        /* 0x08 */
    __IO uint32_t CIR;         /* 0x0C */
    __IO uint32_t AHB1RSTR;    /* 0x10 */
    __IO uint32_t AHB2RSTR;    /* 0x14 */
    __IO uint32_t AHB3RSTR;    /* 0x18 */
    uint32_t      RES0;        /* 0x1C */
    __IO uint32_t APB1RSTR;    /* 0x20 */
    __IO uint32_t APB2RSTR;    /* 0x24 */
    uint32_t      RES1[2];     /* 0x28 */
    __IO uint32_t AHB1ENR;     /* 0x30 */
    __IO uint32_t AHB2ENR;     /* 0x34 */
    __IO uint32_t AHB3ENR;     /* 0x38 */
    uint32_t      RES2;        /* 0x3C */
    __IO uint32_t APB1ENR;     /* 0x40 */
    __IO uint32_t APB2ENR;     /* 0x44 */
    uint32_t      RES3[2];     /* 0x48 */
    __IO uint32_t AHB1LPENR;   /* 0x50 */
    __IO uint32_t AHB2LPENR;   /* 0x54 */
    __IO uint32_t AHB3LPENR;   /* 0x58 */
    uint32_t      RES4;        /* 0x5C */
    __IO uint32_t APB1LPENR;   /* 0x60 */
    __IO uint32_t APB2LPENR;   /* 0x64 */
    uint32_t      RES5[2];     /* 0x68 */
    __IO uint32_t BDCR;        /* 0x70 */
    __IO uint32_t CSR;         /* 0x74 */
    uint32_t      RES6[2];     /* 0x78 */
    __IO uint32_t SSCGR;       /* 0x80 */
    __IO uint32_t PLLI2SCFGR;  /* 0x84 */
    __IO uint32_t PLLSAICFGR;  /* 0x88 */
    __IO uint32_t DCKCFGR;     /* 0x8C */
    __IO uint32_t CKGATENR;    /* 0x90 */
    __IO uint32_t DCKCFGR2;    /* 0x94 */
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
#define RCC_CR_PLLI2SON     (1U << 26)
#define RCC_CR_PLLI2SRDY    (1U << 27)
/* RCC_PLLCFGR */
#define RCC_PLLCFGR_PLLM_Pos  0
#define RCC_PLLCFGR_PLLN_Pos  6
#define RCC_PLLCFGR_PLLP_Pos  16
#define RCC_PLLCFGR_PLLSRC_HSE (1U << 22)
#define RCC_PLLCFGR_PLLQ_Pos  24
#define RCC_PLLCFGR_PLLR_Pos  28
/* RCC_PLLI2SCFGR */
#define RCC_PLLI2SCFGR_M_Pos  0
#define RCC_PLLI2SCFGR_N_Pos  6
#define RCC_PLLI2SCFGR_P_Pos  16
#define RCC_PLLI2SCFGR_Q_Pos  24
#define RCC_PLLI2SCFGR_R_Pos  28
/* RCC_CFGR */
#define RCC_CFGR_SW_Msk     (3U << 0)
#define RCC_CFGR_SW_HSI     (0U << 0)
#define RCC_CFGR_SW_HSE     (1U << 0)
#define RCC_CFGR_SW_PLL     (2U << 0)
#define RCC_CFGR_SWS_Msk    (3U << 2)
#define RCC_CFGR_SWS_PLL    (2U << 2)
#define RCC_CFGR_HPRE_Pos   4
#define RCC_CFGR_HPRE_Msk   (0xFU << 4)
#define RCC_CFGR_PPRE1_Pos  10
#define RCC_CFGR_PPRE1_Msk  (7U << 10)
#define RCC_CFGR_PPRE2_Pos  13
#define RCC_CFGR_PPRE2_Msk  (7U << 13)
#define RCC_CFGR_MCO1_Pos   21
#define RCC_CFGR_MCO1PRE_Pos 24
#define RCC_CFGR_MCO2PRE_Pos 27
#define RCC_CFGR_MCO2_Pos   30
/* RCC_AHB1ENR */
#define RCC_AHB1ENR_GPIOAEN (1U << 0)
#define RCC_AHB1ENR_GPIOBEN (1U << 1)
#define RCC_AHB1ENR_GPIOCEN (1U << 2)
#define RCC_AHB1ENR_GPIODEN (1U << 3)
#define RCC_AHB1ENR_GPIOEEN (1U << 4)
#define RCC_AHB1ENR_GPIOHEN (1U << 7)
#define RCC_AHB1ENR_CRCEN   (1U << 12)
#define RCC_AHB1ENR_BKPSRAMEN (1U << 18)
#define RCC_AHB1ENR_DMA1EN  (1U << 21)
#define RCC_AHB1ENR_DMA2EN  (1U << 22)
/* RCC_APB1ENR */
#define RCC_APB1ENR_TIM2EN  (1U << 0)
#define RCC_APB1ENR_TIM3EN  (1U << 1)
#define RCC_APB1ENR_TIM4EN  (1U << 2)
#define RCC_APB1ENR_TIM5EN  (1U << 3)
#define RCC_APB1ENR_TIM6EN  (1U << 4)
#define RCC_APB1ENR_TIM7EN  (1U << 5)
#define RCC_APB1ENR_WWDGEN  (1U << 11)
#define RCC_APB1ENR_SPI2EN  (1U << 14)
#define RCC_APB1ENR_SPI3EN  (1U << 15)
#define RCC_APB1ENR_USART2EN (1U << 17)
#define RCC_APB1ENR_USART3EN (1U << 18)
#define RCC_APB1ENR_I2C1EN  (1U << 21)
#define RCC_APB1ENR_I2C2EN  (1U << 22)
#define RCC_APB1ENR_I2C3EN  (1U << 23)
#define RCC_APB1ENR_CAN1EN  (1U << 25)
#define RCC_APB1ENR_CAN2EN  (1U << 26)
#define RCC_APB1ENR_PWREN   (1U << 28)
#define RCC_APB1ENR_DACEN   (1U << 29)
/* RCC_APB2ENR */
#define RCC_APB2ENR_TIM1EN  (1U << 0)
#define RCC_APB2ENR_TIM8EN  (1U << 1)
#define RCC_APB2ENR_USART1EN (1U << 4)
#define RCC_APB2ENR_USART6EN (1U << 5)
#define RCC_APB2ENR_ADC1EN  (1U << 8)
#define RCC_APB2ENR_SPI1EN  (1U << 12)
#define RCC_APB2ENR_SYSCFGEN (1U << 14)
#define RCC_APB2ENR_TIM9EN  (1U << 16)
/* RCC_BDCR */
#define RCC_BDCR_LSEON      (1U << 0)
#define RCC_BDCR_LSERDY     (1U << 1)
#define RCC_BDCR_LSEBYP     (1U << 2)
#define RCC_BDCR_RTCSEL_Msk (3U << 8)
#define RCC_BDCR_RTCSEL_LSE (1U << 8)
#define RCC_BDCR_RTCSEL_LSI (2U << 8)
#define RCC_BDCR_RTCEN      (1U << 15)
#define RCC_BDCR_BDRST      (1U << 16)
/* RCC_CSR */
#define RCC_CSR_LSION       (1U << 0)
#define RCC_CSR_LSIRDY      (1U << 1)
#define RCC_CSR_RMVF        (1U << 24)
#define RCC_CSR_BORRSTF     (1U << 25)
#define RCC_CSR_PINRSTF     (1U << 26)
#define RCC_CSR_PORRSTF     (1U << 27)
#define RCC_CSR_SFTRSTF     (1U << 28)
#define RCC_CSR_IWDGRSTF    (1U << 29)
#define RCC_CSR_WWDGRSTF    (1U << 30)
#define RCC_CSR_LPWRRSTF    (1U << 31)
/* RCC_DCKCFGR */
#define RCC_DCKCFGR_I2S1SRC_Pos 21  /* fonte do I2S do APB1 (I2S2/I2S3) */
#define RCC_DCKCFGR_I2S2SRC_Pos 23  /* fonte do I2S do APB2 (I2S1/I2S4) */

#define RCC ((RCC_TypeDef *) RCC_BASE)

/* ===========================================================================
 *  FLASH (interface) - RM0390 seção 3
 * ======================================================================== */
typedef struct {
    __IO uint32_t ACR;     /* 0x00 */
    __IO uint32_t KEYR;    /* 0x04 */
    __IO uint32_t OPTKEYR; /* 0x08 */
    __IO uint32_t SR;      /* 0x0C */
    __IO uint32_t CR;      /* 0x10 */
    __IO uint32_t OPTCR;   /* 0x14 */
} FLASH_TypeDef;

#define FLASH_ACR_LATENCY_Msk (0xFU << 0)
#define FLASH_ACR_PRFTEN    (1U << 8)
#define FLASH_ACR_ICEN      (1U << 9)
#define FLASH_ACR_DCEN      (1U << 10)
#define FLASH_ACR_ICRST     (1U << 11)
#define FLASH_ACR_DCRST     (1U << 12)
#define FLASH_SR_EOP        (1U << 0)
#define FLASH_SR_OPERR      (1U << 1)
#define FLASH_SR_WRPERR     (1U << 4)
#define FLASH_SR_PGAERR     (1U << 5)
#define FLASH_SR_PGPERR     (1U << 6)
#define FLASH_SR_PGSERR     (1U << 7)
#define FLASH_SR_BSY        (1U << 16)
#define FLASH_CR_PG         (1U << 0)
#define FLASH_CR_SER        (1U << 1)
#define FLASH_CR_MER        (1U << 2)
#define FLASH_CR_SNB_Pos    3
#define FLASH_CR_PSIZE_Pos  8
#define FLASH_CR_STRT       (1U << 16)
#define FLASH_CR_LOCK       (1U << 31)
#define FLASH_KEY1          0x45670123UL
#define FLASH_KEY2          0xCDEF89ABUL

#define FLASH ((FLASH_TypeDef *) FLASH_R_BASE)

/* ===========================================================================
 *  PWR - RM0390 seção 5
 * ======================================================================== */
typedef struct {
    __IO uint32_t CR;   /* 0x00 */
    __IO uint32_t CSR;  /* 0x04 */
} PWR_TypeDef;

#define PWR_CR_LPDS      (1U << 0)
#define PWR_CR_PDDS      (1U << 1)
#define PWR_CR_CWUF      (1U << 2)
#define PWR_CR_CSBF      (1U << 3)
#define PWR_CR_DBP       (1U << 8)
#define PWR_CR_FPDS      (1U << 9)
#define PWR_CR_VOS_Pos   14
#define PWR_CR_VOS_Msk   (3U << 14)
#define PWR_CR_ODEN      (1U << 16)
#define PWR_CR_ODSWEN    (1U << 17)
#define PWR_CSR_WUF      (1U << 0)
#define PWR_CSR_SBF      (1U << 1)
#define PWR_CSR_EWUP2    (1U << 7)   /* WKUP2 = PC13 (F446) */
#define PWR_CSR_EWUP1    (1U << 8)   /* WKUP1 = PA0         */
#define PWR_CSR_VOSRDY   (1U << 14)
#define PWR_CSR_ODRDY    (1U << 16)
#define PWR_CSR_ODSWRDY  (1U << 17)

#define PWR ((PWR_TypeDef *) PWR_BASE)

/* ===========================================================================
 *  GPIO - RM0390 seção 7
 * ======================================================================== */
typedef struct {
    __IO uint32_t MODER;    /* 0x00 */
    __IO uint32_t OTYPER;   /* 0x04 */
    __IO uint32_t OSPEEDR;  /* 0x08 */
    __IO uint32_t PUPDR;    /* 0x0C */
    __I  uint32_t IDR;      /* 0x10 */
    __IO uint32_t ODR;      /* 0x14 */
    __IO uint32_t BSRR;     /* 0x18 */
    __IO uint32_t LCKR;     /* 0x1C */
    __IO uint32_t AFR[2];   /* 0x20, 0x24 */
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD ((GPIO_TypeDef *) GPIOD_BASE)
#define GPIOH ((GPIO_TypeDef *) GPIOH_BASE)

/* ===========================================================================
 *  SYSCFG e EXTI
 * ======================================================================== */
typedef struct {
    __IO uint32_t MEMRMP;    /* 0x00 */
    __IO uint32_t PMC;       /* 0x04 */
    __IO uint32_t EXTICR[4]; /* 0x08..0x14 */
    uint32_t      RES[2];    /* 0x18 */
    __IO uint32_t CMPCR;     /* 0x20 */
} SYSCFG_TypeDef;

typedef struct {
    __IO uint32_t IMR;
    __IO uint32_t EMR;
    __IO uint32_t RTSR;
    __IO uint32_t FTSR;
    __IO uint32_t SWIER;
    __IO uint32_t PR;
} EXTI_TypeDef;

#define SYSCFG ((SYSCFG_TypeDef *) SYSCFG_BASE)
#define EXTI   ((EXTI_TypeDef *)   EXTI_BASE)

/* ===========================================================================
 *  USART (IP "clássica", com SR/DR) - RM0390 seção 25
 * ======================================================================== */
typedef struct {
    __IO uint32_t SR;    /* 0x00 */
    __IO uint32_t DR;    /* 0x04 */
    __IO uint32_t BRR;   /* 0x08 */
    __IO uint32_t CR1;   /* 0x0C */
    __IO uint32_t CR2;   /* 0x10 */
    __IO uint32_t CR3;   /* 0x14 */
    __IO uint32_t GTPR;  /* 0x18 */
} USART_TypeDef;

#define USART_SR_PE      (1U << 0)
#define USART_SR_FE      (1U << 1)
#define USART_SR_NE      (1U << 2)
#define USART_SR_ORE     (1U << 3)
#define USART_SR_IDLE    (1U << 4)
#define USART_SR_RXNE    (1U << 5)
#define USART_SR_TC      (1U << 6)
#define USART_SR_TXE     (1U << 7)
#define USART_CR1_RE     (1U << 2)
#define USART_CR1_TE     (1U << 3)
#define USART_CR1_IDLEIE (1U << 4)
#define USART_CR1_RXNEIE (1U << 5)
#define USART_CR1_TCIE   (1U << 6)
#define USART_CR1_TXEIE  (1U << 7)
#define USART_CR1_PS     (1U << 9)
#define USART_CR1_PCE    (1U << 10)
#define USART_CR1_M      (1U << 12)
#define USART_CR1_UE     (1U << 13)
#define USART_CR1_OVER8  (1U << 15)
#define USART_CR3_EIE    (1U << 0)
#define USART_CR3_DMAR   (1U << 6)
#define USART_CR3_DMAT   (1U << 7)

#define USART1 ((USART_TypeDef *) USART1_BASE)
#define USART2 ((USART_TypeDef *) USART2_BASE)
#define USART3 ((USART_TypeDef *) USART3_BASE)
#define USART6 ((USART_TypeDef *) USART6_BASE)

/* ===========================================================================
 *  TIM
 * ======================================================================== */
typedef struct {
    __IO uint32_t CR1;    /* 0x00 */
    __IO uint32_t CR2;    /* 0x04 */
    __IO uint32_t SMCR;   /* 0x08 */
    __IO uint32_t DIER;   /* 0x0C */
    __IO uint32_t SR;     /* 0x10 */
    __IO uint32_t EGR;    /* 0x14 */
    __IO uint32_t CCMR1;  /* 0x18 */
    __IO uint32_t CCMR2;  /* 0x1C */
    __IO uint32_t CCER;   /* 0x20 */
    __IO uint32_t CNT;    /* 0x24 */
    __IO uint32_t PSC;    /* 0x28 */
    __IO uint32_t ARR;    /* 0x2C */
    __IO uint32_t RCR;    /* 0x30 */
    __IO uint32_t CCR1;   /* 0x34 */
    __IO uint32_t CCR2;   /* 0x38 */
    __IO uint32_t CCR3;   /* 0x3C */
    __IO uint32_t CCR4;   /* 0x40 */
    __IO uint32_t BDTR;   /* 0x44 */
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
#define TIM4  ((TIM_TypeDef *) TIM4_BASE)
#define TIM5  ((TIM_TypeDef *) TIM5_BASE)
#define TIM6  ((TIM_TypeDef *) TIM6_BASE)
#define TIM7  ((TIM_TypeDef *) TIM7_BASE)

/* ===========================================================================
 *  SPI / I2S - RM0390 seção 26
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
#define SPI_CR1_DFF       (1U << 11)  /* 0 = 8 bits, 1 = 16 bits */
#define SPI_CR1_BIDIOE    (1U << 14)
#define SPI_CR1_BIDIMODE  (1U << 15)
#define SPI_CR2_RXDMAEN   (1U << 0)
#define SPI_CR2_TXDMAEN   (1U << 1)
#define SPI_CR2_SSOE      (1U << 2)
#define SPI_SR_RXNE       (1U << 0)
#define SPI_SR_TXE        (1U << 1)
#define SPI_SR_CHSIDE     (1U << 2)
#define SPI_SR_UDR        (1U << 3)
#define SPI_SR_MODF       (1U << 5)
#define SPI_SR_OVR        (1U << 6)
#define SPI_SR_BSY        (1U << 7)
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
#define SPI3 ((SPI_TypeDef *) SPI3_BASE)

/* ===========================================================================
 *  I2C (IP "clássica", com SR1/SR2/CCR) - RM0390 seção 24
 * ======================================================================== */
typedef struct {
    __IO uint32_t CR1;    /* 0x00 */
    __IO uint32_t CR2;    /* 0x04 */
    __IO uint32_t OAR1;   /* 0x08 */
    __IO uint32_t OAR2;   /* 0x0C */
    __IO uint32_t DR;     /* 0x10 */
    __IO uint32_t SR1;    /* 0x14 */
    __IO uint32_t SR2;    /* 0x18 */
    __IO uint32_t CCR;    /* 0x1C */
    __IO uint32_t TRISE;  /* 0x20 */
    __IO uint32_t FLTR;   /* 0x24 */
} I2C_TypeDef;

#define I2C_CR1_PE       (1U << 0)
#define I2C_CR1_START    (1U << 8)
#define I2C_CR1_STOP     (1U << 9)
#define I2C_CR1_ACK      (1U << 10)
#define I2C_CR1_POS      (1U << 11)
#define I2C_CR1_SWRST    (1U << 15)
#define I2C_CR2_FREQ_Msk (0x3FU << 0)
#define I2C_SR1_SB       (1U << 0)
#define I2C_SR1_ADDR     (1U << 1)
#define I2C_SR1_BTF      (1U << 2)
#define I2C_SR1_STOPF    (1U << 4)
#define I2C_SR1_RXNE     (1U << 6)
#define I2C_SR1_TXE      (1U << 7)
#define I2C_SR1_BERR     (1U << 8)
#define I2C_SR1_ARLO     (1U << 9)
#define I2C_SR1_AF       (1U << 10)
#define I2C_SR1_OVR      (1U << 11)
#define I2C_SR2_MSL      (1U << 0)
#define I2C_SR2_BUSY     (1U << 1)
#define I2C_SR2_TRA      (1U << 2)
#define I2C_CCR_DUTY     (1U << 14)
#define I2C_CCR_FS       (1U << 15)

#define I2C1 ((I2C_TypeDef *) I2C1_BASE)
#define I2C2 ((I2C_TypeDef *) I2C2_BASE)
#define I2C3 ((I2C_TypeDef *) I2C3_BASE)

/* ===========================================================================
 *  ADC - RM0390 seção 13
 * ======================================================================== */
typedef struct {
    __IO uint32_t SR;     /* 0x00 */
    __IO uint32_t CR1;    /* 0x04 */
    __IO uint32_t CR2;    /* 0x08 */
    __IO uint32_t SMPR1;  /* 0x0C canais 10..18 */
    __IO uint32_t SMPR2;  /* 0x10 canais 0..9   */
    __IO uint32_t JOFR[4];/* 0x14..0x20 */
    __IO uint32_t HTR;    /* 0x24 */
    __IO uint32_t LTR;    /* 0x28 */
    __IO uint32_t SQR1;   /* 0x2C L[23:20] + SQ13..16 */
    __IO uint32_t SQR2;   /* 0x30 SQ7..12 */
    __IO uint32_t SQR3;   /* 0x34 SQ1..6  */
    __IO uint32_t JSQR;   /* 0x38 */
    __I  uint32_t JDR[4]; /* 0x3C..0x48 */
    __I  uint32_t DR;     /* 0x4C */
} ADC_TypeDef;

typedef struct {
    __I  uint32_t CSR;    /* 0x300 */
    __IO uint32_t CCR;    /* 0x304 */
    __I  uint32_t CDR;    /* 0x308 */
} ADC_Common_TypeDef;

#define ADC_SR_AWD       (1U << 0)
#define ADC_SR_EOC       (1U << 1)
#define ADC_SR_STRT      (1U << 4)
#define ADC_SR_OVR       (1U << 5)
#define ADC_CR1_SCAN     (1U << 8)
#define ADC_CR1_RES_Pos  24
#define ADC_CR2_ADON     (1U << 0)
#define ADC_CR2_CONT     (1U << 1)
#define ADC_CR2_DMA      (1U << 8)
#define ADC_CR2_DDS      (1U << 9)
#define ADC_CR2_EOCS     (1U << 10)
#define ADC_CR2_ALIGN    (1U << 11)
#define ADC_CR2_SWSTART  (1U << 30)
#define ADC_SQR1_L_Pos   20
#define ADC_CCR_ADCPRE_Pos 16
#define ADC_CCR_VBATE    (1U << 22)
#define ADC_CCR_TSVREFE  (1U << 23)

#define ADC1       ((ADC_TypeDef *) ADC1_BASE)
#define ADC_COMMON ((ADC_Common_TypeDef *) ADC_COMMON_BASE)

/* ===========================================================================
 *  DAC - RM0390 seção 14 (mesmo layout do F0)
 * ======================================================================== */
typedef struct {
    __IO uint32_t CR;
    __IO uint32_t SWTRIGR;
    __IO uint32_t DHR12R1;
    __IO uint32_t DHR12L1;
    __IO uint32_t DHR8R1;
    __IO uint32_t DHR12R2;
    __IO uint32_t DHR12L2;
    __IO uint32_t DHR8R2;
    __IO uint32_t DHR12RD;
    __IO uint32_t DHR12LD;
    __IO uint32_t DHR8RD;
    __I  uint32_t DOR1;
    __I  uint32_t DOR2;
    __IO uint32_t SR;
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
 *  DMA (controlador por STREAMS + seleção de CANAL) - RM0390 seção 9
 * ======================================================================== */
typedef struct {
    __IO uint32_t CR;    /* 0x00 */
    __IO uint32_t NDTR;  /* 0x04 */
    __IO uint32_t PAR;   /* 0x08 */
    __IO uint32_t M0AR;  /* 0x0C */
    __IO uint32_t M1AR;  /* 0x10 */
    __IO uint32_t FCR;   /* 0x14 */
} DMA_Stream_TypeDef;

typedef struct {
    __I  uint32_t LISR;  /* 0x00 streams 0..3 */
    __I  uint32_t HISR;  /* 0x04 streams 4..7 */
    __IO uint32_t LIFCR; /* 0x08 */
    __IO uint32_t HIFCR; /* 0x0C */
    DMA_Stream_TypeDef S[8]; /* 0x10 + 0x18 * n */
} DMA_TypeDef;

#define DMA_SxCR_EN        (1U << 0)
#define DMA_SxCR_DMEIE     (1U << 1)
#define DMA_SxCR_TEIE      (1U << 2)
#define DMA_SxCR_HTIE      (1U << 3)
#define DMA_SxCR_TCIE      (1U << 4)
#define DMA_SxCR_PFCTRL    (1U << 5)
#define DMA_SxCR_DIR_Pos   6     /* 00 P->M, 01 M->P, 10 M->M */
#define DMA_SxCR_CIRC      (1U << 8)
#define DMA_SxCR_PINC      (1U << 9)
#define DMA_SxCR_MINC      (1U << 10)
#define DMA_SxCR_PSIZE_Pos 11
#define DMA_SxCR_MSIZE_Pos 13
#define DMA_SxCR_PL_Pos    16
#define DMA_SxCR_CHSEL_Pos 25
#define DMA_SxFCR_DMDIS    (1U << 2)

/* Posição base das flags de um stream dentro de LISR/HISR (n = 0..7) */
static inline uint32_t dma_flag_shift(uint32_t n)
{
    static const uint8_t s[4] = { 0, 6, 16, 22 };
    return s[n & 3U];
}
#define DMA_FEIF   (1U << 0)
#define DMA_DMEIF  (1U << 2)
#define DMA_TEIF   (1U << 3)
#define DMA_HTIF   (1U << 4)
#define DMA_TCIF   (1U << 5)
#define DMA_ALLF   (0x3DU)

#define DMA1 ((DMA_TypeDef *) DMA1_BASE)
#define DMA2 ((DMA_TypeDef *) DMA2_BASE)

/* ===========================================================================
 *  bxCAN - RM0390 seção 30
 * ======================================================================== */
typedef struct {
    __IO uint32_t TIR;
    __IO uint32_t TDTR;
    __IO uint32_t TDLR;
    __IO uint32_t TDHR;
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
    __IO uint32_t MCR;
    __IO uint32_t MSR;
    __IO uint32_t TSR;
    __IO uint32_t RF0R;
    __IO uint32_t RF1R;
    __IO uint32_t IER;
    __IO uint32_t ESR;
    __IO uint32_t BTR;
    uint32_t      RES0[88];
    CAN_TxMailBox_TypeDef   sTxMailBox[3];
    CAN_FIFOMailBox_TypeDef sFIFOMailBox[2];
    uint32_t      RES1[12];
    __IO uint32_t FMR;
    __IO uint32_t FM1R;
    uint32_t      RES2;
    __IO uint32_t FS1R;
    uint32_t      RES3;
    __IO uint32_t FFA1R;
    uint32_t      RES4;
    __IO uint32_t FA1R;
    uint32_t      RES5[8];
    CAN_FilterRegister_TypeDef sFilterRegister[28];
} CAN_TypeDef;

#define CAN1 ((CAN_TypeDef *) CAN1_BASE)
#define CAN2 ((CAN_TypeDef *) CAN2_BASE)

/* ===========================================================================
 *  IWDG / WWDG
 * ======================================================================== */
typedef struct {
    __IO uint32_t KR;
    __IO uint32_t PR;
    __IO uint32_t RLR;
    __I  uint32_t SR;
} IWDG_TypeDef;

typedef struct {
    __IO uint32_t CR;
    __IO uint32_t CFR;
    __IO uint32_t SR;
} WWDG_TypeDef;

#define IWDG ((IWDG_TypeDef *) IWDG_BASE)
#define WWDG ((WWDG_TypeDef *) WWDG_BASE)

/* ===========================================================================
 *  RTC - RM0390 seção 22
 * ======================================================================== */
typedef struct {
    __IO uint32_t TR;       /* 0x00 */
    __IO uint32_t DR;       /* 0x04 */
    __IO uint32_t CR;       /* 0x08 */
    __IO uint32_t ISR;      /* 0x0C */
    __IO uint32_t PRER;     /* 0x10 */
    __IO uint32_t WUTR;     /* 0x14 */
    __IO uint32_t CALIBR;   /* 0x18 */
    __IO uint32_t ALRMAR;   /* 0x1C */
    __IO uint32_t ALRMBR;   /* 0x20 */
    __IO uint32_t WPR;      /* 0x24 */
    __I  uint32_t SSR;      /* 0x28 */
    __IO uint32_t SHIFTR;   /* 0x2C */
    __I  uint32_t TSTR;     /* 0x30 */
    __I  uint32_t TSDR;     /* 0x34 */
    __I  uint32_t TSSSR;    /* 0x38 */
    __IO uint32_t CALR;     /* 0x3C */
    __IO uint32_t TAFCR;    /* 0x40 */
    __IO uint32_t ALRMASSR; /* 0x44 */
    __IO uint32_t ALRMBSSR; /* 0x48 */
    uint32_t      RES;      /* 0x4C */
    __IO uint32_t BKPR[20]; /* 0x50 backup registers (80 bytes) */
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
    __I  uint32_t IDCODE;
    __IO uint32_t CR;
    __IO uint32_t APB1FZ;
    __IO uint32_t APB2FZ;
} DBGMCU_TypeDef;

#define DBGMCU ((DBGMCU_TypeDef *) DBGMCU_BASE)

/* ===========================================================================
 *  Funções inline do núcleo
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
    NVIC->ISER[(uint32_t)irq >> 5] = 1U << ((uint32_t)irq & 0x1FU);
}

static inline void NVIC_DisableIRQ(IRQn_Type irq)
{
    NVIC->ICER[(uint32_t)irq >> 5] = 1U << ((uint32_t)irq & 0x1FU);
    __DSB();
    __ISB();
}

static inline void NVIC_ClearPendingIRQ(IRQn_Type irq)
{
    NVIC->ICPR[(uint32_t)irq >> 5] = 1U << ((uint32_t)irq & 0x1FU);
}

/* prio: 0 (mais alta) .. 15 (mais baixa) */
static inline void NVIC_SetPriority(IRQn_Type irq, uint32_t prio)
{
    NVIC->IP[(uint32_t)irq] = (uint8_t)((prio << (8U - NVIC_PRIO_BITS)) & 0xFFU);
}

static inline void NVIC_SystemReset(void)
{
    __DSB();
    SCB->AIRCR = SCB_AIRCR_VECTKEY | (SCB->AIRCR & (7U << 8)) | SCB_AIRCR_SYSRESETREQ;
    __DSB();
    for (;;) { }
}

#endif /* STM32F4_H */
