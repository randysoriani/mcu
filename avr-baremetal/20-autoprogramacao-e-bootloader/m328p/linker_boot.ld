/* =============================================================================
 *  linker_boot.ld - igual a common/m328p/linker.ld + seção .bootloader em 0x7000
 *  (seção de boot de 2 KB: fuse alto com BOOTSZ1:0 = 00). Módulo 20.
 *  Explicado em 03-linker-script/README.md
 *
 *  O AVR tem memórias SEPARADAS (arquitetura Harvard), cada uma com seus
 *  próprios endereços começando em 0. Para o linker (que só conhece um espaço
 *  de endereços), o GCC usa deslocamentos convencionais:
 *     Flash  -> 0x000000
 *     SRAM   -> 0x800000  (0x800100 = endereço 0x0100 da RAM)
 *     EEPROM -> 0x810000
 * ========================================================================== */
OUTPUT_FORMAT("elf32-avr", "elf32-avr", "elf32-avr")
OUTPUT_ARCH(avr:5)
ENTRY(__vectors)

MEMORY
{
    text   (rx)   : ORIGIN = 0x000000, LENGTH = 32K
    data   (rw!x) : ORIGIN = 0x800100, LENGTH = 2K
    eeprom (rw!x) : ORIGIN = 0x810000, LENGTH = 1K
}

/* SP inicial = último byte da RAM (0x08FF) */
__stack = 0x8008FF;

SECTIONS
{
    .text :
    {
        KEEP(*(.vectors))          /* tabela de saltos no endereço 0 */
        *(.progmem*)               /* constantes guardadas só na Flash (PROGMEM) */
        . = ALIGN(2);
        KEEP(*(.init))             /* código de inicialização (startup.S) */
        *(.text)
        *(.text.*)
        . = ALIGN(2);
        _etext = .;
    } > text

    /* Código que precisa rodar da seção de boot (só lá a instrução SPM funciona) */
    .bootloader 0x7000 :
    {
        KEEP(*(.bootloader))
    } > text

    /* .data: usada na RAM, gravada na Flash logo após o código.
     * ATENÇÃO: no AVR as constantes comuns (.rodata) também vão para a RAM,
     * porque um ponteiro C comum só enxerga o espaço de dados. */
    .data :
    {
        PROVIDE(__data_start = .);
        *(.data)
        *(.data.*)
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(2);
        PROVIDE(__data_end = .);
    } > data AT> text

    __data_load_start = LOADADDR(.data);

    .bss (NOLOAD) :
    {
        PROVIDE(__bss_start = .);
        *(.bss)
        *(.bss.*)
        *(COMMON)
        PROVIDE(__bss_end = .);
    } > data

    .noinit (NOLOAD) :
    {
        *(.noinit*)
    } > data

    __heap_start = .;              /* RAM livre entre aqui e a pilha */

    .eeprom :
    {
        KEEP(*(.eeprom*))          /* variáveis com valor inicial na EEPROM */
    } > eeprom
}
