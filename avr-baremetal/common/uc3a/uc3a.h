/*
 * uc3a.h - Registradores do AT32UC3A0512 (AVR32 UC3, 32 bits)
 *
 * Escrito à mão para o curso a partir do datasheet "AT32UC3A Series"
 * (doc32058) e do "AVR32 Architecture Document" (doc32000). Não usa o ASF
 * (Atmel Software Framework) nem os headers <avr32/io.h>.
 *
 * Os periféricos do UC3A (USART, SPI, TWI, TC, PWM, ADC...) são os mesmos
 * blocos usados nos ARM SAM7 da Atmel: quem conhece um, conhece o outro.
 */
#ifndef UC3A_H
#define UC3A_H

#include <stdint.h>

#define __IO volatile
#define __I  volatile const
#define REG32(a) (*(volatile uint32_t *)(a))

/* ---------------- Mapa de memória ---------------- */
#define SRAM_BASE      0x00000000UL   /* 64 KB de SRAM interna (0x0000_0000-0x0000_FFFF) */
#define FLASH_BASE     0x80000000UL   /* 512 KB de Flash */
#define USERPAGE_BASE  0x80800000UL   /* "user page": 512 bytes de Flash à parte */
#define FLASH_PAGE_SIZE 512UL

/* ---------------- Registradores de sistema do núcleo (mfsr/mtsr) ---------------- */
#define SYSREG_SR       0     /* status: bit 16 GM (máscara global de interrupções), 21 EM (exceções) */
#define SYSREG_EVBA     4     /* endereço base da tabela de exceções */
#define SYSREG_COUNT    264   /* contador de ciclos (incrementa a cada clock da CPU) */
#define SYSREG_COMPARE  268   /* quando COUNT == COMPARE: interrupção (grupo 0) */

#define mfsr(r)     __builtin_mfsr(r)
#define mtsr(r, v)  __builtin_mtsr((r), (v))
static inline void irq_enable(void)  { __asm__ __volatile__ ("csrf 16" ::: "memory"); }  /* GM = 0 */
static inline void irq_disable(void) { __asm__ __volatile__ ("ssrf 16" ::: "memory"); __asm__ __volatile__ ("nop\n nop"); }
static inline void nop(void)         { __asm__ __volatile__ ("nop"); }

/* Rotina de interrupção: o atributo faz o GCC terminar a função com "rete" */
#define ISR_ATTR __attribute__((__interrupt__))

/* ---------------- GPIO (0xFFFF1000, uma "porta" de 32 pinos a cada 0x100) ----------------
 * Quase todo registrador tem 4 endereços: valor, SET (+4: escreve 1 = liga bits),
 * CLR (+8: escreve 1 = desliga bits) e TGL (+C: escreve 1 = inverte bits).
 * Assim dá para mexer num pino sem ler-modificar-escrever. */
typedef struct { __IO uint32_t VAL, SET, CLR, TGL; } GPIO_REG4;

typedef struct {
    GPIO_REG4 GPER;   /* 0x00 1 = pino controlado pelo GPIO; 0 = por um periférico */
    GPIO_REG4 PMR0;   /* 0x10 escolha do periférico (bit 0 da função: A=00, B=01, C=10) */
    GPIO_REG4 PMR1;   /* 0x20 (bit 1 da função) */
    uint32_t  RES0[4];
    GPIO_REG4 ODER;   /* 0x40 1 = saída */
    GPIO_REG4 OVR;    /* 0x50 valor de saída */
    GPIO_REG4 PVR;    /* 0x60 valor lido do pino (só VAL) */
    GPIO_REG4 PUER;   /* 0x70 pull-up */
    GPIO_REG4 ODMER;  /* 0x80 open-drain */
    GPIO_REG4 IER;    /* 0x90 interrupção habilitada */
    GPIO_REG4 IMR0;   /* 0xA0 modo da interrupção: 00 mudança, 01 subida, 10 descida */
    GPIO_REG4 IMR1;   /* 0xB0 */
    GPIO_REG4 GFER;   /* 0xC0 filtro de glitch */
    GPIO_REG4 IFR;    /* 0xD0 flags de interrupção (VAL lê, CLR em 0xD8 limpa) */
    uint32_t  RES1[8];
} GPIO_Port;

#define GPIO_BASE       0xFFFF1000UL
#define GPIO_PORT(pin)  ((GPIO_Port *)(GPIO_BASE + 0x100UL * ((pin) >> 5)))
#define GPIO_MASK(pin)  (1UL << ((pin) & 31U))
/* número do pino: PA0..PA31 = 0..31, PB0..PB31 = 32..63, PC0..PC5 = 64..69 */
#define PA(n) (n)
#define PB(n) (32 + (n))
#define PC(n) (64 + (n))

/* ---------------- INTC - controlador de interrupções (0xFFFF0800) ---------------- */
typedef struct {
    __IO uint32_t IPR[64];   /* 0x000 prioridade + autovetor de cada GRUPO */
    __I  uint32_t IRR[64];   /* 0x100 linhas pendentes de cada grupo */
    __I  uint32_t ICR[4];    /* 0x200 grupo que causou a interrupção: ICR[0] = nível 3 ... ICR[3] = nível 0 */
} INTC_TypeDef;
#define INTC ((INTC_TypeDef *) 0xFFFF0800UL)
#define INTC_IPR_INTLEVEL_Pos 30

/* Grupos de interrupção (datasheet, "Interrupt Request Signal Map") */
#define IRQ_GRP_COMPARE 0
#define IRQ_GRP_EIC     1
#define IRQ_GRP_GPIO    2    /* linha = pino / 8 */
#define IRQ_GRP_PDCA    3
#define IRQ_GRP_PM      4
#define IRQ_GRP_RTC     5
#define IRQ_GRP_USART0  6
#define IRQ_GRP_USART1  7
#define IRQ_GRP_USART2  8
#define IRQ_GRP_USART3  9
#define IRQ_GRP_SPI0    10
#define IRQ_GRP_SPI1    11
#define IRQ_GRP_TWI     12
#define IRQ_GRP_PWM     13
#define IRQ_GRP_SSC     14
#define IRQ_GRP_TC      15   /* linhas 0, 1, 2 = canais do TC */
#define IRQ_GRP_ADC     16

/* ---------------- PM - Power Manager: clocks e reset (0xFFFF0C00) ---------------- */
typedef struct {
    __IO uint32_t MCCTRL;    /* 0x00 clock principal: MCSEL[1:0] 0 RC 115 kHz, 1 OSC0, 2 PLL0; OSC0EN bit 2 */
    __IO uint32_t CKSEL;     /* 0x04 divisores CPU/HSB/PBA/PBB */
    __IO uint32_t CPUMASK;   /* 0x08 clocks liberados (1 = ligado) */
    __IO uint32_t HSBMASK;   /* 0x0C */
    __IO uint32_t PBAMASK;   /* 0x10 */
    __IO uint32_t PBBMASK;   /* 0x14 */
    uint32_t      RES0[2];
    __IO uint32_t PLL0;      /* 0x20 */
    __IO uint32_t PLL1;      /* 0x24 */
    __IO uint32_t OSCCTRL0;  /* 0x28 */
    __IO uint32_t OSCCTRL1;  /* 0x2C */
    __IO uint32_t OSCCTRL32; /* 0x30 */
    uint32_t      RES1[3];
    __IO uint32_t IER;       /* 0x40 */
    __IO uint32_t IDR;       /* 0x44 */
    __I  uint32_t IMR;       /* 0x48 */
    __I  uint32_t ISR;       /* 0x4C */
    __IO uint32_t ICR;       /* 0x50 */
    __I  uint32_t POSCSR;    /* 0x54 status: LOCK0 bit 0, OSC0RDY bit 7 */
    uint32_t      RES2[42];
    __IO uint32_t RCCR;      /* 0x100 calibração do RC */
    uint32_t      RES3[15];
    __I  uint32_t RCAUSE;    /* 0x140 causa do último reset */
} PM_TypeDef;
#define PM ((PM_TypeDef *) 0xFFFF0C00UL)
#define PM_MCCTRL_MCSEL_Msk  3UL
#define PM_MCCTRL_MCSEL_PLL0 2UL
#define PM_MCCTRL_OSC0EN     (1UL << 2)
#define PM_POSCSR_LOCK0      (1UL << 0)
#define PM_POSCSR_OSC0RDY    (1UL << 7)
#define PM_RCAUSE_POR  (1UL << 0)
#define PM_RCAUSE_BOD  (1UL << 1)
#define PM_RCAUSE_EXT  (1UL << 2)
#define PM_RCAUSE_WDT  (1UL << 3)
#define PM_RCAUSE_JTAG (1UL << 4)
#define PM_RCAUSE_CPUERR (1UL << 7)

/* Sleep: instrução "sleep N" (0 idle, 1 frozen, 2 standby, 3 stop, 5 deepstop, 6 static) */
#define cpu_sleep(mode) __asm__ __volatile__ ("sleep %0" :: "i"(mode) : "memory")

/* ---------------- FLASHC (0xFFFE1400) ---------------- */
typedef struct {
    __IO uint32_t FCR;    /* 0x00 FWS bit 6 = 1 wait state (CPU > 33 MHz) */
    __IO uint32_t FCMD;   /* 0x04 comando: CMD[5:0], PAGEN[23:8], KEY[31:24] = 0xA5 */
    __I  uint32_t FSR;    /* 0x08 FRDY bit 0, LOCKE bit 2, PROGE bit 3 */
    __I  uint32_t FGPFRHI;
    __I  uint32_t FGPFRLO;
} FLASHC_TypeDef;
#define FLASHC ((FLASHC_TypeDef *) 0xFFFE1400UL)
#define FLASHC_FCR_FWS    (1UL << 6)
#define FLASHC_FSR_FRDY   (1UL << 0)
#define FLASHC_FSR_LOCKE  (1UL << 2)
#define FLASHC_FSR_PROGE  (1UL << 3)
#define FLASHC_KEY        (0xA5UL << 24)
#define FLASHC_CMD_WP   1    /* write page        */
#define FLASHC_CMD_EP   2    /* erase page        */
#define FLASHC_CMD_CPB  3    /* clear page buffer */
#define FLASHC_CMD_WUP  13   /* write user page   */
#define FLASHC_CMD_EUP  14   /* erase user page   */

/* ---------------- USART (0xFFFF1400 + 0x400 * n) ---------------- */
typedef struct {
    __IO uint32_t CR;     /* 0x00 */
    __IO uint32_t MR;     /* 0x04 */
    __IO uint32_t IER;    /* 0x08 */
    __IO uint32_t IDR;    /* 0x0C */
    __I  uint32_t IMR;    /* 0x10 */
    __I  uint32_t CSR;    /* 0x14 */
    __I  uint32_t RHR;    /* 0x18 */
    __IO uint32_t THR;    /* 0x1C */
    __IO uint32_t BRGR;   /* 0x20 CD[15:0], FP[18:16] */
    __IO uint32_t RTOR;   /* 0x24 */
    __IO uint32_t TTGR;   /* 0x28 */
} USART_TypeDef;
#define USART0 ((USART_TypeDef *) 0xFFFF1400UL)
#define USART1 ((USART_TypeDef *) 0xFFFF1800UL)
#define USART_CR_RSTRX  (1UL << 2)
#define USART_CR_RSTTX  (1UL << 3)
#define USART_CR_RXEN   (1UL << 4)
#define USART_CR_RXDIS  (1UL << 5)
#define USART_CR_TXEN   (1UL << 6)
#define USART_CR_TXDIS  (1UL << 7)
#define USART_CR_RSTSTA (1UL << 8)
#define USART_MR_CHRL_8   (3UL << 6)
#define USART_MR_PAR_NONE (4UL << 9)
#define USART_CSR_RXRDY   (1UL << 0)
#define USART_CSR_TXRDY   (1UL << 1)
#define USART_CSR_OVRE    (1UL << 5)
#define USART_CSR_TXEMPTY (1UL << 9)

/* ---------------- SPI (0xFFFF2400 / 0xFFFF2800) ---------------- */
typedef struct {
    __IO uint32_t CR;     /* 0x00 SPIEN 0, SPIDIS 1, SWRST 7, LASTXFER 24 */
    __IO uint32_t MR;     /* 0x04 MSTR 0, PS 1, PCSDEC 2, MODFDIS 4, LLB 7, PCS[19:16] */
    __I  uint32_t RDR;    /* 0x08 */
    __IO uint32_t TDR;    /* 0x0C */
    __I  uint32_t SR;     /* 0x10 RDRF 0, TDRE 1, MODF 2, OVRES 3, TXEMPTY 9 */
    __IO uint32_t IER;
    __IO uint32_t IDR;
    __I  uint32_t IMR;
    uint32_t      RES[4];
    __IO uint32_t CSR[4]; /* 0x30 CPOL 0, NCPHA 1, CSAAT 3, BITS[7:4], SCBR[15:8], DLYBS, DLYBCT */
} SPI_TypeDef;
#define SPI0 ((SPI_TypeDef *) 0xFFFF2400UL)
#define SPI1 ((SPI_TypeDef *) 0xFFFF2800UL)

/* ---------------- TWI (I2C) (0xFFFF2C00) ---------------- */
typedef struct {
    __IO uint32_t CR;     /* 0x00 START 0, STOP 1, MSEN 2, MSDIS 3, SVDIS 5, SWRST 7 */
    __IO uint32_t MMR;    /* 0x04 IADRSZ[9:8], MREAD 12, DADR[22:16] */
    __IO uint32_t SMR;    /* 0x08 */
    __IO uint32_t IADR;   /* 0x0C endereço interno (registrador do escravo) */
    __IO uint32_t CWGR;   /* 0x10 CLDIV[7:0], CHDIV[15:8], CKDIV[18:16] */
    uint32_t      RES[3];
    __I  uint32_t SR;     /* 0x20 TXCOMP 0, RXRDY 1, TXRDY 2, NACK 8 */
    __IO uint32_t IER;
    __IO uint32_t IDR;
    __I  uint32_t IMR;
    __I  uint32_t RHR;    /* 0x30 */
    __IO uint32_t THR;    /* 0x34 */
} TWI_TypeDef;
#define TWI ((TWI_TypeDef *) 0xFFFF2C00UL)

/* ---------------- PWM (0xFFFF3000) ---------------- */
typedef struct {
    __IO uint32_t CMR;    /* +0x00 CPRE[3:0], CALG 8, CPOL 9 */
    __IO uint32_t CDTY;   /* +0x04 duty */
    __IO uint32_t CPRD;   /* +0x08 período */
    __I  uint32_t CCNT;   /* +0x0C */
    __IO uint32_t CUPD;   /* +0x10 atualização sincronizada */
    uint32_t      RES[3];
} PWM_CH_TypeDef;
typedef struct {
    __IO uint32_t MR;     /* 0x00 DIVA/PREA/DIVB/PREB */
    __IO uint32_t ENA;    /* 0x04 liga canais */
    __IO uint32_t DIS;    /* 0x08 */
    __I  uint32_t SR;     /* 0x0C */
    __IO uint32_t IER, IDR;
    __I  uint32_t IMR, ISR;
    uint32_t      RES[120];
    PWM_CH_TypeDef CH[7]; /* 0x200 */
} PWM_TypeDef;
#define PWM ((PWM_TypeDef *) 0xFFFF3000UL)

/* ---------------- TC - Timer/Counter (0xFFFF3800), 3 canais ---------------- */
typedef struct {
    __IO uint32_t CCR;    /* +0x00 CLKEN 0, CLKDIS 1, SWTRG 2 */
    __IO uint32_t CMR;    /* +0x04 modo */
    uint32_t      RES0[2];
    __I  uint32_t CV;     /* +0x10 contador */
    __IO uint32_t RA;     /* +0x14 */
    __IO uint32_t RB;     /* +0x18 */
    __IO uint32_t RC;     /* +0x1C */
    __I  uint32_t SR;     /* +0x20 COVFS 0, LOVRS 1, CPAS 2, CPBS 3, CPCS 4, LDRAS 5, LDRBS 6 */
    __IO uint32_t IER;    /* +0x24 */
    __IO uint32_t IDR;    /* +0x28 */
    __I  uint32_t IMR;    /* +0x2C */
    uint32_t      RES1[4];
} TC_CH_TypeDef;
typedef struct {
    TC_CH_TypeDef CH[3];
    __IO uint32_t BCR;    /* 0xC0 */
    __IO uint32_t BMR;    /* 0xC4 */
} TC_TypeDef;
#define TC ((TC_TypeDef *) 0xFFFF3800UL)
#define TC_CCR_CLKEN  (1UL << 0)
#define TC_CCR_CLKDIS (1UL << 1)
#define TC_CCR_SWTRG  (1UL << 2)
#define TC_SR_CPCS    (1UL << 4)
#define TC_SR_LDRAS   (1UL << 5)
#define TC_SR_LDRBS   (1UL << 6)
/* TCCLKS: 0 = 32 kHz, 1 = PBA/2, 2 = PBA/8, 3 = PBA/32, 4 = PBA/128 */

/* ---------------- ADC (0xFFFF3C00) ---------------- */
typedef struct {
    __IO uint32_t CR;     /* 0x00 SWRST 0, START 1 */
    __IO uint32_t MR;     /* 0x04 TRGEN 0, LOWRES 4, SLEEP 5, PRESCAL[15:8], STARTUP[22:16], SHTIM[27:24] */
    uint32_t      RES0[2];
    __IO uint32_t CHER;   /* 0x10 liga canais */
    __IO uint32_t CHDR;   /* 0x14 */
    __I  uint32_t CHSR;   /* 0x18 */
    __I  uint32_t SR;     /* 0x1C EOCn bits 0-7, DRDY bit 16 */
    __I  uint32_t LCDR;   /* 0x20 último resultado */
    __IO uint32_t IER, IDR;
    __I  uint32_t IMR;
    __I  uint32_t CDR[8]; /* 0x30 resultado de cada canal */
} ADC_TypeDef;
#define ADC ((ADC_TypeDef *) 0xFFFF3C00UL)

/* ---------------- PDCA - DMA (0xFFFF0000), canais a cada 0x40 ---------------- */
typedef struct {
    __IO uint32_t MAR;    /* +0x00 endereço da memória */
    __IO uint32_t PSR;    /* +0x04 qual periférico (PID) */
    __IO uint32_t TCR;    /* +0x08 quantidade de transferências */
    __IO uint32_t MARR;   /* +0x0C recarga */
    __IO uint32_t TCRR;   /* +0x10 */
    __IO uint32_t CR;     /* +0x14 TEN 0, TDIS 1, ECLR 8 */
    __IO uint32_t MR;     /* +0x18 SIZE[1:0]: 0 byte, 1 halfword, 2 word */
    __I  uint32_t SR;     /* +0x1C TEN bit 0 */
    __IO uint32_t IER, IDR;
    __I  uint32_t IMR;
    __I  uint32_t ISR;    /* +0x2C RCZ 0, TRC 1 (terminou), TERR 2 */
    uint32_t      RES[4];
} PDCA_CH_TypeDef;
#define PDCA_CH(n) ((PDCA_CH_TypeDef *)(0xFFFF0000UL + 0x40UL * (n)))
#define PDCA_PID_ADC        0
#define PDCA_PID_USART0_RX  2
#define PDCA_PID_USART0_TX  10
#define PDCA_ISR_TRC        (1UL << 1)

/* ---------------- WDT (0xFFFF0D30) ---------------- */
typedef struct {
    __IO uint32_t CTRL;   /* EN bit 0, PSEL[12:8], KEY[31:24] (escrever 0x55 e depois 0xAA) */
    __IO uint32_t CLR;    /* qualquer escrita "alimenta" o watchdog */
} WDT_TypeDef;
#define WDT ((WDT_TypeDef *) 0xFFFF0D30UL)

/* ---------------- EIC - interrupções externas (0xFFFF0D80) ---------------- */
typedef struct {
    __IO uint32_t IER, IDR;
    __I  uint32_t IMR, ISR;
    __IO uint32_t ICR;
    __IO uint32_t MODE;   /* 1 = nível */
    __IO uint32_t EDGE;   /* 1 = subida */
    __IO uint32_t LEVEL;
    __IO uint32_t FILTER;
    __IO uint32_t TEST;
    __IO uint32_t ASYNC;
    __IO uint32_t SCAN;
    __IO uint32_t EN, DIS;
    __I  uint32_t CTRL;
} EIC_TypeDef;
#define EIC ((EIC_TypeDef *) 0xFFFF0D80UL)

/* ---------------- Funções do controlador de interrupções do curso (intc.c) ---------------- */
typedef void (*irq_handler_t)(void);
void intc_init(void);
void intc_register(uint32_t group, irq_handler_t handler, uint32_t level);   /* level 0..3 */

/* Seleciona a função periférica (0 = A, 1 = B, 2 = C) de um pino */
static inline void gpio_periph(uint32_t pin, uint32_t func)
{
    GPIO_Port *p = GPIO_PORT(pin);
    uint32_t m = GPIO_MASK(pin);
    if (func & 1U) p->PMR0.SET = m; else p->PMR0.CLR = m;
    if (func & 2U) p->PMR1.SET = m; else p->PMR1.CLR = m;
    p->GPER.CLR = m;                  /* entrega o pino ao periférico */
}

#endif
