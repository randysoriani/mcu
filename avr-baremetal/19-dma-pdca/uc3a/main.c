/*
 * Módulo 19 - DMA com o PDCA (só AT32UC3A; o ATmega328P não tem DMA)
 *
 * O PDCA (Peripheral DMA Controller) copia dados entre a memória e um
 * periférico sem a CPU. Cada canal é ligado a um periférico pelo PID.
 * Aqui: canal 0, PID 10 = USART0 TX. Um texto longo sai pela serial
 * enquanto a CPU conta quantas voltas de laço consegue dar "de graça".
 */
#include "system.h"

static const char texto[] =
    "Este texto sai pela USART0 via PDCA (DMA). Enquanto isso a CPU fica livre:\r\n"
    "ela so conta voltas de um laco. Sem DMA, ela teria de esperar cada byte\r\n"
    "(cerca de 87 us a 115200 bps) antes de escrever o proximo.\r\n";

static void pdca_tx(const void *buf, uint32_t n)
{
    PDCA_CH_TypeDef *ch = PDCA_CH(0);
    ch->CR  = 1UL << 1;                           /* TDIS: desliga para configurar */
    ch->PSR = PDCA_PID_USART0_TX;                 /* qual periférico */
    ch->MR  = 0;                                  /* SIZE = byte */
    ch->MAR = (uint32_t)buf;                      /* de onde ler */
    ch->TCR = n;                                  /* quantos bytes */
    ch->CR  = 1UL << 0;                           /* TEN: começa (a USART pede um byte a cada TXRDY) */
}

static int pdca_ocupado(void) { return !(PDCA_CH(0)->ISR & PDCA_ISR_TRC); }   /* TRC = terminou */

int main(void)
{
    system_init();
    uart_init(115200);
    kprintf("\n=== Modulo 19: PDCA (DMA) ===\n");
    for (;;) {
        uint32_t voltas = 0;
        pdca_tx(texto, sizeof texto - 1U);
        while (pdca_ocupado()) voltas++;
        while (!(USART0->CSR & USART_CSR_TXEMPTY)) { }
        kprintf("a CPU deu %lu voltas durante o envio por DMA\n\n", (unsigned long)voltas);
        delay_ms(2000);
    }
}
