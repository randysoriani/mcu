/*
 * Módulo 17 - I2S por SOFTWARE: TIMER0 + uDMA escrevendo no GPIO
 *
 * A família TM4C não tem periférico I2S. Aqui o uDMA, disparado pelo TIMER0,
 * copia uma sequência pré-calculada de bytes para o registrador DATA de uma
 * porta GPIO, gerando BCLK, LRCLK (WS) e SD bit a bit:
 *
 *   cada bit I2S = 2 escritas: [BCLK=0, WS, SD] e [BCLK=1, WS, SD]
 *   1 quadro estéreo = 2 x 16 bits = 32 bits = 64 escritas
 *   Fs = 15 625 Hz -> 1 000 000 escritas/s -> TIMER0 a 1 MHz
 *   (123: 80 MHz/80, 129: 120 MHz/120 -> divisões exatas)
 *
 *          BCLK   WS    SD    porta (acesso mascarado ao DATA)
 *   123:   PE1    PE2   PE3   GPIOE
 *   129:   PK0    PK1   PK2   GPIOK
 *
 * DAC sugerido: MAX98357A (aceita 8-96 kHz) ou PCM5102A (SCK ao GND: PLL
 * interno a partir do BCLK de 32 fs). Tom gerado: 15625/16 = 976,6 Hz.
 *
 * Os 16 quadros do tom = 1024 bytes = exatamente uma transferência uDMA.
 * Modo PING-PONG: primária e alternativa apontam para o mesmo buffer; quando
 * uma termina, o laço principal a re-arma enquanto a outra roda.
 *
 * ESTADO: técnica didática, não validada em hardware neste curso.
 */
#include "system.h"
#include "gpio.h"

#if defined(TM4C123)
#  define I2S_PORT  GPIOE
#  define I2S_PORTN PORTE
#  define B_BCLK 1
#  define B_WS   2
#  define B_SD   3
#else
#  define I2S_PORT  GPIOK
#  define I2S_PORTN PORTK
#  define B_BCLK 0
#  define B_WS   1
#  define B_SD   2
#endif
#define PIN_MASK ((1U << B_BCLK) | (1U << B_WS) | (1U << B_SD))
#define CH_TIMER0A 18U          /* canal 18, encoding 0 = Timer 0A */
#define FRAMES 16U
#define NBYTES (FRAMES * 64U)   /* 1024 */

static UDMA_Entry ctl_table[64] __attribute__((aligned(1024)));
static uint8_t pattern[NBYTES];

static const int16_t sine16[FRAMES] = {
    0, 4592, 8485, 11087, 12000, 11087, 8485, 4592, 0, -4592, -8485, -11087, -12000, -11087, -8485, -4592,
};

/* Philips I2S: WS muda 1 BCLK ANTES do MSB; dado muda na descida, é lido na subida */
static void build_pattern(void)
{
    uint32_t k = 0;
    for (uint32_t f = 0; f < FRAMES; f++) {
        uint16_t L = (uint16_t)sine16[f], R = (uint16_t)(sine16[f] / 2);
        for (uint32_t slot = 0; slot < 32; slot++) {
            uint16_t word = (slot < 16) ? L : R;
            uint32_t bit  = (word >> (15U - (slot & 15U))) & 1U;
            uint32_t ws   = (((slot + 1U) & 31U) >= 16U) ? 1U : 0U;
            uint8_t  base = (uint8_t)((ws << B_WS) | (bit << B_SD));
            pattern[k++] = base;                          /* BCLK = 0 */
            pattern[k++] = base | (uint8_t)(1U << B_BCLK);/* BCLK = 1 */
        }
    }
}

static void arm(UDMA_Entry *e)
{
    e->SRCENDP = &pattern[NBYTES - 1U];
    e->DSTENDP = &I2S_PORT->DATA_BITS[PIN_MASK];
    e->CHCTL   = UDMA_CHCTL(UDMA_INC_NONE, UDMA_SIZE_8, UDMA_INC_8, UDMA_SIZE_8,
                            0, NBYTES, UDMA_MODE_PINGPONG);
}

int main(void)
{
    system_init();
    uart_init(115200);
    build_pattern();

    gpio_port_enable(I2S_PORTN);
    gpio_output(I2S_PORT, B_BCLK);
    gpio_output(I2S_PORT, B_WS);
    gpio_output(I2S_PORT, B_SD);
    I2S_PORT->DR8R |= PIN_MASK;

    periph_enable(&SYSCTL_RCGCDMA, &SYSCTL_PRDMA, 0);
    UDMA->CFG     = 1U;
    UDMA->CTLBASE = (uint32_t)ctl_table;
    UDMA->CHMAP[2] &= ~(0xFU << 8);                 /* canal 18 -> encoding 0 */
    arm(&ctl_table[CH_TIMER0A]);                    /* primária    */
    arm(&ctl_table[32 + CH_TIMER0A]);               /* alternativa */
    UDMA->ALTCLR = 1U << CH_TIMER0A;                /* começa pela primária */
    UDMA->ENASET = 1U << CH_TIMER0A;

    periph_enable(&SYSCTL_RCGCTIMER, &SYSCTL_PRTIMER, 0);
    TIMER0->CTL   = 0;
    TIMER0->CFG   = TIMER_CFG_32;
    TIMER0->TAMR  = TIMER_TAMR_PERIOD;
    TIMER0->TAILR = SystemCoreClock / 1000000U - 1U;   /* 1 MHz */
#if defined(TM4C129)
    TIMER0->DMAEV = TIMER_DMAEV_TATODMAEN;          /* 129: habilita o pedido de DMA no timeout */
#endif
    TIMER0->CTL   = TIMER_CTL_TAEN;

    kprintf("\n=== Modulo 17: I2S por software (uDMA -> GPIO) ===\n");
    kprintf("Fs = 15625 Hz, tom = 976 Hz, buffer = %u bytes\n", NBYTES);

    uint32_t refills = 0, t = 0;
    for (;;) {
        /* Quando uma estrutura termina, o controlador zera o modo dela: re-arma */
        if ((ctl_table[CH_TIMER0A].CHCTL & 7U) == UDMA_MODE_STOP)      { arm(&ctl_table[CH_TIMER0A]); refills++; }
        if ((ctl_table[32 + CH_TIMER0A].CHCTL & 7U) == UDMA_MODE_STOP) { arm(&ctl_table[32 + CH_TIMER0A]); refills++; }
        if (!(UDMA->ENASET & (1U << CH_TIMER0A))) {     /* atrasou: canal parou */
            UDMA->ENASET = 1U << CH_TIMER0A;
        }
        if (millis() - t >= 2000) { t = millis(); kprintf("buffers re-armados: %u\n", refills); }
    }
}
