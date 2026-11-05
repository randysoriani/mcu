/*
 * Módulo 17 - I2S: tom de 1 kHz estéreo, 16 bits, ~48 kHz, via DMA circular
 *
 * Periférico: SPI2 em modo I2S mestre transmissor, padrão Philips.
 *   PB12 = WS (LRCLK)   PB13 = CK (BCLK)   PB15 = SD (DIN)
 *   (Nucleo: PB12/PB15 no conector morpho CN10, PB13 = CN10 pino 30)
 *
 * DAC I2S sugerido: PCM5102A (SCK do módulo ao GND, FMT=0, XSMT=3V3) ou
 * MAX98357A (amplificador classe D, ligue direto num alto-falante).
 *
 * Clock do I2S:
 *   F0: I2SxCLK = SYSCLK = 48 MHz -> Fs = 48e6 / (32 * (2*15 + 1)) = 48 387 Hz
 *   F4: PLLI2S: HSI/8 = 2 MHz x192 = 384 MHz /R=5 = 76,8 MHz
 *       Fs = 76,8e6 / (32 * (2*25 + 0)) = 48 000 Hz exatos
 */
#include "system.h"
#include "gpio.h"

/* 48 amostras de 12000*sin(): 1 período a 48 kHz = 1 kHz */
static const int16_t sine48[48] = {
        0,   1566,   3106,   4592,   6000,   7305,   8485,   9520,  10392,  11087,  11591,  11897,
    12000,  11897,  11591,  11087,  10392,   9520,   8485,   7305,   6000,   4592,   3106,   1566,
        0,  -1566,  -3106,  -4592,  -6000,  -7305,  -8485,  -9520, -10392, -11087, -11591, -11897,
   -12000, -11897, -11591, -11087, -10392,  -9520,  -8485,  -7305,  -6000,  -4592,  -3106,  -1566,
};

/* Quadro I2S: [L0, R0, L1, R1, ...] */
static int16_t audio[2 * 48];

static void i2s_pins(void)
{
#if defined(STM32F0)
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    const uint32_t af = 0;               /* PB12/13/15 = SPI2/I2S2 no AF0 */
#else
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    const uint32_t af = 5;               /* AF5 */
#endif
    gpio_af(GPIOB, 12, af);
    gpio_af(GPIOB, 13, af);
    gpio_af(GPIOB, 15, af);
    gpio_speed(GPIOB, 13, GPIO_SPEED_HIGH);
    gpio_speed(GPIOB, 15, GPIO_SPEED_HIGH);
}

#if defined(STM32F4)
static void plli2s_init(void)
{
    /* Fonte do PLLI2S = mesma do PLL principal (HSI 16 MHz) / PLLI2SM */
    RCC->CR &= ~RCC_CR_PLLI2SON;
    while (RCC->CR & RCC_CR_PLLI2SRDY) { }
    RCC->PLLI2SCFGR = (8U   << RCC_PLLI2SCFGR_M_Pos)   /* 2 MHz      */
                    | (192U << RCC_PLLI2SCFGR_N_Pos)   /* VCO 384 MHz */
                    | (0U   << RCC_PLLI2SCFGR_P_Pos)   /* /2 (SPDIF)  */
                    | (4U   << RCC_PLLI2SCFGR_Q_Pos)   /* /4 (SAI)    */
                    | (5U   << RCC_PLLI2SCFGR_R_Pos);  /* /5 -> 76,8 MHz para o I2S */
    /* DCKCFGR.I2S1SRC = 00: I2S do APB1 (SPI2/SPI3) usa PLLI2S_R (reset) */
    RCC->DCKCFGR &= ~(3U << RCC_DCKCFGR_I2S1SRC_Pos);
    RCC->CR |= RCC_CR_PLLI2SON;
    while (!(RCC->CR & RCC_CR_PLLI2SRDY)) { }
}
#endif

static void i2s_dma_start(void)
{
#if defined(STM32F0)
    /* DMA1 canal 5 = SPI2_TX (F07x) */
    RCC->AHBENR |= RCC_AHBENR_DMAEN;
    DMA_Channel_TypeDef *ch = &DMA1->CH[5 - 1];
    ch->CCR   = 0;
    ch->CPAR  = (uint32_t)&SPI2->DR;
    ch->CMAR  = (uint32_t)audio;
    ch->CNDTR = sizeof audio / sizeof audio[0];
    ch->CCR   = DMA_CCR_DIR | DMA_CCR_MINC | DMA_CCR_CIRC
              | (1U << DMA_CCR_MSIZE_Pos) | (1U << DMA_CCR_PSIZE_Pos)
              | (3U << DMA_CCR_PL_Pos) | DMA_CCR_EN;
#else
    /* DMA1 Stream4 Canal0 = SPI2_TX */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    DMA_Stream_TypeDef *s = &DMA1->S[4];
    s->CR = 0;
    while (s->CR & DMA_SxCR_EN) { }
    DMA1->HIFCR = DMA_ALLF << dma_flag_shift(4);
    s->PAR  = (uint32_t)&SPI2->DR;
    s->M0AR = (uint32_t)audio;
    s->NDTR = sizeof audio / sizeof audio[0];
    s->FCR  = 0;
    s->CR   = (0U << DMA_SxCR_CHSEL_Pos) | (1U << DMA_SxCR_DIR_Pos)
            | (1U << DMA_SxCR_MSIZE_Pos) | (1U << DMA_SxCR_PSIZE_Pos)
            | DMA_SxCR_MINC | DMA_SxCR_CIRC | (3U << DMA_SxCR_PL_Pos);
    s->CR  |= DMA_SxCR_EN;
#endif
}

static void i2s_init(void)
{
#if defined(STM32F0)
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    const uint32_t div = 15, odd = 1;
#else
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    plli2s_init();
    const uint32_t div = 25, odd = 0;
#endif
    SPI2->I2SCFGR = 0;                          /* I2SE = 0 para configurar */
    SPI2->I2SPR   = div | (odd ? SPI_I2SPR_ODD : 0U);   /* MCKOE = 0 */
    SPI2->I2SCFGR = SPI_I2SCFGR_I2SMOD                  /* modo I2S (não SPI)    */
                  | (2U << SPI_I2SCFGR_I2SCFG_Pos)      /* 10: mestre transmissor */
                  | (0U << SPI_I2SCFGR_I2SSTD_Pos)      /* 00: Philips            */
                  | (0U << SPI_I2SCFGR_DATLEN_Pos);     /* 00: 16 bits, CHLEN = 0 (16) */
    SPI2->CR2 = SPI_CR2_TXDMAEN;                /* pede DMA a cada TXE */
    i2s_dma_start();
    SPI2->I2SCFGR |= SPI_I2SCFGR_I2SE;          /* liga: começa a gerar CK e WS */
}

int main(void)
{
    system_init();
    uart_init(115200);

    for (int i = 0; i < 48; i++) {
        audio[2 * i]     = sine48[i];           /* esquerdo */
        audio[2 * i + 1] = sine48[i] / 2;       /* direito: metade do volume */
    }

    i2s_pins();
    i2s_init();

    kprintf("\n=== Modulo 17: I2S (WS=PB12 CK=PB13 SD=PB15) ===\n");
#if defined(STM32F0)
    kprintf("Fs = %u Hz\n", SystemCoreClock / (32U * 31U));
#else
    kprintf("Fs = %u Hz\n", 76800000U / (32U * 50U));
#endif
    for (;;) {
        kprintf("I2S SR = 0x%04X %s\n", SPI2->SR, (SPI2->SR & SPI_SR_UDR) ? "(underrun)" : "");
        delay_ms(2000);
    }
}
