/*
 * Módulo 01 - Arquitetura e organização da memória
 *
 * Imprime na serial (115200 8N1, /dev/ttyACM0) informações do núcleo e do
 * chip e mostra EM QUE ENDEREÇO cada tipo de objeto C vai parar.
 * (clock, uart e kprintf são explicados nos módulos 04, 06 e 07; aqui são
 *  usados como "caixa preta".)
 *
 * O mesmo arquivo compila para F0 e F4 (diferenças isoladas em #if).
 */
#include "system.h"
#include "gpio.h"

const uint32_t tabela_const[4] = { 1, 2, 3, 4 };   /* .rodata -> FLASH       */
uint32_t       var_inicializada = 0xCAFEBABE;      /* .data   -> RAM (cópia) */
uint32_t       var_zerada;                         /* .bss    -> RAM (zerada)*/
extern const uint32_t vector_table[];               /* definida no startup.c  */

static uint32_t get_msp(void)
{
    uint32_t sp;
    __asm volatile ("mrs %0, msp" : "=r"(sp));
    return sp;
}

#if defined(STM32F4)
/*
 * Bit-banding (só Cortex-M3/M4): cada BIT da região 0x40000000-0x400FFFFF
 * tem uma WORD própria em 0x42000000-0x43FFFFFF. Escrever 0/1 nessa word
 * altera apenas aquele bit, de forma atômica (sem read-modify-write).
 *   alias = 0x42000000 + (offset_do_byte * 32) + (bit * 4)
 */
#define BITBAND_PERIPH(reg_addr, bit) \
    (*(volatile uint32_t *)(PERIPH_BB_BASE + (((uint32_t)(reg_addr) - PERIPH_BASE) * 32U) + ((bit) * 4U)))
#endif

int main(void)
{
    uint32_t var_local = 42;                       /* pilha -> RAM (topo) */

    system_init();
    uart_init(115200);

    kprintf("\n=== Modulo 01: arquitetura e memoria ===\n");

    /* --- CPUID (SCB, 0xE000ED00): identifica o núcleo ARM --- */
    uint32_t cpuid = SCB->CPUID;
    uint32_t partno = (cpuid >> 4) & 0xFFFU;
    kprintf("CPUID       = 0x%08X  (implementer 0x%02X, partno 0x%03X = %s, r%up%u)\n",
            cpuid, cpuid >> 24, partno,
            partno == 0xC20 ? "Cortex-M0" : partno == 0xC60 ? "Cortex-M0+" :
            partno == 0xC24 ? "Cortex-M4" : partno == 0xC27 ? "Cortex-M7" : "?",
            (cpuid >> 20) & 0xFU, cpuid & 0xFU);

    /* --- IDCODE (DBGMCU): identifica o CHIP da ST ---
     * F072 -> DEV_ID 0x448 ; F446 -> DEV_ID 0x421
     * Obs.: em alguns F4 (errata) o IDCODE lê 0 sem debugger conectado. */
#if defined(STM32F0)
    RCC->APB2ENR |= RCC_APB2ENR_DBGMCUEN;
#endif
    uint32_t idcode = DBGMCU->IDCODE;
    kprintf("IDCODE      = 0x%08X  (DEV_ID 0x%03X, REV_ID 0x%04X)\n",
            idcode, idcode & 0xFFFU, idcode >> 16);

    /* --- Dados gravados na fábrica (System Memory) --- */
    kprintf("Flash       = %u KB\n", *(const uint16_t *)FLASHSIZE_BASE);
    const uint32_t *uid = (const uint32_t *)UID_BASE;
    kprintf("Unique ID   = %08X-%08X-%08X\n", uid[2], uid[1], uid[0]);

    /* --- Onde cada coisa mora --- */
    kprintf("\nEndereco das coisas:\n");
    kprintf("  vector_table     %p  (inicio da FLASH)\n", (void *)vector_table);
    kprintf("  main()           %p  (.text, bit0=1 -> Thumb)\n", (void *)main);
    kprintf("  tabela_const     %p  (.rodata, FLASH)\n", (void *)tabela_const);
    kprintf("  var_inicializada %p  (.data, RAM) = 0x%08X\n", (void *)&var_inicializada, var_inicializada);
    kprintf("  var_zerada       %p  (.bss,  RAM) = %u\n", (void *)&var_zerada, var_zerada);
    kprintf("  var_local        %p  (pilha)      = %u\n", (void *)&var_local, var_local);
    kprintf("  MSP atual        0x%08X\n", get_msp());
    kprintf("  SP inicial       0x%08X  (vector_table[0])\n", vector_table[0]);
    kprintf("  RCC              %p  (periferico)\n", (void *)RCC);
    kprintf("  GPIOA            %p  (periferico)\n", (void *)GPIOA);

    /* --- LED --- */
#if defined(STM32F0)
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
#else
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
#endif
    gpio_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT);

#if defined(STM32F4)
    volatile uint32_t *led_bb = &BITBAND_PERIPH(&GPIOA->ODR, LED_PIN);
    kprintf("\nAlias bit-band de GPIOA->ODR bit5 = %p\n", (void *)led_bb);
#endif

    for (;;) {
#if defined(STM32F4)
        *led_bb ^= 1U;                 /* alterna só o bit 5 do ODR */
#else
        gpio_toggle(LED_PORT, LED_PIN);
#endif
        delay_ms(500);
    }
}
