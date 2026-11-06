/*
 * Módulo 11 - Captura de entrada e contador de eventos (ATmega328P)
 *
 * Gerador de teste: Timer2 em Fast PWM com TOP = OCR2A (modo 7), saída
 * OC2B = pino 3 (PD3). Muda de frequência a cada 2 s.
 *
 * DEMO 1 (padrão): CAPTURA. Timer1 em modo normal a 2 MHz; o pino ICP1 =
 *   pino 8 (PB0) copia o TCNT1 para ICR1 na borda escolhida (ICES1).
 *   Trocando a borda a cada captura medimos período e tempo em alto.
 *   >>> jumper do pino 3 para o pino 8 <<<
 * DEMO 2: CONTADOR. Timer1 com clock externo no pino T1 = pino 5 (PD5):
 *   conta as bordas de subida. >>> jumper do pino 3 para o pino 5 <<<
 *
 *   make clean && make MCU=m328p EXTRA=-DDEMO=2
 */
#include "system.h"

#ifndef DEMO
#define DEMO 1
#endif

/* Gerador: período e alto em unidades de 4 us (16 MHz / 64) */
static void gerador(uint8_t periodo_4us, uint8_t alto_4us)
{
    DDRD  |= (1U << 3);
    TCCR2A = (1U << COM2B1) | (1U << WGM21) | (1U << WGM20);
    TCCR2B = (1U << WGM22) | (1U << CS22);       /* modo 7, ÷64 */
    OCR2A  = periodo_4us - 1U;                    /* TOP */
    OCR2B  = alto_4us - 1U;
}

#if DEMO == 1
static volatile uint16_t t_subida, periodo, alto;
static volatile uint8_t  pronto;

ISR(TIMER1_CAPT_vect)
{
    uint16_t t = ICR1;
    if (TCCR1B & (1U << ICES1)) {                 /* esta captura foi na SUBIDA */
        periodo  = t - t_subida;                  /* aritmética de 16 bits trata a volta */
        t_subida = t;
        pronto   = 1;
        TCCR1B  &= (uint8_t)~(1U << ICES1);       /* próxima: descida */
    } else {
        alto     = t - t_subida;
        TCCR1B  |= (1U << ICES1);                 /* próxima: subida */
    }
    TIFR1 = (1U << ICF1);    /* trocar a borda pode gerar uma captura falsa: limpa */
}

static void demo(void)
{
    static const uint8_t cfg[][2] = { {250, 63}, {125, 100}, {50, 10} };
    TCCR1A = 0;
    TCCR1B = (1U << ICNC1) | (1U << ICES1) | (1U << CS11);  /* filtro, subida, ÷8 = 2 MHz */
    TIMSK1 = (1U << ICIE1);
    kprintf("DEMO 1: captura (jumper pino 3 -> pino 8)\n");
    for (uint8_t i = 0;; i = (uint8_t)((i + 1U) % 3U)) {
        gerador(cfg[i][0], cfg[i][1]);
        kprintf("gerando: periodo %u us, alto %u us\n", cfg[i][0] * 4U, cfg[i][1] * 4U);
        delay_ms(500);
        uint8_t s = SREG; cli(); uint16_t p = periodo, a = alto; SREG = s;
        if (pronto) kprintf("medido : periodo %u us, alto %u us\n", p / 2U, a / 2U);
        delay_ms(1500);
    }
}

#else
static void demo(void)
{
    static const uint8_t per[] = { 250, 100, 25 };
    TCCR1A = 0;
    TCCR1B = (1U << CS12) | (1U << CS11) | (1U << CS10);    /* clock externo em T1, borda de subida */
    kprintf("DEMO 2: contador (jumper pino 3 -> pino 5)\n");
    for (uint8_t i = 0;; i = (uint8_t)((i + 1U) % 3U)) {
        gerador(per[i], per[i] / 2U);
        uint16_t a = TCNT1;
        delay_ms(1000);
        uint16_t b = TCNT1;
        kprintf("gerando %u Hz -> contou %u bordas em 1 s\n",
                (unsigned)(250000UL / per[i]), (unsigned)(uint16_t)(b - a));
    }
}
#endif

int main(void)
{
    system_init();
    uart_init(115200);
    kprintf("\n=== Modulo 11: captura e contador ===\n");
    demo();
}
