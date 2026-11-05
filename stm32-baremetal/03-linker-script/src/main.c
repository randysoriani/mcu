/*
 * Módulo 03 - Linker script e seções
 *
 * Mostra, na prática, cada seção definida em common/<mcu>/linker.ld:
 *   .text, .rodata, .data, .bss, .noinit, .ramfunc e os símbolos do linker.
 *
 * Experimento: aperte o botão RESET (preto) várias vezes. O contador em
 * .noinit sobrevive ao reset; desligue o USB e ele volta a 1.
 */
#include "system.h"
#include "gpio.h"

/* ---- símbolos exportados pelo linker script ---- */
extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack, _etext;

/* ---- um objeto de cada tipo ---- */
const char      texto_flash[] = "eu moro na FLASH (.rodata)";   /* .rodata  */
uint32_t        contador_data = 100;                            /* .data    */
uint32_t        contador_bss;                                   /* .bss     */
static uint8_t  buffer_grande[1024];                            /* .bss     */

/* .noinit: o startup NÃO toca nessa região -> sobrevive a reset "quente" */
__attribute__((section(".noinit"))) uint32_t reset_magic;
__attribute__((section(".noinit"))) uint32_t reset_count;
#define MAGIC 0xDEADBEEFU

/*
 * Função executada da RAM. Vai para a seção .ramfunc, que o linker coloca
 * dentro de .data (VMA na RAM, LMA na Flash) -> o startup a copia junto.
 * long_call: a distância Flash(0x08..) -> RAM(0x20..) é maior que o alcance
 * de 16 MB da instrução BL, então o compilador precisa usar BLX via registrador.
 */
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

    /* Detecta power-on x reset quente */
    if (reset_magic != MAGIC) {
        reset_magic = MAGIC;
        reset_count = 0;
    }
    reset_count++;

    kprintf("\n=== Modulo 03: linker script ===\n");
    kprintf("Resets desde o power-on: %u\n\n", reset_count);

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
    kprintf("  main()         %p\n", (void *)main);

    /* Obs.: escrever em texto_flash[] via ponteiro NÃO altera a Flash; ela
     * só é gravada pela interface FLASH (módulo 22). Dependendo do chip a
     * escrita é ignorada, sinaliza erro em FLASH->SR ou gera BusFault. */

#if defined(STM32F0)
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
#else
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
#endif
    gpio_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT);

    /* Pisca o LED 'reset_count' vezes, pausa, repete */
    for (;;) {
        for (uint32_t i = 0; i < reset_count; i++) {
            gpio_set(LED_PORT, LED_PIN);   delay_ms(150);
            gpio_clear(LED_PORT, LED_PIN); delay_ms(250);
        }
        buffer_grande[contador_bss++ & 1023U] = 1;   /* usa o buffer (evita remoção) */
        delay_ms(1500);
    }
}
