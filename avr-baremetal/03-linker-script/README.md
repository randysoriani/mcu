# Módulo 03 — Linker script e organização do programa na memória

> **Objetivo deste módulo:** entender como o código C vira bytes em endereços exatos da Flash, da RAM (e da EEPROM, no ATmega), quem decide esses endereços, e a maior armadilha de memória do AVR de 8 bits: **constantes que ocupam RAM**.

Arquivos: [`common/m328p/linker.ld`](../common/m328p/linker.ld), [`common/uc3a/linker.ld`](../common/uc3a/linker.ld), [`src/main.c`](src/main.c).

```sh
make MCU=m328p flash && picocom -b 115200 /dev/ttyACM0     # aperte RESET algumas vezes
```

Saída real (ATmega328P no QEMU):
```
=== Modulo 03: linker script ===
Resets desde que a placa foi ligada: 1

Simbolos do linker (enderecos de RAM somam 0x800000 no ELF):
  __data_load_start (Flash) = 0x08a2
  __data_start..__data_end  = 0x0100..0x02fc (508 bytes)
  __bss_start..__bss_end    = 0x02fc..0x0402 (262 bytes)
  RAM livre ate a pilha     ~ 1243 bytes

texto_ram   em 0x02ce (RAM): "const comum"
texto_flash em 0x0068 (Flash): "so na Flash (PROGMEM)"
contador_data = 100, contador_bss = 0, buffer em 0x2fc
```
Repare: o `.data` tem 508 bytes, quase todos **textos dos `kprintf`**, que ocupam RAM — um quarto dos 2 KB do chip.

---

## 1. Compilar e linkar

```
 main.c ─[compilador]─► main.o ─┐
 uart.c ─[compilador]─► uart.o ─┼─[linker + linker.ld]─► programa.elf ─[objcopy]─► .hex (Flash) + .eep (EEPROM)
startup.S [montador]─► startup.o┘
```
1. O **compilador** traduz cada `.c` separadamente para código de máquina (`.o`), sem saber os endereços finais.
2. O **linker** junta tudo, **decide onde cada função e variável vai morar** e preenche as referências entre arquivos, seguindo o **linker script**.
3. O **objcopy** extrai do ELF o que vai para cada memória: `.hex` para a Flash e `.eep` para a EEPROM.

Analogia: os `.o` são as matérias de um jornal; o linker é o diagramador; o linker script é o modelo de diagramação.

## 2. Seções

| Seção | O que contém | ATmega328P | UC3A |
|---|---|---|---|
| `.vectors` / `.reset` | tabela de vetores / trampolim (módulo 02) | Flash, endereço 0 | Flash, `0x8000_0000` |
| `.text` | código | Flash | Flash (a partir de `0x8000_2000`) |
| `.rodata` | constantes (`const`, textos) | **RAM** (cópia na Flash) | Flash |
| `.progmem` | constantes marcadas `PROGMEM` | Flash | — |
| `.data` | globais com valor inicial | RAM (valor inicial na Flash) | RAM (idem) |
| `.bss` | globais zeradas | RAM | RAM |
| `.noinit` | não tocada pelo startup | RAM | RAM |
| `.eeprom` | variáveis com valor inicial na EEPROM | EEPROM | — |
| `.exception` | tabela de exceções | — | Flash, alinhada a 512 bytes |

---

## 3. O linker script do ATmega328P

### 3.1 Três memórias, um linker
O linker do GNU só conhece **um** espaço de endereços, mas o AVR tem três memórias com endereços próprios (módulo 01). A solução do avr-gcc é uma convenção: somar um deslocamento a cada memória.

```ld
MEMORY
{
    text   (rx)   : ORIGIN = 0x000000, LENGTH = 32K     /* Flash */
    data   (rw!x) : ORIGIN = 0x800100, LENGTH = 2K      /* RAM: 0x800000 + 0x0100 */
    eeprom (rw!x) : ORIGIN = 0x810000, LENGTH = 1K      /* EEPROM: 0x810000 + 0 */
}
```
No ELF uma variável aparece em `0x800100`, mas no programa o endereço é `0x0100`: o compilador usa só os 16 bits de baixo. A RAM começa em `0x0100` porque abaixo disso ficam os registradores (módulo 01).

### 3.2 A pilha
```ld
__stack = 0x8008FF;          /* último byte da RAM */
```
O startup coloca esse valor no SP (módulo 02). A pilha cresce para baixo, em direção às variáveis.

### 3.3 Código e tabela de vetores
```ld
.text :
{
    KEEP(*(.vectors))        /* tabela de saltos no endereço 0 — KEEP: nunca descartar */
    *(.progmem*)             /* constantes PROGMEM */
    KEEP(*(.init))           /* código do startup */
    *(.text) *(.text.*)
} > text
```

### 3.4 `.data` e as constantes na RAM
```ld
.data :
{
    PROVIDE(__data_start = .);
    *(.data) *(.data.*)
    *(.rodata) *(.rodata.*)        /* <- constantes TAMBÉM vão para a RAM */
    PROVIDE(__data_end = .);
} > data AT> text                  /* usa na RAM, grava na Flash */
__data_load_start = LOADADDR(.data);
```
`> data AT> text` dá à seção **dois endereços**: onde ela é **usada** (RAM, chamado VMA) e onde é **gravada** (Flash, chamado LMA). É como uma mudança: os móveis viajam no caminhão (Flash) e são usados na casa nova (RAM); o startup descarrega o caminhão.

**Por que `.rodata` está aqui?** Porque no AVR um ponteiro comum (`const char *p`) só enxerga o espaço de dados. Se a constante ficasse só na Flash, `p` leria o endereço errado. Então o avr-gcc coloca `const` na RAM, e você paga em RAM por cada texto e tabela constante.

### 3.5 PROGMEM: constantes só na Flash
Para dados grandes (textos, tabelas de seno, fontes de display), põe-se a constante numa seção que fica na Flash e lê-se com a instrução `LPM`:
```c
#define PROGMEM __attribute__((section(".progmem.data")))
const char texto_flash[] PROGMEM = "so na Flash (PROGMEM)";

static inline uint8_t flash_read_byte(const void *addr)
{
    uint8_t r;
    __asm__ __volatile__ ("lpm %0, Z" : "=r"(r) : "z"(addr));   /* Z = endereço; LPM lê a Flash */
    return r;
}
```
O preço: não dá para passar `texto_flash` para funções que esperam um ponteiro comum (como `kprintf("%s")`); é preciso ler byte a byte. (A avr-libc oferece `PROGMEM`, `pgm_read_byte` e `printf_P` com esse fim; o GCC moderno também tem o qualificador `__flash`.)

### 3.6 `.bss`, `.noinit`, EEPROM
```ld
.bss (NOLOAD)    : { __bss_start = .; *(.bss) *(.bss.*) *(COMMON) __bss_end = .; } > data
.noinit (NOLOAD) : { *(.noinit*) } > data
__heap_start = .;                       /* RAM livre começa aqui */
.eeprom          : { KEEP(*(.eeprom*)) } > eeprom
```
- `NOLOAD`: ocupa endereços, mas não tem conteúdo a gravar (o startup zera o `.bss`).
- `.noinit`: fora da área zerada; sobrevive a um reset sem desligar a energia (veja `reset_count` no exemplo).
- `.eeprom`: variáveis declaradas com `__attribute__((section(".eeprom")))` ganham endereço na EEPROM, e o valor inicial vai para o arquivo `.eep` (módulo 16).

### 3.7 O mapa da RAM do ATmega328P
```
0x08FF ┌──────────────┐ <- __stack (SP inicial)
       │ pilha ↓      │
       │ (livre)      │   ~1,2 KB neste exemplo
       ├──────────────┤ <- __heap_start
       │ .noinit      │
       │ .bss         │
       │ .data+rodata │   valores iniciais copiados da Flash
0x0100 └──────────────┘
0x00FF   registradores de E/S e r0..r31
```
Com só 2 KB, a pilha pode encostar nas variáveis sem nenhum aviso: o programa passa a se comportar de forma estranha. Meça sempre (exercício 3).

---

## 4. O linker script do UC3A
```ld
MEMORY
{
    FLASH    : ORIGIN = 0x80000000, LENGTH = 512K
    INTRAM   : ORIGIN = 0x00000004, LENGTH = 0x0000FFFC   /* SRAM sem o endereço 0 */
    USERPAGE : ORIGIN = 0x80800000, LENGTH = 512
}
.reset 0x80000000 : { KEEP(*(.reset)) } > FLASH            /* trampolim */
.text  0x80002000 : { *(.text*) *(.rodata*) } > FLASH       /* depois do bootloader */
.exception : { . = ALIGN(0x200); KEEP(*(.exception)) } > FLASH
.data : { ... } > INTRAM AT> FLASH
```
- Mapa único: `const` fica na Flash e é lida normalmente por ponteiros.
- A RAM começa em `0x4`: assim nenhuma variável fica no endereço 0, e um ponteiro nulo não aponta para dado válido.
- A tabela de exceções precisa estar alinhada (o EVBA exige).
- `ASSERT(. + _stack_size <= _estack, ...)`: o link falha se não sobrarem 4 KB de RAM para a pilha.

---

## 5. Ferramentas
```sh
avr-size -A build/m328p/03-linker-script.elf     # tamanho de cada seção
avr-nm -n -S build/m328p/03-linker-script.elf    # símbolos com endereço e tamanho
less build/m328p/03-linker-script.map            # de onde veio cada byte
```

---

## Exercícios
1. Troque um `kprintf` longo por uma versão que lê o texto da Flash com `flash_read_byte` e meça a economia de RAM no `avr-size`.
2. Crie `const uint8_t tabela[1500]` sem PROGMEM e compile para o ATmega: o que o linker diz?
3. Calcule a RAM livre (`SP − __heap_start`) no começo do `main` e dentro de uma função recursiva.
4. Aperte RESET várias vezes e depois desligue o USB: explique o comportamento do contador `.noinit`.
