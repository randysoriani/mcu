/*
 * Módulo 10 - PWM (AT32UC3A): o bloco PWM, 7 canais independentes
 *
 *   Canal 0 -> PB19 (função A): LED4 da EVK1100 "respirando", 1 kHz
 *   Canal 1 -> PB20 (função A): 50 Hz para servo, pulso de 1 a 2 ms
 *   (confira no esquemático da sua placa quais pinos levam aos LEDs)
 *
 * Clock do canal: CPRE = 5 -> PBA/32 = 1,5 MHz
 * Período = CPRD contagens; tempo em nível ativo = CDTY contagens.
 */
#include "system.h"

#define PWM_CLK (48000000UL / 32UL)

static void pwm_ch_init(uint32_t ch, uint32_t prd, uint32_t dty)
{
    PWM->CH[ch].CMR  = 5UL                        /* CPRE: MCK/32 */
                     | (1UL << 9);                /* CPOL = 1: começa em nível alto */
    PWM->CH[ch].CPRD = prd;
    PWM->CH[ch].CDTY = dty;
}

static void pwm_set_duty(uint32_t ch, uint32_t dty)
{
    PWM->CH[ch].CUPD = dty;   /* CPD = 0: o novo duty entra no fim do período (sem glitch) */
}

int main(void)
{
    system_init();
    uart_init(115200);

    gpio_periph(PB(19), 0);
    gpio_periph(PB(20), 0);
    pwm_ch_init(0, PWM_CLK / 1000UL, 0);          /* 1500 contagens = 1 kHz */
    pwm_ch_init(1, PWM_CLK / 50UL, 2250);         /* 30000 contagens = 50 Hz, 1,5 ms */
    PWM->ENA = (1UL << 0) | (1UL << 1);

    kprintf("\n=== Modulo 10: PWM ===\n");
    int32_t nivel = 0, passo = 1;
    uint32_t servo = 1500, t_servo = 0;
    for (;;) {
        nivel += passo;
        if (nivel >= 100 || nivel <= 0) passo = -passo;
        pwm_set_duty(0, (uint32_t)(nivel * nivel) * (PWM_CLK / 1000UL) / 10000UL);
        if (millis() - t_servo >= 200) {
            t_servo = millis();
            servo += 150;
            if (servo > 3000) servo = 1500;       /* 1500..3000 contagens = 1..2 ms */
            pwm_set_duty(1, servo);
        }
        delay_ms(10);
    }
}
