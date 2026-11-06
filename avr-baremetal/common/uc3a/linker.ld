/* =============================================================================
 *  linker.ld - AT32UC3A0512 (512 KB Flash, 64 KB SRAM)
 *  Explicado em 03-linker-script/README.md
 *
 *  0x8000_0000  trampolim (.reset)        } os 8 KB do bootloader DFU da EVK1100
 *  0x8000_2000  programa (.text...)       } o bootloader salta para cá
 *  0x0000_0004  SRAM (.data, .bss, pilha) — o endereço 0 fica de fora para que
 *               um ponteiro nulo nunca aponte para uma variável válida
 * ========================================================================== */
OUTPUT_FORMAT("elf32-avr32", "elf32-avr32", "elf32-avr32")
OUTPUT_ARCH(avr32:uc)
ENTRY(_trampoline)

MEMORY
{
    FLASH    (rxai!w) : ORIGIN = 0x80000000, LENGTH = 512K
    INTRAM   (wxa!ri) : ORIGIN = 0x00000004, LENGTH = 0x0000FFFC
    USERPAGE (rxai!w) : ORIGIN = 0x80800000, LENGTH = 512
}

_estack = 0x00010000;          /* fim da SRAM de 64 KB */
_stack_size = 0x1000;          /* 4 KB reservados (verificação abaixo) */

SECTIONS
{
    .reset 0x80000000 : { KEEP(*(.reset)) } > FLASH

    .text 0x80002000 :
    {
        *(.text._start)
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4);
    } > FLASH

    .exception :
    {
        . = ALIGN(0x200);
        KEEP(*(.exception))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        _edata = .;
    } > INTRAM AT> FLASH
    _sidata = LOADADDR(.data);

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > INTRAM

    .noinit (NOLOAD) : { . = ALIGN(4); *(.noinit*) } > INTRAM

    ASSERT(. + _stack_size <= _estack, "SRAM insuficiente para a pilha")

    .userpage : { *(.userpage*) } > USERPAGE
}
