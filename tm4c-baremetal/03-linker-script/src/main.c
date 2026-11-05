/*
 * Módulo 03 - Linker script e seções (TM4C123 / TM4C129)
 * Aperte o RESET algumas vezes: o contador em .noinit sobrevive ao reset;
 * desligue o USB e ele volta a 1.
 */
#include "system.h"
#include "gpio.h"

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack, _etext;

const char      texto_flash[] = "eu moro na FLASH (.rodata)";
uint32_t        contador_data = 100;
uint32_t        contador_bss;
static uint8_t  buffer_grande[1024];

__attribute__((section(".noinit"))) uint32_t reset_magic;
__attribute__((section(".noinit"))) uint32_t reset_count;
#define MAGIC 0xDEADBEEFU

/* Executada da RAM: Flash (0x0000_0000) -> RAM (0x2000_0000) = 512 MB, longe
 * demais para o BL (+-16 MB) -> long_call */
__attribute__((section(".ramfunc"), noinline, long_call))
uint32_t soma_na_ram(uint32_t a, uint32_t b)
{
    return a + b;
}

static uint32_t get_msp(void)
{
    uint32_t sp;
    __asm volatile ("mrs %0, msp" : "=r"(sp));
    return sp;
}

int main(void)
{
    system_init();
    uart_init(115200);

    if (reset_magic != MAGIC) { reset_magic = MAGIC; reset_count = 0; }
    reset_count++;

    kprintf("\n=== Modulo 03: linker script ===\n");
    kprintf("Resets desde o power-on: %u (RESC = 0x%08X)\n\n", reset_count, SYSCTL_RESC);
    SYSCTL_RESC = 0;

    kprintf("Simbolos do linker:\n");
    kprintf("  _etext  (fim do codigo)        = %p\n", (void *)&_etext);
    kprintf("  _sidata (LMA de .data, FLASH)  = %p\n", (void *)&_sidata);
    kprintf("  _sdata  (VMA de .data, RAM)    = %p\n", (void *)&_sdata);
    kprintf("  _edata                         = %p  (.data = %u bytes)\n",
            (void *)&_edata, (uint32_t)((uint8_t *)&_edata - (uint8_t *)&_sdata));
    kprintf("  _sbss                          = %p\n", (void *)&_sbss);
    kprintf("  _ebss                          = %p  (.bss = %u bytes)\n",
            (void *)&_ebss, (uint32_t)((uint8_t *)&_ebss - (uint8_t *)&_sbss));
    kprintf("  _estack (topo da pilha)        = %p\n", (void *)&_estack);
    kprintf("  MSP agora                      = 0x%08X\n\n", get_msp());

    kprintf("Objetos:\n");
    kprintf("  texto_flash    %p  \"%s\"\n", (void *)texto_flash, texto_flash);
    kprintf("  contador_data  %p  = %u\n", (void *)&contador_data, contador_data);
    kprintf("  contador_bss   %p  = %u\n", (void *)&contador_bss, contador_bss);
    kprintf("  buffer_grande  %p  (%u bytes)\n", (void *)buffer_grande, (uint32_t)sizeof buffer_grande);
    kprintf("  reset_count    %p  (.noinit)\n", (void *)&reset_count);
    kprintf("  soma_na_ram()  %p  -> 2+3 = %u\n", (void *)soma_na_ram, soma_na_ram(2, 3));

    gpio_port_enable(LED_PORT_NUM);
    gpio_output(LED_PORT, LED_PIN);
    for (;;) {
        for (uint32_t i = 0; i < reset_count; i++) {
            gpio_set(LED_PORT, LED_PIN);   delay_ms(150);
            gpio_clear(LED_PORT, LED_PIN); delay_ms(250);
        }
        buffer_grande[contador_bss++ & 1023U] = 1;
        delay_ms(1500);
    }
}
