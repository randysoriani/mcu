/*
 * Módulo 11 - Captura de entrada (AT32UC3A): TC em modo "capture"
 *
 * Gerador: canal 1 do PWM em PB20 (função A), como no módulo 10.
 * Medição: canal 0 do TC em modo captura, entrada TIOA0 = PB23 (função A),
 *   clock PBA/8 = 6 MHz. RA é carregado na subida e RB na descida.
 *   >>> jumper de PB20 para PB23 <<<
 * (confira na tabela "GPIO Controller Function Multiplexing" do datasheet
 *  o pino TC-A0 do seu encapsulamento)
 */
#include "system.h"

static volatile uint32_t ra_ant, periodo, alto;
static volatile uint8_t  pronto;

ISR_ATTR static void tc_handler(void)
{
    uint32_t sr = TC->CH[0].SR;                    /* ler SR limpa as flags */
    if (sr & TC_SR_LDRAS) {                        /* subida: RA carregado */
        uint32_t ra = TC->CH[0].RA;
        periodo = (ra - ra_ant) & 0xFFFFUL;        /* contador de 16 bits */
        ra_ant  = ra;
        pronto  = 1;
    }
    if (sr & TC_SR_LDRBS) {                        /* descida: RB carregado */
        alto = (TC->CH[0].RB - ra_ant) & 0xFFFFUL;
    }
}

int main(void)
{
    system_init();
    uart_init(115200);

    /* Gerador de teste: PWM canal 1, 1,5 MHz de clock */
    gpio_periph(PB(20), 0);
    PWM->CH[1].CMR = 5UL | (1UL << 9);
    PWM->ENA = (1UL << 1);

    /* Captura */
    gpio_periph(PB(23), 0);
    TC->CH[0].CCR = TC_CCR_CLKDIS;
    TC->CH[0].CMR = (2UL << 0)                     /* TCCLKS = PBA/8 */
                  | (1UL << 16)                    /* LDRA = subida de TIOA */
                  | (2UL << 18);                   /* LDRB = descida de TIOA */
    TC->CH[0].IER = TC_SR_LDRAS | TC_SR_LDRBS;
    intc_register(IRQ_GRP_TC, tc_handler, 1);
    TC->CH[0].CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;

    kprintf("\n=== Modulo 11: captura (jumper PB20 -> PB23) ===\n");
    static const uint32_t cfg[][2] = { {1500, 375}, {750, 600}, {300, 60} };   /* em contagens de 1,5 MHz */
    for (unsigned i = 0;; i = (i + 1U) % 3U) {
        PWM->CH[1].CPRD = cfg[i][0];
        PWM->CH[1].CDTY = cfg[i][1];
        kprintf("gerando: periodo %lu us, alto %lu us\n",
                (unsigned long)(cfg[i][0] * 2UL / 3UL), (unsigned long)(cfg[i][1] * 2UL / 3UL));
        delay_ms(500);
        if (pronto) kprintf("medido : periodo %lu us, alto %lu us\n",
                            (unsigned long)(periodo / 6UL), (unsigned long)(alto / 6UL));
        delay_ms(1500);
    }
}
