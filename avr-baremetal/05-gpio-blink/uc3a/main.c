/*
 * Módulo 05 - Hello World: pisca o LED0 da EVK1100 (PB27, aceso em nível BAIXO).
 * Programa autocontido. Roda no oscilador RC de reset (~115 kHz).
 */
#include "device.h"

#define LED  PB(27)          /* GPIO 59: porta 1 (PB), bit 27 */

static void delay(volatile uint32_t n)
{
    while (n--) { __asm__ __volatile__ ("nop"); }
}

int main(void)
{
    GPIO_Port *p = GPIO_PORT(LED);            /* 0xFFFF1100: bloco da porta B */
    uint32_t   m = GPIO_MASK(LED);            /* 1 << 27 */

    p->GPER.SET = m;          /* 1) pino controlado pelo GPIO (não por um periférico) */
    p->OVR.SET  = m;          /* 2) valor inicial 1 = LED apagado                    */
    p->ODER.SET = m;          /* 3) saída                                            */

    for (;;) {
        p->OVR.TGL = m;       /* inverte só o pino 27: uma escrita, sem ler antes */
        delay(3000);
    }
}
