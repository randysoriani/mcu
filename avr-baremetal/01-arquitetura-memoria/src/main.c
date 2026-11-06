/*
 * Módulo 01 - Arquitetura e organização da memória (ATmega328P e AT32UC3A)
 *
 * Imprime na serial onde cada tipo de objeto C foi parar e alguns
 * registradores que revelam a arquitetura de cada chip.
 */
#include "system.h"

const uint16_t tabela_const[4] = { 1, 2, 3, 4 };   /* AVR8: vai para a RAM! (módulo 03) */
uint16_t       var_inicializada = 0xCAFE;          /* .data */
uint16_t       var_zerada;                         /* .bss  */

int main(void)
{
    uint16_t var_local = 42;                       /* pilha */

    system_init();
    uart_init(115200);
    led_init();

    kprintf("\n=== Modulo 01: arquitetura e memoria ===\n");

#if defined(AVR8)
    kprintf("ATmega328P: AVR de 8 bits, arquitetura Harvard\n");
    kprintf("  sizeof(int) = %u bytes, sizeof(long) = %u, sizeof(void*) = %u\n",
            (unsigned)sizeof(int), (unsigned)sizeof(long), (unsigned)sizeof(void *));
    /* Os 32 registradores r0..r31 estão mapeados no começo do espaço de
     * dados: ler o endereço 0x0018 é ler o registrador r24. */
    kprintf("  registrador r24 visto pelo endereco 0x18: 0x%02x\n", *(volatile uint8_t *)0x18);
    kprintf("  SREG = 0x%02x (bit 7 = interrupcoes ligadas)\n", SREG);
    kprintf("  SP   = 0x%04x (pilha no fim da RAM, RAMEND = 0x%04x)\n",
            (unsigned)((SPH << 8) | SPL), RAMEND);
#else
    kprintf("AT32UC3A0512: AVR32 de 32 bits, mapa de memoria unico\n");
    kprintf("  sizeof(int) = %u bytes, sizeof(long) = %u, sizeof(void*) = %u\n",
            (unsigned)sizeof(int), (unsigned)sizeof(long), (unsigned)sizeof(void *));
    kprintf("  COUNT (ciclos desde o ultimo tick) = %lu\n", (unsigned long)mfsr(SYSREG_COUNT));
    kprintf("  EVBA (tabela de excecoes) = 0x%08lx\n", (unsigned long)mfsr(SYSREG_EVBA));
    kprintf("  RCAUSE (causa do reset)   = 0x%08lx\n", (unsigned long)PM->RCAUSE);
#endif

    kprintf("\nOnde cada coisa mora:\n");
    kprintf("  main()           0x%lx (Flash)\n", (unsigned long)(uintptr_t)main);
    kprintf("  tabela_const     0x%lx\n", (unsigned long)(uintptr_t)tabela_const);
    kprintf("  var_inicializada 0x%lx = 0x%x\n", (unsigned long)(uintptr_t)&var_inicializada, var_inicializada);
    kprintf("  var_zerada       0x%lx = %u\n", (unsigned long)(uintptr_t)&var_zerada, var_zerada);
    kprintf("  var_local        0x%lx = %u (pilha)\n", (unsigned long)(uintptr_t)&var_local, var_local);
#if defined(AVR8)
    kprintf("\nNo AVR8 o endereco de main() e contado em PALAVRAS de 16 bits:\n");
    kprintf("  em bytes, main() esta em 0x%lx\n", 2UL * (unsigned long)(uintptr_t)main);
#endif

    for (;;) {
        led_toggle();
        delay_ms(500);
    }
}
