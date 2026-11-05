/* =============================================================================
 *  linker.ld - STM32F072RB (128 KB Flash, 16 KB SRAM)
 *
 *  Explicado em detalhes em 03-linker-script/README.md
 * ========================================================================== */

/* Ponto de entrada: usado pelo GDB/OpenOCD e para o --gc-sections saber o
 * que é "raiz". O hardware NÃO usa isso: ele lê o vetor de reset em 0x00000004 */
ENTRY(Reset_Handler)

/* Tamanhos mínimos reservados (só para o linker reclamar se faltar RAM) */
_Min_Heap_Size  = 0x0;     /* o curso não usa malloc */
_Min_Stack_Size = 0x400;   /* 1 KB */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 128K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
}

/* A pilha do Cortex-M cresce para BAIXO: começa no fim da RAM */
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* 1) Tabela de vetores: TEM que estar no início da Flash (0x08000000),
     *    que é espelhada em 0x00000000 no boot pela Flash. */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))      /* KEEP: não deixar o --gc-sections remover */
        . = ALIGN(4);
    } > FLASH

    /* 2) Código */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7) *(.glue_7t)    /* veneers ARM<->Thumb (vazio no Cortex-M) */
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* 3) Constantes (const, strings literais) */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* Tabelas de unwind (C++/exceções). Mantidas por compatibilidade. */
    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    /* Construtores globais (C++ ou __attribute__((constructor)) em C) */
    .init_array :
    {
        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        __init_array_end = .;
    } > FLASH

    /* Endereço na FLASH onde fica a imagem inicial de .data (LMA) */
    _sidata = LOADADDR(.data);

    /* 4) Variáveis inicializadas: vivem na RAM (VMA) mas o valor inicial
     *    é gravado na FLASH (LMA). O startup copia FLASH -> RAM. */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.ramfunc*)              /* funções que devem rodar da RAM */
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH

    /* 5) Variáveis não inicializadas / zeradas: o startup zera essa região */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > RAM

    /* 6) Variáveis que NÃO devem ser zeradas no reset (sobrevivem a um
     *    reset por software/watchdog, mas não a um power-on) */
    .noinit (NOLOAD) :
    {
        . = ALIGN(4);
        *(.noinit*)
        . = ALIGN(4);
    } > RAM

    /* 7) Heap + pilha: só verifica se cabem. 'end' é usado pelo _sbrk da newlib */
    ._user_heap_stack (NOLOAD) :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    .ARM.attributes 0 : { *(.ARM.attributes) }
}
