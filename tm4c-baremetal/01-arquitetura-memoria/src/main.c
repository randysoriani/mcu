/*
 * Módulo 01 - Arquitetura e organização da memória (TM4C123 e TM4C129)
 *
 * Imprime na serial (115200 8N1, /dev/ttyACM0) a identificação do núcleo e do
 * chip, tamanhos de Flash/SRAM e onde cada tipo de objeto C foi parar.
 * Pisca o LED usando BIT-BANDING sobre o registrador DATA do GPIO.
 */
#include "system.h"
#include "gpio.h"

const uint32_t tabela_const[4] = { 1, 2, 3, 4 };   /* .rodata -> FLASH */
uint32_t       var_inicializada = 0xCAFEBABE;      /* .data   -> RAM   */
uint32_t       var_zerada;                         /* .bss    -> RAM   */
extern const uint32_t vector_table[];

static uint32_t get_msp(void)
{
    uint32_t sp;
    __asm volatile ("mrs %0, msp" : "=r"(sp));
    return sp;
}

/* alias = 0x42000000 + (offset_do_byte * 32) + (bit * 4) */
#define BITBAND_PERIPH(reg_addr, bit) \
    (*(volatile uint32_t *)(0x42000000UL + (((uint32_t)(reg_addr) - 0x40000000UL) * 32U) + ((bit) * 4U)))

int main(void)
{
    uint32_t var_local = 42;

    system_init();
    uart_init(115200);

    kprintf("\n=== Modulo 01: arquitetura e memoria ===\n");

    uint32_t cpuid = SCB->CPUID;
    kprintf("CPUID  = 0x%08X (partno 0x%03X = %s, r%up%u)\n", cpuid, (cpuid >> 4) & 0xFFFU,
            ((cpuid >> 4) & 0xFFFU) == 0xC24 ? "Cortex-M4" : "?", (cpuid >> 20) & 0xFU, cpuid & 0xFU);

    /* DID0: CLASS [23:16] = 0x05 (TM4C123) / 0x0A (TM4C129), revisão MAJOR.MINOR
     * DID1: PARTNO [23:16] = 0xA1 (TM4C123GH6PM) / 0x1F (TM4C1294NCPDT) */
    uint32_t did0 = SYSCTL_DID0, did1 = SYSCTL_DID1;
    kprintf("DID0   = 0x%08X (classe 0x%02X, revisao %c%u)\n", did0, (did0 >> 16) & 0xFFU,
            (int)('A' + ((did0 >> 8) & 0xFFU)), did0 & 0xFFU);
    kprintf("DID1   = 0x%08X (part 0x%02X, %u pinos-code)\n", did1, (did1 >> 16) & 0xFFU, (did1 >> 13) & 7U);

    /* FLASHPP.SIZE: (n+1) * 2 KB ; SSIZE: (n+1) * 256 bytes */
    kprintf("Flash  = %u KB, SRAM = %u KB\n",
            ((FLASH_PP & 0xFFFFU) + 1U) * 2U, ((FLASH_SSIZE & 0xFFFFU) + 1U) / 4U);
    kprintf("Clock  = %u Hz\n", SystemCoreClock);

    kprintf("\nEndereco das coisas:\n");
    kprintf("  vector_table     %p  (inicio da FLASH = 0x0)\n", (void *)vector_table);
    kprintf("  main()           %p  (.text, bit0=1 -> Thumb)\n", (void *)main);
    kprintf("  tabela_const     %p  (.rodata)\n", (void *)tabela_const);
    kprintf("  var_inicializada %p  (.data) = 0x%08X\n", (void *)&var_inicializada, var_inicializada);
    kprintf("  var_zerada       %p  (.bss)  = %u\n", (void *)&var_zerada, var_zerada);
    kprintf("  var_local        %p  (pilha) = %u\n", (void *)&var_local, var_local);
    kprintf("  MSP atual        0x%08X, SP inicial 0x%08X\n", get_msp(), vector_table[0]);
    kprintf("  SYSCTL           0x%08X\n", SYSCTL_BASE);
    kprintf("  LED_PORT         %p\n", (void *)LED_PORT);

    gpio_port_enable(LED_PORT_NUM);
    gpio_output(LED_PORT, LED_PIN);

    volatile uint32_t *led_bb = &BITBAND_PERIPH(&LED_PORT->DATA, LED_PIN);
    kprintf("\nAlias bit-band do bit %u de DATA = %p\n", LED_PIN, (void *)led_bb);

    for (;;) {
        *led_bb ^= 1U;
        delay_ms(500);
    }
}
