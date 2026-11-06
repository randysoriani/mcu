/*
 * Módulo 03 - Linker script: seções, PROGMEM, .noinit
 * Aperte o RESET várias vezes: o contador em .noinit sobrevive.
 */
#include "system.h"

#if defined(AVR8)
extern char __data_start[], __data_end[], __bss_start[], __bss_end[], __heap_start[], __data_load_start[];
/* No AVR8, "const" sozinho NÃO deixa o dado só na Flash: ele é copiado para
 * a RAM pelo startup. Para ficar só na Flash, use a seção .progmem e leia
 * com a instrução LPM. */
#define PROGMEM __attribute__((section(".progmem.data")))
static inline uint8_t flash_read_byte(const void *addr)
{
    uint8_t r;
    __asm__ __volatile__ ("lpm %0, Z" : "=r"(r) : "z"(addr));
    return r;
}
#else
extern char _sdata[], _edata[], _sbss[], _ebss[], _sidata[], _estack[];
#endif

const char texto_ram[]   = "const comum";
#if defined(AVR8)
const char texto_flash[] PROGMEM = "so na Flash (PROGMEM)";
#endif
uint16_t contador_data = 100;
uint16_t contador_bss;
static uint8_t buffer_grande[256];

__attribute__((section(".noinit"))) uint16_t reset_magic;
__attribute__((section(".noinit"))) uint16_t reset_count;

int main(void)
{
    system_init();
    uart_init(115200);
    led_init();

    if (reset_magic != 0xBEEF) { reset_magic = 0xBEEF; reset_count = 0; }
    reset_count++;

    kprintf("\n=== Modulo 03: linker script ===\n");
    kprintf("Resets desde que a placa foi ligada: %u\n\n", reset_count);

#if defined(AVR8)
    kprintf("Simbolos do linker (enderecos de RAM somam 0x800000 no ELF):\n");
    kprintf("  __data_load_start (Flash) = 0x%04x\n", (unsigned)(uintptr_t)__data_load_start);
    kprintf("  __data_start..__data_end  = 0x%04x..0x%04x (%u bytes)\n",
            (unsigned)(uintptr_t)__data_start, (unsigned)(uintptr_t)__data_end,
            (unsigned)(__data_end - __data_start));
    kprintf("  __bss_start..__bss_end    = 0x%04x..0x%04x (%u bytes)\n",
            (unsigned)(uintptr_t)__bss_start, (unsigned)(uintptr_t)__bss_end,
            (unsigned)(__bss_end - __bss_start));
    kprintf("  RAM livre ate a pilha     ~ %u bytes\n",
            (unsigned)(((SPH << 8) | SPL) - (uintptr_t)__heap_start));
    kprintf("\ntexto_ram   em 0x%04x (RAM): \"%s\"\n", (unsigned)(uintptr_t)texto_ram, texto_ram);
    kprintf("texto_flash em 0x%04x (Flash): \"", (unsigned)(uintptr_t)texto_flash);
    for (const char *p = texto_flash; flash_read_byte(p); p++) uart_putc((char)flash_read_byte(p));
    kprintf("\"\n");
#else
    kprintf("  _sidata (Flash) = 0x%08lx\n", (unsigned long)(uintptr_t)_sidata);
    kprintf("  .data           = 0x%08lx..0x%08lx\n", (unsigned long)(uintptr_t)_sdata, (unsigned long)(uintptr_t)_edata);
    kprintf("  .bss            = 0x%08lx..0x%08lx\n", (unsigned long)(uintptr_t)_sbss, (unsigned long)(uintptr_t)_ebss);
    kprintf("  _estack         = 0x%08lx\n", (unsigned long)(uintptr_t)_estack);
    kprintf("texto_ram em 0x%08lx: no AVR32 const fica na Flash (mapa unico)\n", (unsigned long)(uintptr_t)texto_ram);
#endif
    kprintf("contador_data = %u, contador_bss = %u, buffer em 0x%lx\n",
            contador_data, contador_bss, (unsigned long)(uintptr_t)buffer_grande);

    for (;;) {
        for (uint16_t i = 0; i < reset_count; i++) {
            led_on();  delay_ms(150);
            led_off(); delay_ms(250);
        }
        buffer_grande[contador_bss++ & 0xFF] = 1;
        delay_ms(1500);
    }
}
