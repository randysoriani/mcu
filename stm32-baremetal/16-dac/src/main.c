/*
 * Módulo 16 - DAC
 *
 * DEMO 1 (padrão): senoide de 1 kHz em PA4 (Arduino A2)
 *     TIM6 gera um TRGO a 32 kHz -> cada TRGO o DAC pede um DMA -> o DMA
 *     copia a próxima amostra da tabela (32 pontos) para DHR12R1.
 *     CPU: 0% de uso.
 * DEMO 2: onda triangular gerada pelo PRÓPRIO DAC (WAVE1 = 10), sem tabela.
 *
 * Veja com osciloscópio em A2. Com um filtro RC (1 kΩ + 100 nF) a senoide
 * fica bem mais limpa.
 *
 *   make clean && make MCU=f0 EXTRA=-DDEMO=2
 */
#include "system.h"
#include "gpio.h"

#ifndef DEMO
#define DEMO 1
#endif

#define N_SAMPLES 32U
#define F_SINE    1000U

/* 2048 + 2000*sin(2*pi*i/32): 12 bits, centrada em VDDA/2 */
static const uint16_t sine12[N_SAMPLES] = {
    2048, 2438, 2813, 3159, 3462, 3711, 3896, 4010, 4048, 4010, 3896, 3711, 3462, 3159, 2813, 2438,
    2048, 1658, 1283,  937,  634,  385,  200,   86,   48,   86,  200,  385,  634,  937, 1283, 1658,
};

static uint32_t timclk(void)
{
#if defined(STM32F0)
    return APB1Clock;
#else
    return (APB1Clock == SystemCoreClock) ? APB1Clock : 2U * APB1Clock;
#endif
}

static void tim6_trgo(uint32_t hz)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    TIM6->CR1 = 0;
    TIM6->PSC = 0;
    TIM6->ARR = timclk() / hz - 1U;
    TIM6->CR2 = (2U << TIM_CR2_MMS_Pos);     /* MMS = 010: TRGO no evento de update */
    TIM6->EGR = TIM_EGR_UG;
    TIM6->CR1 = TIM_CR1_CEN;
}

#if defined(STM32F4)
static uint32_t sine32[N_SAMPLES];           /* F4: DMA em modo direto exige PSIZE = MSIZE */
#endif

__attribute__((unused)) static void dac_dma_sine(void)
{
#if defined(STM32F0)
    /* DMA1 canal 3 = TIM6_UP / DAC canal 1 (fixo no F07x) */
    RCC->AHBENR |= RCC_AHBENR_DMAEN;
    DMA_Channel_TypeDef *ch = &DMA1->CH[3 - 1];
    ch->CCR   = 0;
    ch->CPAR  = (uint32_t)&DAC->DHR12R1;
    ch->CMAR  = (uint32_t)sine12;
    ch->CNDTR = N_SAMPLES;
    /* memória 16 bits -> periférico 32 bits: o DMA do F0 completa com zeros */
    ch->CCR   = DMA_CCR_DIR | DMA_CCR_MINC | DMA_CCR_CIRC
              | (1U << DMA_CCR_MSIZE_Pos) | (2U << DMA_CCR_PSIZE_Pos)
              | (2U << DMA_CCR_PL_Pos) | DMA_CCR_EN;
#else
    /* DMA1 Stream5 Canal7 = DAC1 */
    for (unsigned i = 0; i < N_SAMPLES; i++) sine32[i] = sine12[i];
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    DMA_Stream_TypeDef *s = &DMA1->S[5];
    s->CR = 0;
    while (s->CR & DMA_SxCR_EN) { }
    DMA1->HIFCR = DMA_ALLF << dma_flag_shift(5);
    s->PAR  = (uint32_t)&DAC->DHR12R1;
    s->M0AR = (uint32_t)sine32;
    s->NDTR = N_SAMPLES;
    s->FCR  = 0;
    s->CR   = (7U << DMA_SxCR_CHSEL_Pos) | (1U << DMA_SxCR_DIR_Pos)
            | (2U << DMA_SxCR_MSIZE_Pos) | (2U << DMA_SxCR_PSIZE_Pos)
            | DMA_SxCR_MINC | DMA_SxCR_CIRC | (2U << DMA_SxCR_PL_Pos);
    s->CR  |= DMA_SxCR_EN;
#endif

    /* DAC canal 1: trigger externo (TEN1) = TIM6 TRGO (TSEL1 = 000), DMA ligado */
    DAC->CR = DAC_CR_TEN1 | (0U << DAC_CR_TSEL1_Pos) | DAC_CR_DMAEN1 | DAC_CR_EN1;
    tim6_trgo(N_SAMPLES * F_SINE);           /* 32 kHz */
}

__attribute__((unused)) static void dac_triangle(void)
{
    /* WAVE1 = 10 (triângulo), MAMP1 = 1011 (amplitude 4095), trigger TIM6.
     * A cada trigger o contador interno sobe/desce 1 LSB e é somado a DHR. */
    DAC->DHR12R1 = 0;
    DAC->CR = DAC_CR_TEN1 | (0U << DAC_CR_TSEL1_Pos)
            | (2U << DAC_CR_WAVE1_Pos) | (11U << DAC_CR_MAMP1_Pos) | DAC_CR_EN1;
    tim6_trgo(400000);                       /* 8190 passos/período -> ~49 Hz */
}

int main(void)
{
    system_init();
    uart_init(115200);

#if defined(STM32F0)
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
#else
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
#endif
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    /* PA4 analógico: desliga o buffer digital e evita consumo parasita */
    gpio_mode(GPIOA, 4, GPIO_MODE_ANALOG);

    kprintf("\n=== Modulo 16: DAC (saida em PA4 / A2) ===\n");
#if DEMO == 1
    dac_dma_sine();
    kprintf("senoide %u Hz, %u amostras, TIM6 ARR = %u\n", F_SINE, N_SAMPLES, TIM6->ARR);
#else
    dac_triangle();
    kprintf("triangular gerada pelo hardware do DAC\n");
#endif

    for (;;) {
        kprintf("DOR1 = %4u  %s\n", DAC->DOR1,
                (DAC->SR & DAC_SR_DMAUDR1) ? "(DMA underrun!)" : "");
        delay_ms(1000);
    }
}
