# Módulo 03 — Linker script e organização da memória do programa

O compilador gera código **relocável** (sem endereços definitivos). Quem decide que `main` fica em `0x080001d4` e que `contador` fica em `0x20000000` é o **linker**, seguindo o **linker script**.

Arquivos:
- [`../common/f0/linker.ld`](../common/f0/linker.ld) — STM32F072RB (128 KB Flash / 16 KB RAM)
- [`../common/f4/linker.ld`](../common/f4/linker.ld) — STM32F446RE (512 KB / 128 KB)
- [`src/main.c`](src/main.c) — imprime onde cada seção foi parar, testa `.noinit` e uma função executada da RAM

```sh
make MCU=f4 flash && picocom -b 115200 /dev/ttyACM0
# aperte o botão RESET (preto) algumas vezes e veja o contador .noinit
```

Saída real (F4, executado no QEMU):
```
Resets desde o power-on: 1

Simbolos do linker:
  _etext  (fim do codigo)        = 0x080008f4
  _sidata (LMA de .data, FLASH)  = 0x08000bf8
  _sdata  (VMA de .data, RAM)    = 0x20000000
  _edata                         = 0x20000014  (.data = 20 bytes)
  _sbss                          = 0x20000014
  _ebss                          = 0x2000041c  (.bss = 1032 bytes)
  _estack (topo da pilha)        = 0x20020000
  MSP agora                      = 0x2001FFD0
Objetos:
  texto_flash    0x08000bd4  "eu moro na FLASH (.rodata)"
  contador_data  0x20000000  = 100
  contador_bss   0x20000414  = 0
  buffer_grande  0x20000014  (1024 bytes)
  reset_count    0x2000041c  (.noinit)
  soma_na_ram()  0x20000011  -> 2+3 = 5
```

---

## 1. Seções geradas pelo compilador

| Seção | O que contém | Vai para |
|---|---|---|
| `.text` | código | Flash |
| `.rodata` | `const`, strings literais, tabelas de `switch` | Flash |
| `.data` | globais/estáticas **com** inicializador ≠ 0 | RAM (valor inicial na Flash) |
| `.bss` | globais/estáticas sem inicializador ou = 0 | RAM (zerada pelo startup) |
| `.isr_vector` | tabela de vetores (nome escolhido por nós) | início da Flash |
| `.init_array` | ponteiros para construtores | Flash |
| `.ARM.exidx`/`.ARM.extab` | tabelas de *unwind* (C++/exceções) | Flash |
| `COMMON` | globais "tentativas" (código C antigo; evitado com `-fno-common`) | RAM |

Com `-ffunction-sections -fdata-sections`, cada função vira `.text.<nome>` e cada variável `.data.<nome>`/`.bss.<nome>`. Veja com:
```sh
arm-none-eabi-objdump -h build/f4/obj/main.o
```

---

## 2. Anatomia do linker script

### `ENTRY`
```ld
ENTRY(Reset_Handler)
```
Informa o ponto de entrada no cabeçalho ELF (usado pelo GDB/OpenOCD) e serve de raiz para o `--gc-sections`. **O hardware ignora**: ele usa o vetor de reset.

### `MEMORY`
```ld
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 128K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
}
```
Declara as regiões físicas. Os atributos (`r`, `w`, `x`) só servem para o linker escolher regiões para seções não atribuídas. Se o programa não couber, o link falha com `region 'FLASH' overflowed by N bytes` — melhor do que descobrir na placa.

### Símbolos
```ld
_estack = ORIGIN(RAM) + LENGTH(RAM);
```
Cria um símbolo cujo **endereço** é o fim da RAM. O C o enxerga com `extern uint32_t _estack;` e usa `&_estack`. A pilha do Cortex-M é *full descending*: o SP aponta para o último item empilhado e decrementa antes de cada push, por isso começar exatamente no fim é correto (o primeiro push escreve em `_estack - 4`).

O contador de localização `.` representa "o endereço atual" dentro de uma seção.

### `SECTIONS`
```ld
.isr_vector :
{
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
} > FLASH
```
- `*(.isr_vector)` — "pegue a seção `.isr_vector` de **todos** os arquivos de entrada".
- `KEEP()` — não descarte, mesmo que ninguém a referencie.
- `> FLASH` — coloque na região FLASH. Como é a primeira seção, cai em `0x0800_0000`.

```ld
.text :
{
    *(.text)
    *(.text*)          /* .text.main, .text.uart_init, ... */
    KEEP(*(.init))
    KEEP(*(.fini))
    _etext = .;
} > FLASH
```

### VMA × LMA: o truque do `.data`
```ld
_sidata = LOADADDR(.data);

.data :
{
    _sdata = .;
    *(.data)
    *(.data*)
    *(.ramfunc*)
    _edata = .;
} > RAM AT> FLASH
```

- **VMA** (Virtual Memory Address): onde a seção estará quando o programa **rodar** → RAM.
- **LMA** (Load Memory Address): onde ela é **gravada** → Flash, logo após o `.rodata`.
- `> RAM AT> FLASH` define as duas. `LOADADDR(.data)` devolve a LMA.

```sh
$ arm-none-eabi-objdump -h build/f4/03-linker-script.elf
Idx Name              Size      VMA       LMA
  0 .isr_vector       000001c4  08000000  08000000
  1 .text             00000730  080001c4  080001c4
  2 .rodata           000002fc  080008f4  080008f4
  3 .ARM              00000008  08000bf0  08000bf0
  4 .init_array       00000000  08000bf8  08000bf8
  5 .data             00000014  20000000  08000bf8   <- VMA (RAM) ≠ LMA (Flash)
  6 .bss              00000408  20000014  08000c0c
  7 .noinit           00000008  2000041c  08000c0c
  8 ._user_heap_stack 00000804  20000424  08000c0c
```

O código usa os endereços de RAM (VMA); o gravador escreve os bytes na Flash (LMA); o startup copia LMA → VMA. É exatamente o laço do módulo 02.

### `.bss` e `NOLOAD`
```ld
.bss (NOLOAD) :
{
    _sbss = .;
    *(.bss) *(.bss*) *(COMMON)
    _ebss = .;
} > RAM
```
`NOLOAD`: a seção ocupa endereço mas não tem conteúdo no arquivo — não vai para o `.bin`.

### `.noinit`
```ld
.noinit (NOLOAD) : { *(.noinit*) } > RAM
```
Fica **fora** de `_sbss.._ebss`, então o startup não a zera. Uma variável ali mantém o valor após um reset por botão, watchdog ou software (a RAM continua energizada), mas tem lixo após um power-on. Uso típico: guardar a causa de um crash para ler no próximo boot. No C:
```c
__attribute__((section(".noinit"))) uint32_t reset_count;
```
Sempre combine com um "número mágico" para distinguir lixo de valor válido (ver `main.c`).

### Heap e pilha
```ld
._user_heap_stack (NOLOAD) :
{
    . = ALIGN(8);
    PROVIDE(end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
} > RAM
```
Não reserva nada de verdade: só faz o linker **falhar** se, depois de `.data` + `.bss`, não sobrarem `_Min_Heap_Size + _Min_Stack_Size` bytes. A pilha real cresce de `_estack` para baixo; `end` marca onde o heap (do `malloc` da newlib, via `_sbrk`) começaria. O curso não usa `malloc`.

```
RAM (F072, 16 KB)
0x20004000 ┌──────────────┐ <- _estack (SP inicial)
           │   pilha  ↓   │
           │              │
           │   (livre)    │
           │   heap   ↑   │
           ├──────────────┤ <- end
           │   .noinit    │
           ├──────────────┤ <- _ebss
           │   .bss       │
           ├──────────────┤ <- _sbss = _edata
           │   .data      │
0x20000000 └──────────────┘ <- _sdata
```

> Não há proteção contra *stack overflow* no M0: a pilha simplesmente invade o `.bss`. No M4 pode-se usar a MPU para criar uma região de guarda.

---

## 3. Executando uma função da RAM

Por que? Código na RAM roda sem wait states e continua executando enquanto a Flash está ocupada (apagando/gravando, módulo 22).

```c
__attribute__((section(".ramfunc"), noinline, long_call))
uint32_t soma_na_ram(uint32_t a, uint32_t b) { return a + b; }
```

- `section(".ramfunc")` → o linker script inclui `*(.ramfunc*)` dentro de `.data`: a função ganha VMA na RAM e LMA na Flash, e o startup a copia junto com as variáveis.
- `long_call` → a instrução `BL` só alcança ±16 MB. A distância entre a Flash (`0x0800_0000`) e a RAM (`0x2000_0000`) é ~384 MB, então o compilador precisa carregar o endereço num registrador e usar `BLX`. (O linker do GNU também sabe gerar *veneers* automaticamente, mas ser explícito é melhor.)
- O endereço impresso (`0x20000011`) tem bit 0 = 1: é Thumb.
- A função não pode chamar código na Flash se o objetivo for rodar durante uma gravação.

---

## 4. O map file

`build/<mcu>/<modulo>.map` mostra, para cada seção de saída, de qual arquivo/seção de entrada veio cada byte, além das seções descartadas pelo `--gc-sections`.

```sh
grep -A3 "^ \.text\.main" build/f4/03-linker-script.map
grep -A20 "Discarded input sections" build/f4/03-linker-script.map | head
```

Outras ferramentas:
```sh
arm-none-eabi-nm -n -S --size-sort build/f4/03-linker-script.elf   # maiores símbolos
arm-none-eabi-size -A build/f4/03-linker-script.elf
```

---

## 5. Customizações comuns

### Reservar uma área da Flash para dados (módulo 22)
```ld
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 126K
    DATA  (r)   : ORIGIN = 0x0801F800, LENGTH = 2K     /* última página do F072 */
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
}
```

### Aplicação após um bootloader
```ld
FLASH (rx) : ORIGIN = 0x08008000, LENGTH = 480K       /* bootloader ocupa 32 KB */
```
+ `SCB->VTOR = 0x08008000;` no início (F4).

### Estrutura em endereço fixo (ex.: versão do firmware)
```ld
.fw_info 0x08000200 : { KEEP(*(.fw_info)) } > FLASH
```
```c
__attribute__((section(".fw_info"), used))
const struct { uint32_t magic, version; } fw_info = { 0xF1A5C0DE, 0x00010002 };
```

### CCM RAM (F407/F429: 64 KB em `0x1000_0000`, só acessível pela CPU, não pelo DMA)
```ld
CCMRAM (rw) : ORIGIN = 0x10000000, LENGTH = 64K
...
.ccmram (NOLOAD) : { *(.ccmram*) } > CCMRAM
```

---

## Exercícios

1. Crie uma variável `const uint8_t tabela[4096]` e veja o impacto no `size`. Tire o `const` e veja de novo (dica: olhe `data`, e a RAM no `--print-memory-usage`).
2. Abra o `.map` e descubra quantos bytes a função `kprintf` ocupa.
3. Mude `_Min_Stack_Size` do F0 para `0x3C00` e compile o módulo 03. Leia a mensagem de erro.
4. Crie uma seção `.fw_info` em endereço fixo e leia-a com `st-flash read` ou no GDB com `x/2xw 0x08000200`.
5. Faça o `reset_count` ser mostrado também após um reset pelo watchdog (combine com o módulo 19).
