/*
 * tm4c_common.h - Núcleo Cortex-M4F + periféricos comuns ao TM4C123 e TM4C129
 *
 * Escrito à mão para o curso a partir dos datasheets TM4C123GH6PM (SPMS376)
 * e TM4C1294NCPDT (SPMS433). NÃO usa TivaWare, driverlib ou CMSIS.
 *
 * Os blocos (GPIO, UART, GPTM, SSI, I2C, ADC, CAN, PWM, QEI, WDT, HIB, uDMA)
 * têm o MESMO layout nas duas famílias; endereços-base, números de IRQ e
 * o controle de clock ficam nos headers tm4c123.h / tm4c129.h.
 */
#ifndef TM4C_COMMON_H
#define TM4C_COMMON_H

#include <stdint.h>

#define __IO volatile
#define __I  volatile const
#define __O  volatile

#define REG32(addr) (*(volatile uint32_t *)(addr))

/* ===========================================================================
 *  Núcleo: SysTick, NVIC, SCB (iguais em qualquer Cortex-M4)
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
    __IO uint32_t IABR[8];   /* 0x200 */
    uint32_t      RES4[56];
    __IO uint8_t  IP[240];   /* 0x300 */
    uint32_t      RES5[644];
    __IO uint32_t STIR;      /* 0xE00 */
} NVIC_TypeDef;

typedef struct {
    __I  uint32_t CPUID;   /* 0x00 */
    __IO uint32_t ICSR;    /* 0x04 */
    __IO uint32_t VTOR;    /* 0x08 */
    __IO uint32_t AIRCR;   /* 0x0C */
    __IO uint32_t SCR;     /* 0x10 */
    __IO uint32_t CCR;     /* 0x14 */
    __IO uint8_t  SHP[12]; /* 0x18 */
    __IO uint32_t SHCSR;   /* 0x24 */
    __IO uint32_t CFSR;    /* 0x28 */
    __IO uint32_t HFSR;    /* 0x2C */
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
    __IO uint32_t CPACR;   /* 0x88 */
} SCB_TypeDef;

#define SCB_SCR_SLEEPONEXIT   (1U << 1)
#define SCB_SCR_SLEEPDEEP     (1U << 2)
#define SCB_AIRCR_VECTKEY     (0x05FAU << 16)
#define SCB_AIRCR_SYSRESETREQ (1U << 2)

#define SysTick ((SysTick_TypeDef *) 0xE000E010UL)
#define NVIC    ((NVIC_TypeDef *)    0xE000E100UL)
#define SCB     ((SCB_TypeDef *)     0xE000ED00UL)

/* ===========================================================================
 *  GPIO (datasheet: "General-Purpose Input/Outputs")
 *  Particularidade TI: o registrador DATA é ENDEREÇADO POR MÁSCARA. Os bits
 *  [9:2] do endereço escolhem quais pinos a leitura/escrita afeta:
 *     DATA_BITS[1 << pino] acessa só aquele pino (sem read-modify-write)
 *     DATA (offset 0x3FC) acessa os 8 pinos
 * ======================================================================== */
typedef struct {
    __IO uint32_t DATA_BITS[255]; /* 0x000-0x3F8 acesso mascarado          */
    __IO uint32_t DATA;           /* 0x3FC todos os pinos                  */
    __IO uint32_t DIR;            /* 0x400 1 = saída                       */
    __IO uint32_t IS;             /* 0x404 1 = interrupção por nível       */
    __IO uint32_t IBE;            /* 0x408 1 = ambas as bordas             */
    __IO uint32_t IEV;            /* 0x40C 1 = subida/nível alto           */
    __IO uint32_t IM;             /* 0x410 1 = interrupção habilitada      */
    __I  uint32_t RIS;            /* 0x414 raw status                      */
    __I  uint32_t MIS;            /* 0x418 masked status                   */
    __O  uint32_t ICR;            /* 0x41C escreve 1 para limpar           */
    __IO uint32_t AFSEL;          /* 0x420 1 = função alternativa          */
    uint32_t      RES0[55];       /* 0x424-0x4FC                           */
    __IO uint32_t DR2R;           /* 0x500 drive 2 mA                      */
    __IO uint32_t DR4R;           /* 0x504 drive 4 mA                      */
    __IO uint32_t DR8R;           /* 0x508 drive 8 mA                      */
    __IO uint32_t ODR;            /* 0x50C open-drain                      */
    __IO uint32_t PUR;            /* 0x510 pull-up                         */
    __IO uint32_t PDR;            /* 0x514 pull-down                       */
    __IO uint32_t SLR;            /* 0x518 slew-rate control               */
    __IO uint32_t DEN;            /* 0x51C 1 = função DIGITAL habilitada   */
    __IO uint32_t LOCK;           /* 0x520 chave 0x4C4F434B                */
    __IO uint32_t CR;             /* 0x524 commit (pinos protegidos)       */
    __IO uint32_t AMSEL;          /* 0x528 1 = analógico                   */
    __IO uint32_t PCTL;           /* 0x52C 4 bits por pino: qual periférico */
    __IO uint32_t ADCCTL;         /* 0x530 */
    __IO uint32_t DMACTL;         /* 0x534 */
} GPIO_TypeDef;

#define GPIO_LOCK_KEY 0x4C4F434BUL   /* "LOCK" em ASCII */

/* ===========================================================================
 *  UART (compatível com o ARM PL011)
 * ======================================================================== */
typedef struct {
    __IO uint32_t DR;        /* 0x000 dado + flags de erro do byte */
    __IO uint32_t RSR;       /* 0x004 status/erro (ECR na escrita) */
    uint32_t      RES0[4];
    __I  uint32_t FR;        /* 0x018 flags */
    uint32_t      RES1;
    __IO uint32_t ILPR;      /* 0x020 IrDA */
    __IO uint32_t IBRD;      /* 0x024 divisor inteiro */
    __IO uint32_t FBRD;      /* 0x028 divisor fracionário (1/64) */
    __IO uint32_t LCRH;      /* 0x02C line control */
    __IO uint32_t CTL;       /* 0x030 */
    __IO uint32_t IFLS;      /* 0x034 nível da FIFO p/ interrupção */
    __IO uint32_t IM;        /* 0x038 */
    __I  uint32_t RIS;       /* 0x03C */
    __I  uint32_t MIS;       /* 0x040 */
    __O  uint32_t ICR;       /* 0x044 */
    __IO uint32_t DMACTL;    /* 0x048 */
    uint32_t      RES2[991];
    __IO uint32_t CC;        /* 0xFC8 fonte de clock: 0 = sysclk, 5 = PIOSC */
} UART_TypeDef;

#define UART_FR_BUSY   (1U << 3)
#define UART_FR_RXFE   (1U << 4)
#define UART_FR_TXFF   (1U << 5)
#define UART_FR_RXFF   (1U << 6)
#define UART_FR_TXFE   (1U << 7)
#define UART_LCRH_PEN  (1U << 1)
#define UART_LCRH_EPS  (1U << 2)
#define UART_LCRH_STP2 (1U << 3)
#define UART_LCRH_FEN  (1U << 4)
#define UART_LCRH_WLEN_8 (3U << 5)
#define UART_CTL_UARTEN (1U << 0)
#define UART_CTL_EOT   (1U << 4)
#define UART_CTL_HSE   (1U << 5)
#define UART_CTL_LBE   (1U << 7)
#define UART_CTL_TXE   (1U << 8)
#define UART_CTL_RXE   (1U << 9)
#define UART_INT_RX    (1U << 4)
#define UART_INT_TX    (1U << 5)
#define UART_INT_RT    (1U << 6)   /* receive timeout */
#define UART_INT_FE    (1U << 7)
#define UART_INT_PE    (1U << 8)
#define UART_INT_BE    (1U << 9)
#define UART_INT_OE    (1U << 10)
#define UART_DMACTL_RXDMAE (1U << 0)
#define UART_DMACTL_TXDMAE (1U << 1)

/* ===========================================================================
 *  GPTM - General-Purpose Timer Module
 * ======================================================================== */
typedef struct {
    __IO uint32_t CFG;       /* 0x000 0 = 32 bits (concatenado), 4 = 16 bits (split) */
    __IO uint32_t TAMR;      /* 0x004 modo do timer A */
    __IO uint32_t TBMR;      /* 0x008 */
    __IO uint32_t CTL;       /* 0x00C */
    __IO uint32_t SYNC;      /* 0x010 */
    uint32_t      RES0;
    __IO uint32_t IMR;       /* 0x018 */
    __I  uint32_t RIS;       /* 0x01C */
    __I  uint32_t MIS;       /* 0x020 */
    __O  uint32_t ICR;       /* 0x024 */
    __IO uint32_t TAILR;     /* 0x028 interval load */
    __IO uint32_t TBILR;     /* 0x02C */
    __IO uint32_t TAMATCHR;  /* 0x030 */
    __IO uint32_t TBMATCHR;  /* 0x034 */
    __IO uint32_t TAPR;      /* 0x038 prescaler (ou extensão em capture) */
    __IO uint32_t TBPR;      /* 0x03C */
    __IO uint32_t TAPMR;     /* 0x040 */
    __IO uint32_t TBPMR;     /* 0x044 */
    __I  uint32_t TAR;       /* 0x048 valor capturado / atual */
    __I  uint32_t TBR;       /* 0x04C */
    __IO uint32_t TAV;       /* 0x050 contador "vivo" */
    __IO uint32_t TBV;       /* 0x054 */
    __I  uint32_t RTCPD;     /* 0x058 */
    __I  uint32_t TAPS;      /* 0x05C */
    __I  uint32_t TBPS;      /* 0x060 */
    __I  uint32_t TAPV;      /* 0x064 */
    __I  uint32_t TBPV;      /* 0x068 */
    __IO uint32_t DMAEV;     /* 0x06C só TM4C129 */
    __IO uint32_t ADCEV;     /* 0x070 só TM4C129 */
    uint32_t      RES1[979];
    __I  uint32_t PP;        /* 0xFC0 */
    uint32_t      RES2;
    __IO uint32_t CC;        /* 0xFC8 só TM4C129 */
} TIMER_TypeDef;

#define TIMER_CFG_32       0x0U
#define TIMER_CFG_16       0x4U
#define TIMER_TAMR_ONESHOT 0x1U
#define TIMER_TAMR_PERIOD  0x2U
#define TIMER_TAMR_CAP     0x3U
#define TIMER_TAMR_TACMR   (1U << 2)   /* 1 = edge-time, 0 = edge-count */
#define TIMER_TAMR_TAAMS   (1U << 3)   /* 1 = PWM */
#define TIMER_TAMR_TACDIR  (1U << 4)   /* 1 = conta para cima */
#define TIMER_TAMR_TAMIE   (1U << 5)
#define TIMER_TAMR_TAILD   (1U << 8)
#define TIMER_CTL_TAEN     (1U << 0)
#define TIMER_CTL_TASTALL  (1U << 1)   /* para quando o debugger para a CPU */
#define TIMER_CTL_TAEVENT_POS  (0U << 2)
#define TIMER_CTL_TAEVENT_NEG  (1U << 2)
#define TIMER_CTL_TAEVENT_BOTH (3U << 2)
#define TIMER_CTL_TAOTE    (1U << 5)   /* trigger de ADC */
#define TIMER_CTL_TAPWML   (1U << 6)   /* PWM invertido */
#define TIMER_CTL_TBEN     (1U << 8)
#define TIMER_INT_TATO     (1U << 0)   /* timeout A */
#define TIMER_INT_CAM      (1U << 1)   /* capture match A */
#define TIMER_INT_CAE      (1U << 2)   /* capture event A */
#define TIMER_INT_TAM      (1U << 4)   /* match A */
#define TIMER_INT_TBTO     (1U << 8)
#define TIMER_DMAEV_TATODMAEN (1U << 0)

/* ===========================================================================
 *  SSI - Synchronous Serial Interface (o "SPI" da TI)
 * ======================================================================== */
typedef struct {
    __IO uint32_t CR0;       /* 0x000 SCR, SPH, SPO, FRF, DSS */
    __IO uint32_t CR1;       /* 0x004 LBM, SSE, MS, EOT */
    __IO uint32_t DR;        /* 0x008 */
    __I  uint32_t SR;        /* 0x00C */
    __IO uint32_t CPSR;      /* 0x010 prescaler (par, 2..254) */
    __IO uint32_t IM;        /* 0x014 */
    __I  uint32_t RIS;       /* 0x018 */
    __I  uint32_t MIS;       /* 0x01C */
    __O  uint32_t ICR;       /* 0x020 */
    __IO uint32_t DMACTL;    /* 0x024 */
    uint32_t      RES[1000];
    __IO uint32_t CC;        /* 0xFC8 */
} SSI_TypeDef;

#define SSI_CR0_DSS_8    0x7U
#define SSI_CR0_DSS_16   0xFU
#define SSI_CR0_FRF_MOTO (0U << 4)   /* Freescale SPI */
#define SSI_CR0_FRF_TI   (1U << 4)
#define SSI_CR0_FRF_NMW  (2U << 4)   /* Microwire */
#define SSI_CR0_SPO      (1U << 6)
#define SSI_CR0_SPH      (1U << 7)
#define SSI_CR0_SCR_Pos  8
#define SSI_CR1_LBM      (1U << 0)   /* loopback interno */
#define SSI_CR1_SSE      (1U << 1)
#define SSI_CR1_MS       (1U << 2)   /* 1 = escravo */
#define SSI_SR_TFE       (1U << 0)
#define SSI_SR_TNF       (1U << 1)
#define SSI_SR_RNE       (1U << 2)
#define SSI_SR_RFF       (1U << 3)
#define SSI_SR_BSY       (1U << 4)
#define SSI_DMACTL_RXDMAE (1U << 0)
#define SSI_DMACTL_TXDMAE (1U << 1)

/* ===========================================================================
 *  I2C (só a parte MESTRE; o escravo começa no offset 0x800)
 * ======================================================================== */
typedef struct {
    __IO uint32_t MSA;       /* 0x000 endereço do escravo [7:1] + R/S [0] */
    __IO uint32_t MCS;       /* 0x004 controle (escrita) / status (leitura) */
    __IO uint32_t MDR;       /* 0x008 dado */
    __IO uint32_t MTPR;      /* 0x00C timer period (SCL) */
    __IO uint32_t MIMR;      /* 0x010 */
    __I  uint32_t MRIS;      /* 0x014 */
    __I  uint32_t MMIS;      /* 0x018 */
    __O  uint32_t MICR;      /* 0x01C */
    __IO uint32_t MCR;       /* 0x020 MFE = habilita mestre */
    __IO uint32_t MCLKOCNT;  /* 0x024 */
    uint32_t      RES0;
    __I  uint32_t MBMON;     /* 0x02C estado real das linhas SCL/SDA */
} I2C_TypeDef;

/* MCS na escrita */
#define I2C_MCS_RUN    (1U << 0)
#define I2C_MCS_START  (1U << 1)
#define I2C_MCS_STOP   (1U << 2)
#define I2C_MCS_ACK    (1U << 3)
/* MCS na leitura */
#define I2C_MCS_BUSY   (1U << 0)
#define I2C_MCS_ERROR  (1U << 1)
#define I2C_MCS_ADRACK (1U << 2)   /* 1 = endereço NÃO reconhecido */
#define I2C_MCS_DATACK (1U << 3)   /* 1 = dado NÃO reconhecido */
#define I2C_MCS_ARBLST (1U << 4)
#define I2C_MCS_IDLE   (1U << 5)
#define I2C_MCS_BUSBSY (1U << 6)
#define I2C_MCS_CLKTO  (1U << 7)
#define I2C_MCR_LPBK   (1U << 0)
#define I2C_MCR_MFE    (1U << 4)
#define I2C_MCR_SFE    (1U << 5)

/* ===========================================================================
 *  ADC (sequenciadores de amostras SS0..SS3)
 * ======================================================================== */
typedef struct {
    __IO uint32_t MUX;       /* +0x00 canal de cada passo (4 bits por passo) */
    __IO uint32_t CTL;       /* +0x04 END/IE/TS/D por passo (4 bits por passo) */
    __I  uint32_t FIFO;      /* +0x08 resultado */
    __I  uint32_t FSTAT;     /* +0x0C status da FIFO */
    __IO uint32_t OP;        /* +0x10 */
    __IO uint32_t DC;        /* +0x14 */
    __IO uint32_t EMUX;      /* +0x18 (129) */
    __IO uint32_t TSH;       /* +0x1C (129) tempo de amostragem */
} ADC_SS_TypeDef;

typedef struct {
    __IO uint32_t ACTSS;     /* 0x000 ativa sequenciadores */
    __I  uint32_t RIS;       /* 0x004 */
    __IO uint32_t IM;        /* 0x008 */
    __IO uint32_t ISC;       /* 0x00C status/clear (escreve 1) */
    __IO uint32_t OSTAT;     /* 0x010 overflow */
    __IO uint32_t EMUX;      /* 0x014 fonte de trigger (4 bits por SS) */
    __IO uint32_t USTAT;     /* 0x018 underflow */
    __IO uint32_t TSSEL;     /* 0x01C (129) */
    __IO uint32_t SSPRI;     /* 0x020 prioridade dos SS */
    __IO uint32_t SPC;       /* 0x024 */
    __IO uint32_t PSSI;      /* 0x028 dispara por software */
    uint32_t      RES0;
    __IO uint32_t SAC;       /* 0x030 média por hardware (2^n amostras) */
    __IO uint32_t DCISC;     /* 0x034 */
    __IO uint32_t CTL;       /* 0x038 referência (VREF) */
    uint32_t      RES1;
    ADC_SS_TypeDef SS[4];    /* 0x040, 0x060, 0x080, 0x0A0 */
    uint32_t      RES2[961];
    __IO uint32_t PC;        /* 0xFC4 taxa de amostragem */
    __IO uint32_t CC;        /* 0xFC8 fonte de clock */
} ADC_TypeDef;

#define ADC_SSCTL_END0 (1U << 1)
#define ADC_SSCTL_IE0  (1U << 2)
#define ADC_SSCTL_TS0  (1U << 3)   /* amostra o sensor de temperatura */

/* ===========================================================================
 *  CAN (Bosch C_CAN: 32 "message objects" acessados via IF1/IF2)
 * ======================================================================== */
typedef struct {
    __IO uint32_t CRQ;       /* +0x00 número do objeto + BUSY */
    __IO uint32_t CMSK;      /* +0x04 o que transferir */
    __IO uint32_t MSK1;      /* +0x08 */
    __IO uint32_t MSK2;      /* +0x0C */
    __IO uint32_t ARB1;      /* +0x10 */
    __IO uint32_t ARB2;      /* +0x14 MSGVAL, XTD, DIR, ID */
    __IO uint32_t MCTL;      /* +0x18 */
    __IO uint32_t DA1;       /* +0x1C bytes 0,1 */
    __IO uint32_t DA2;       /* +0x20 bytes 2,3 */
    __IO uint32_t DB1;       /* +0x24 bytes 4,5 */
    __IO uint32_t DB2;       /* +0x28 bytes 6,7 */
} CAN_IF_TypeDef;

typedef struct {
    __IO uint32_t CTL;       /* 0x000 */
    __IO uint32_t STS;       /* 0x004 */
    __I  uint32_t ERR;       /* 0x008 contadores TEC/REC */
    __IO uint32_t BIT;       /* 0x00C bit timing */
    __I  uint32_t INT;       /* 0x010 */
    __IO uint32_t TST;       /* 0x014 test (loopback, silent) */
    __IO uint32_t BRPE;      /* 0x018 */
    uint32_t      RES0;
    CAN_IF_TypeDef IF1;      /* 0x020 */
    uint32_t      RES1[13];
    CAN_IF_TypeDef IF2;      /* 0x080 */
    uint32_t      RES2[21];
    __I  uint32_t TXRQ1;     /* 0x100 */
    __I  uint32_t TXRQ2;
    uint32_t      RES3[6];
    __I  uint32_t NWDA1;     /* 0x120 */
    __I  uint32_t NWDA2;
    uint32_t      RES4[6];
    __I  uint32_t MSG1INT;   /* 0x140 */
    __I  uint32_t MSG2INT;
    uint32_t      RES5[6];
    __I  uint32_t MSG1VAL;   /* 0x160 */
    __I  uint32_t MSG2VAL;
} CAN_TypeDef;

#define CAN_CTL_INIT    (1U << 0)
#define CAN_CTL_IE      (1U << 1)
#define CAN_CTL_CCE     (1U << 6)   /* libera escrita no BIT */
#define CAN_CTL_TEST    (1U << 7)
#define CAN_STS_TXOK    (1U << 3)
#define CAN_STS_RXOK    (1U << 4)
#define CAN_STS_EPASS   (1U << 5)
#define CAN_STS_EWARN   (1U << 6)
#define CAN_STS_BOFF    (1U << 7)
#define CAN_TST_SILENT  (1U << 3)
#define CAN_TST_LBACK   (1U << 4)
#define CAN_IFCRQ_BUSY  (1U << 15)
#define CAN_IFCMSK_DATAB    (1U << 0)
#define CAN_IFCMSK_DATAA    (1U << 1)
#define CAN_IFCMSK_TXRQST   (1U << 2)   /* = NEWDAT na leitura */
#define CAN_IFCMSK_CLRINTPND (1U << 3)
#define CAN_IFCMSK_CONTROL  (1U << 4)
#define CAN_IFCMSK_ARB      (1U << 5)
#define CAN_IFCMSK_MASK     (1U << 6)
#define CAN_IFCMSK_WRNRD    (1U << 7)   /* 1 = escrever no objeto */
#define CAN_IFMSK2_MDIR     (1U << 14)
#define CAN_IFMSK2_MXTD     (1U << 15)
#define CAN_IFARB2_DIR      (1U << 13)  /* 1 = transmitir */
#define CAN_IFARB2_XTD      (1U << 14)
#define CAN_IFARB2_MSGVAL   (1U << 15)
#define CAN_IFMCTL_EOB      (1U << 7)
#define CAN_IFMCTL_TXRQST   (1U << 8)
#define CAN_IFMCTL_RXIE     (1U << 10)
#define CAN_IFMCTL_TXIE     (1U << 11)
#define CAN_IFMCTL_UMASK    (1U << 12)
#define CAN_IFMCTL_INTPND   (1U << 13)
#define CAN_IFMCTL_MSGLST   (1U << 14)
#define CAN_IFMCTL_NEWDAT   (1U << 15)

/* ===========================================================================
 *  PWM (módulos com 4 geradores; cada gerador produz 2 saídas A/B)
 * ======================================================================== */
typedef struct {
    __IO uint32_t CTL;       /* +0x00 */
    __IO uint32_t INTEN;     /* +0x04 */
    __I  uint32_t RIS;       /* +0x08 */
    __IO uint32_t ISC;       /* +0x0C */
    __IO uint32_t LOAD;      /* +0x10 período */
    __I  uint32_t COUNT;     /* +0x14 */
    __IO uint32_t CMPA;      /* +0x18 */
    __IO uint32_t CMPB;      /* +0x1C */
    __IO uint32_t GENA;      /* +0x20 ações da saída A */
    __IO uint32_t GENB;      /* +0x24 ações da saída B */
    __IO uint32_t DBCTL;     /* +0x28 dead-band */
    __IO uint32_t DBRISE;    /* +0x2C */
    __IO uint32_t DBFALL;    /* +0x30 */
    __IO uint32_t FLTSRC0;   /* +0x34 */
    __IO uint32_t FLTSRC1;   /* +0x38 */
    __IO uint32_t MINFLTPER; /* +0x3C */
} PWM_GEN_TypeDef;

typedef struct {
    __IO uint32_t CTL;       /* 0x000 */
    __IO uint32_t SYNC;      /* 0x004 */
    __IO uint32_t ENABLE;    /* 0x008 bit n = saída PWMn */
    __IO uint32_t INVERT;    /* 0x00C */
    __IO uint32_t FAULT;     /* 0x010 */
    __IO uint32_t INTEN;     /* 0x014 */
    __I  uint32_t RIS;       /* 0x018 */
    __IO uint32_t ISC;       /* 0x01C */
    __I  uint32_t STATUS;    /* 0x020 */
    __IO uint32_t FAULTVAL;  /* 0x024 */
    __IO uint32_t ENUPD;     /* 0x028 */
    uint32_t      RES0[5];
    PWM_GEN_TypeDef GEN[4];  /* 0x040, 0x080, 0x0C0, 0x100 */
    uint32_t      RES1[928];
    __I  uint32_t PP;        /* 0xFC0 */
    uint32_t      RES2;
    __IO uint32_t CC;        /* 0xFC8 só TM4C129: divisor do clock PWM */
} PWM_TypeDef;

#define PWM_GEN_CTL_ENABLE  (1U << 0)
#define PWM_GEN_CTL_MODE    (1U << 1)   /* 1 = up/down (centrado) */
/* GENA/GENB: ações 00 nada, 01 inverte, 10 nível 0, 11 nível 1 */
#define PWM_ACT_ZERO_Pos    0
#define PWM_ACT_LOAD_Pos    2
#define PWM_ACT_CMPAU_Pos   4
#define PWM_ACT_CMPAD_Pos   6
#define PWM_ACT_CMPBU_Pos   8
#define PWM_ACT_CMPBD_Pos   10
#define PWM_ACT_LOW         2U
#define PWM_ACT_HIGH        3U

/* ===========================================================================
 *  QEI - Quadrature Encoder Interface
 * ======================================================================== */
typedef struct {
    __IO uint32_t CTL;       /* 0x000 */
    __I  uint32_t STAT;      /* 0x004 */
    __IO uint32_t POS;       /* 0x008 */
    __IO uint32_t MAXPOS;    /* 0x00C */
    __IO uint32_t LOAD;      /* 0x010 */
    __I  uint32_t TIME;      /* 0x014 */
    __I  uint32_t COUNT;     /* 0x018 */
    __I  uint32_t SPEED;     /* 0x01C */
    __IO uint32_t INTEN;     /* 0x020 */
    __I  uint32_t RIS;       /* 0x024 */
    __IO uint32_t ISC;       /* 0x028 */
} QEI_TypeDef;

#define QEI_CTL_ENABLE  (1U << 0)
#define QEI_CTL_SWAP    (1U << 1)
#define QEI_CTL_SIGMODE (1U << 2)
#define QEI_CTL_CAPMODE (1U << 3)   /* 1 = conta bordas de A e B (x4) */
#define QEI_CTL_VELEN   (1U << 5)
#define QEI_CTL_FILTEN  (1U << 13)
#define QEI_STAT_DIR    (1U << 1)

/* ===========================================================================
 *  Watchdog
 * ======================================================================== */
typedef struct {
    __IO uint32_t LOAD;      /* 0x000 */
    __I  uint32_t VALUE;     /* 0x004 */
    __IO uint32_t CTL;       /* 0x008 */
    __O  uint32_t ICR;       /* 0x00C qualquer escrita: limpa int. e recarrega */
    __I  uint32_t RIS;       /* 0x010 */
    __I  uint32_t MIS;       /* 0x014 */
    uint32_t      RES0[256];
    __IO uint32_t TEST;      /* 0x418 STALL */
    uint32_t      RES1[505];
    __IO uint32_t LOCK;      /* 0xC00 0x1ACCE551 destrava */
} WDT_TypeDef;

#define WDT_CTL_INTEN   (1U << 0)
#define WDT_CTL_RESEN   (1U << 1)
#define WDT_CTL_INTTYPE (1U << 2)   /* 1 = NMI */
#define WDT_CTL_WRC     (1U << 31)  /* WDT1: escrita concluída */
#define WDT_TEST_STALL  (1U << 8)
#define WDT_UNLOCK      0x1ACCE551UL

/* ===========================================================================
 *  Hibernation module (contém o RTC)
 * ======================================================================== */
typedef struct {
    __I  uint32_t RTCC;      /* 0x000 contador de segundos */
    __IO uint32_t RTCM0;     /* 0x004 match 0 (alarme) */
    uint32_t      RES0;
    __IO uint32_t RTCLD;     /* 0x00C load (acerta o relógio) */
    __IO uint32_t CTL;       /* 0x010 */
    __IO uint32_t IM;        /* 0x014 */
    __I  uint32_t RIS;       /* 0x018 */
    __I  uint32_t MIS;       /* 0x01C */
    __IO uint32_t IC;        /* 0x020 */
    __IO uint32_t RTCT;      /* 0x024 trim */
    __IO uint32_t RTCSS;     /* 0x028 sub-segundos [14:0] (1/32768 s) */
    __IO uint32_t IO;        /* 0x02C (129) */
    __IO uint32_t DATA[16];  /* 0x030 memória de 64 bytes preservada em hibernação */
} HIB_TypeDef;

#define HIB_CTL_RTCEN   (1U << 0)
#define HIB_CTL_HIBREQ  (1U << 1)
#define HIB_CTL_RTCWEN  (1U << 3)   /* acorda no alarme do RTC */
#define HIB_CTL_PINWEN  (1U << 4)   /* acorda no pino WAKE */
#define HIB_CTL_CLK32EN (1U << 6)
#define HIB_CTL_VDD3ON  (1U << 8)
#define HIB_CTL_WRC     (1U << 31)
#define HIB_INT_RTCALT0 (1U << 0)
#define HIB_INT_EXTW    (1U << 3)
#define HIB_INT_WC      (1U << 4)

/* ===========================================================================
 *  uDMA (base 0x400FF000)
 * ======================================================================== */
typedef struct {
    __I  uint32_t STAT;        /* 0x000 */
    __O  uint32_t CFG;         /* 0x004 MASTEN */
    __IO uint32_t CTLBASE;     /* 0x008 endereço da tabela de controle (alinhada 1 KB) */
    __I  uint32_t ALTBASE;     /* 0x00C */
    __I  uint32_t WAITSTAT;    /* 0x010 */
    __O  uint32_t SWREQ;       /* 0x014 */
    __IO uint32_t USEBURSTSET; /* 0x018 */
    __O  uint32_t USEBURSTCLR; /* 0x01C */
    __IO uint32_t REQMASKSET;  /* 0x020 */
    __O  uint32_t REQMASKCLR;  /* 0x024 */
    __IO uint32_t ENASET;      /* 0x028 */
    __O  uint32_t ENACLR;      /* 0x02C */
    __IO uint32_t ALTSET;      /* 0x030 */
    __O  uint32_t ALTCLR;      /* 0x034 */
    __IO uint32_t PRIOSET;     /* 0x038 */
    __O  uint32_t PRIOCLR;     /* 0x03C */
    uint32_t      RES0[3];
    __IO uint32_t ERRCLR;      /* 0x04C */
    uint32_t      RES1[300];
    __IO uint32_t CHASGN;      /* 0x500 */
    __IO uint32_t CHIS;        /* 0x504 (123: CHIS; 129: idem) */
    uint32_t      RES2[2];
    __IO uint32_t CHMAP[4];    /* 0x510 encoding de cada canal (4 bits por canal) */
} UDMA_TypeDef;

/* Entrada da tabela de controle: 16 bytes por canal */
typedef struct {
    const volatile void *SRCENDP; /* endereço do ÚLTIMO item da origem  */
    volatile void    *DSTENDP;   /* endereço do ÚLTIMO item do destino */
    volatile uint32_t CHCTL;
    uint32_t          SPARE;
} UDMA_Entry;

#define UDMA_INC_8    0U
#define UDMA_INC_16   1U
#define UDMA_INC_32   2U
#define UDMA_INC_NONE 3U
#define UDMA_SIZE_8   0U
#define UDMA_SIZE_16  1U
#define UDMA_SIZE_32  2U
#define UDMA_CHCTL(dinc, dsize, sinc, ssize, arb_log2, n, mode) \
    (((uint32_t)(dinc) << 30) | ((uint32_t)(dsize) << 28) | ((uint32_t)(sinc) << 26) | \
     ((uint32_t)(ssize) << 24) | ((uint32_t)(arb_log2) << 14) | (((uint32_t)(n) - 1U) << 4) | (mode))
#define UDMA_MODE_STOP     0U
#define UDMA_MODE_BASIC    1U
#define UDMA_MODE_AUTO     2U
#define UDMA_MODE_PINGPONG 3U

#define UDMA ((UDMA_TypeDef *) 0x400FF000UL)

/* ===========================================================================
 *  Controle da Flash (base 0x400FD000)
 * ======================================================================== */
#define FLASH_CTRL_BASE 0x400FD000UL
#define FLASH_FMA      REG32(FLASH_CTRL_BASE + 0x000)
#define FLASH_FMD      REG32(FLASH_CTRL_BASE + 0x004)
#define FLASH_FMC      REG32(FLASH_CTRL_BASE + 0x008)
#define FLASH_FCRIS    REG32(FLASH_CTRL_BASE + 0x00C)
#define FLASH_FCMISC   REG32(FLASH_CTRL_BASE + 0x014)
#define FLASH_FMC2     REG32(FLASH_CTRL_BASE + 0x020)
#define FLASH_FLPEKEY  REG32(FLASH_CTRL_BASE + 0x03C)   /* só 129 */
#define FLASH_PP       REG32(FLASH_CTRL_BASE + 0xFC0)   /* tamanho da flash */
#define FLASH_SSIZE    REG32(FLASH_CTRL_BASE + 0xFC4)   /* tamanho da SRAM  */
#define FLASH_FMC_WRITE  (1U << 0)
#define FLASH_FMC_ERASE  (1U << 1)
#define FLASH_FMC_MERASE (1U << 2)
#define FLASH_FCRIS_ARIS (1U << 0)    /* erro de acesso (proteção) */
#define FLASH_FCRIS_PRIS (1U << 1)    /* operação concluída */

/* ===========================================================================
 *  System Control (base 0x400FE000): registradores comuns às duas famílias
 *  Formato "TI": um registrador por periférico, 1 bit por instância.
 * ======================================================================== */
#define SYSCTL_BASE      0x400FE000UL
#define SYSCTL_DID0      REG32(SYSCTL_BASE + 0x000)
#define SYSCTL_DID1      REG32(SYSCTL_BASE + 0x004)
#define SYSCTL_RIS       REG32(SYSCTL_BASE + 0x050)
#define SYSCTL_MISC      REG32(SYSCTL_BASE + 0x058)
#define SYSCTL_RESC      REG32(SYSCTL_BASE + 0x05C)
#define SYSCTL_MOSCCTL   REG32(SYSCTL_BASE + 0x07C)
#define SYSCTL_DSLPCLKCFG REG32(SYSCTL_BASE + 0x144)
#define SYSCTL_PLLFREQ0  REG32(SYSCTL_BASE + 0x160)
#define SYSCTL_PLLFREQ1  REG32(SYSCTL_BASE + 0x164)
#define SYSCTL_PLLSTAT   REG32(SYSCTL_BASE + 0x168)
/* Run Mode Clock Gating Control */
#define SYSCTL_RCGCWD    REG32(SYSCTL_BASE + 0x600)
#define SYSCTL_RCGCTIMER REG32(SYSCTL_BASE + 0x604)
#define SYSCTL_RCGCGPIO  REG32(SYSCTL_BASE + 0x608)
#define SYSCTL_RCGCDMA   REG32(SYSCTL_BASE + 0x60C)
#define SYSCTL_RCGCHIB   REG32(SYSCTL_BASE + 0x614)
#define SYSCTL_RCGCUART  REG32(SYSCTL_BASE + 0x618)
#define SYSCTL_RCGCSSI   REG32(SYSCTL_BASE + 0x61C)
#define SYSCTL_RCGCI2C   REG32(SYSCTL_BASE + 0x620)
#define SYSCTL_RCGCCAN   REG32(SYSCTL_BASE + 0x634)
#define SYSCTL_RCGCADC   REG32(SYSCTL_BASE + 0x638)
#define SYSCTL_RCGCPWM   REG32(SYSCTL_BASE + 0x640)
#define SYSCTL_RCGCQEI   REG32(SYSCTL_BASE + 0x644)
#define SYSCTL_RCGCEEPROM REG32(SYSCTL_BASE + 0x658)
#define SYSCTL_RCGCWTIMER REG32(SYSCTL_BASE + 0x65C)  /* só 123 */
/* Peripheral Ready: espere o bit ir a 1 antes de acessar o periférico */
#define SYSCTL_PRWD      REG32(SYSCTL_BASE + 0xA00)
#define SYSCTL_PRTIMER   REG32(SYSCTL_BASE + 0xA04)
#define SYSCTL_PRGPIO    REG32(SYSCTL_BASE + 0xA08)
#define SYSCTL_PRDMA     REG32(SYSCTL_BASE + 0xA0C)
#define SYSCTL_PRHIB     REG32(SYSCTL_BASE + 0xA14)
#define SYSCTL_PRUART    REG32(SYSCTL_BASE + 0xA18)
#define SYSCTL_PRSSI     REG32(SYSCTL_BASE + 0xA1C)
#define SYSCTL_PRI2C     REG32(SYSCTL_BASE + 0xA20)
#define SYSCTL_PRCAN     REG32(SYSCTL_BASE + 0xA34)
#define SYSCTL_PRADC     REG32(SYSCTL_BASE + 0xA38)
#define SYSCTL_PRPWM     REG32(SYSCTL_BASE + 0xA40)
#define SYSCTL_PRQEI     REG32(SYSCTL_BASE + 0xA44)
#define SYSCTL_PRWTIMER  REG32(SYSCTL_BASE + 0xA5C)

/* Liga o clock de um periférico e espera ele ficar pronto.
 * Ex.: periph_enable(&SYSCTL_RCGCUART, &SYSCTL_PRUART, 0); */
static inline void periph_enable(volatile uint32_t *rcgc, volatile uint32_t *pr, uint32_t n)
{
    *rcgc |= (1U << n);
    while (!(*pr & (1U << n))) { }
}

/* RESC: causa do reset */
#define SYSCTL_RESC_EXT  (1U << 0)
#define SYSCTL_RESC_POR  (1U << 1)
#define SYSCTL_RESC_BOR  (1U << 2)
#define SYSCTL_RESC_WDT0 (1U << 3)
#define SYSCTL_RESC_SW   (1U << 4)
#define SYSCTL_RESC_WDT1 (1U << 5)
#define SYSCTL_RESC_HIB  (1U << 6)   /* 129 */
#define SYSCTL_RESC_MOSCFAIL (1U << 16)

/* ===========================================================================
 *  Funções inline do núcleo
 * ======================================================================== */
static inline void __enable_irq(void)  { __asm volatile ("cpsie i" ::: "memory"); }
static inline void __disable_irq(void) { __asm volatile ("cpsid i" ::: "memory"); }
static inline void __WFI(void)         { __asm volatile ("wfi"); }
static inline void __NOP(void)         { __asm volatile ("nop"); }
static inline void __DSB(void)         { __asm volatile ("dsb 0xF" ::: "memory"); }
static inline void __ISB(void)         { __asm volatile ("isb 0xF" ::: "memory"); }

#define NVIC_PRIO_BITS 3   /* TM4C: 8 níveis de prioridade (bits [7:5]) */

static inline void NVIC_EnableIRQ(uint32_t irq)  { NVIC->ISER[irq >> 5] = 1U << (irq & 0x1FU); }
static inline void NVIC_DisableIRQ(uint32_t irq) { NVIC->ICER[irq >> 5] = 1U << (irq & 0x1FU); __DSB(); __ISB(); }
static inline void NVIC_ClearPendingIRQ(uint32_t irq) { NVIC->ICPR[irq >> 5] = 1U << (irq & 0x1FU); }
static inline void NVIC_SetPriority(uint32_t irq, uint32_t prio)   /* 0..7 */
{
    NVIC->IP[irq] = (uint8_t)((prio << (8U - NVIC_PRIO_BITS)) & 0xFFU);
}
static inline void NVIC_SystemReset(void)
{
    __DSB();
    SCB->AIRCR = SCB_AIRCR_VECTKEY | SCB_AIRCR_SYSRESETREQ;
    __DSB();
    for (;;) { }
}

#endif /* TM4C_COMMON_H */
