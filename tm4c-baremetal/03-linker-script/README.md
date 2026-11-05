# Módulo 03 — Linker script e organização do programa na memória

Arquivos: [`../common/123/linker.ld`](../common/123/linker.ld) (256 KB/32 KB), [`../common/129/linker.ld`](../common/129/linker.ld) (1 MB/256 KB), [`src/main.c`](src/main.c).

```sh
make MCU=123 flash && picocom -b 115200 /dev/ttyACM0
# aperte RESET algumas vezes: o contador em .noinit sobrevive
```

---

## 1. Seções

| Seção | Conteúdo | Destino |
|---|---|---|
| `.isr_vector` | tabela de vetores | **início da Flash (0x0)** |
| `.text` | código | Flash |
| `.rodata` | `const`, strings | Flash |
| `.data` | globais inicializadas | RAM (VMA) + valor inicial na Flash (LMA) |
| `.bss` | globais zeradas | RAM (zerada pelo startup) |
| `.noinit` | não tocada pelo startup | RAM |
| `.ramfunc` | funções executadas da RAM | dentro de `.data` |

## 2. O script

```ld
ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 256K     /* 129: 1024K */
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 32K      /* 129: 256K  */
}

_estack = ORIGIN(RAM) + LENGTH(RAM);     /* pilha cresce para baixo a partir do fim da RAM */

SECTIONS
{
    .isr_vector : { KEEP(*(.isr_vector)) } > FLASH      /* KEEP: não descartar */
    .text       : { *(.text*) ... _etext = .; } > FLASH
    .rodata     : { *(.rodata*) } > FLASH
    _sidata = LOADADDR(.data);
    .data : { _sdata = .; *(.data*) *(.ramfunc*) _edata = .; } > RAM AT> FLASH
    .bss (NOLOAD) : { _sbss = .; *(.bss*) *(COMMON) _ebss = .; } > RAM
    .noinit (NOLOAD) : { *(.noinit*) } > RAM
    ._user_heap_stack (NOLOAD) : { ... } > RAM           /* só verifica se sobra RAM */
}
```

Única diferença conceitual em relação ao STM32: `ORIGIN = 0x00000000` para a Flash.

### VMA × LMA
- **VMA**: endereço em execução (`.data` na RAM).
- **LMA**: onde é gravado (`.data` na Flash, logo após o `.rodata`).
- `> RAM AT> FLASH` define as duas; `LOADADDR(.data)` = `_sidata`.

```sh
arm-none-eabi-objdump -h build/123/03-linker-script.elf     # compare VMA e LMA de .data
```

### `.noinit`
Fora de `_sbss.._ebss`: sobrevive a reset por botão/watchdog/software (a RAM continua energizada), lixo após power-on. Use com um número mágico. O exemplo também imprime `SYSCTL_RESC` (causa do reset, módulo 19).

### Função na RAM
```c
__attribute__((section(".ramfunc"), noinline, long_call))
uint32_t soma_na_ram(uint32_t a, uint32_t b);
```
Flash em `0x0000_0000` e RAM em `0x2000_0000` estão a 512 MB: fora do alcance de ±16 MB do `BL`, daí o `long_call`. Útil para rodar código enquanto a Flash é apagada/gravada (módulo 22).

## 3. Mapa da RAM

```
0x20008000 (123) / 0x20040000 (129)  <- _estack
           pilha ↓
           (livre)
           ├ end
           .noinit
           .bss
           .data
0x20000000
```

## 4. Map file e ferramentas
```sh
less build/123/03-linker-script.map
arm-none-eabi-nm -n -S --size-sort build/123/03-linker-script.elf
arm-none-eabi-size -A build/123/03-linker-script.elf
```

## 5. Customizações
- Reservar o último bloco/setor da Flash para dados (módulo 22): diminua `LENGTH` da FLASH em 1 KB (123) ou 16 KB (129) e crie uma região `DATA`.
- Aplicação após bootloader: `ORIGIN = 0x00002800` + `SCB->VTOR = 0x2800`.
- O 129 tem 256 KB de SRAM: dá para colocar buffers grandes (Ethernet, áudio) em seções próprias alinhadas.

---

## Exercícios
1. Mude `_Min_Stack_Size` do 123 para `0x8000` e leia o erro do linker.
2. Descubra no `.map` o tamanho de `kprintf`.
3. Crie uma seção `.fw_info` em endereço fixo (`0x00000400`) com a versão do firmware.
