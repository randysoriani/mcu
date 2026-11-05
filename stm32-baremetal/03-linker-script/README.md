# Módulo 03 — Linker script e organização do programa na memória

> **Objetivo deste módulo:** entender como o seu código C vira bytes gravados em endereços exatos da Flash e da RAM, e quem decide esses endereços. No PC o sistema operacional resolve isso sozinho; no microcontrolador **nós** escrevemos as regras, num arquivo chamado *linker script*.

Arquivos:
- [`../common/f0/linker.ld`](../common/f0/linker.ld) — STM32F072RB (128 KB de Flash, 16 KB de RAM)
- [`../common/f4/linker.ld`](../common/f4/linker.ld) — STM32F446RE (512 KB de Flash, 128 KB de RAM)
- [`src/main.c`](src/main.c) — imprime onde cada coisa foi parar, testa uma variável que sobrevive ao reset e uma função que roda na RAM

```sh
make MCU=f4 flash && picocom -b 115200 /dev/ttyACM0
# aperte o botão RESET (preto) algumas vezes e veja o contador de resets subir
```

---

## 1. Do código-fonte ao arquivo gravado: compilar e linkar

Quando você digita `make`, acontecem duas etapas bem diferentes:

```
 main.c ──[compilador]──► main.o ─┐
 uart.c ──[compilador]──► uart.o ─┼──[linker + linker.ld]──► programa.elf ──[objcopy]──► programa.bin
 startup.c ─[compilador]► startup.o┘
```

1. **Compilar**: o compilador (`arm-none-eabi-gcc -c`) traduz **cada arquivo `.c` separadamente** em código de máquina, gerando um *arquivo objeto* (`.o`). Nesse momento ele **não sabe em que endereço** cada função ou variável vai morar. Se `main.c` chama `uart_init()`, que está em outro arquivo, o compilador só deixa uma anotação: "aqui vai o endereço de `uart_init`, a preencher".
2. **Linkar**: o *linker* (`ld`) junta todos os `.o`, **decide o endereço final** de cada função e variável e preenche todas aquelas anotações. As regras de onde colocar cada coisa vêm do **linker script**.

Analogia: os `.o` são as matérias de um jornal, escritas separadamente; o linker é o diagramador, que decide em que página e posição cada matéria entra, e o linker script é o modelo de diagramação ("a capa vai na página 1, os classificados no final").

O resultado é um arquivo **ELF** (`.elf`): os bytes do programa, a indicação de em que endereço cada parte vai e informações para o depurador (nomes de funções, números de linha). O `objcopy` extrai dele só os bytes crus (`.bin`), que é o que vai fisicamente para a Flash.

---

## 2. Seções: os "grupos" em que o compilador separa as coisas

O compilador coloca cada coisa que gera num grupo com nome, chamado **seção**, conforme a natureza dela:

```c
int contador = 100;            /* .data   : variável com valor inicial      */
int total;                     /* .bss    : variável sem valor (vale 0)     */
const char nome[] = "LED";     /* .rodata : constante (read-only data)      */
void pisca(void) { ... }       /* .text   : código                          */
```

| Seção | O que contém | Precisa estar em | Por quê |
|---|---|---|---|
| `.text` | o código (instruções) | Flash | não muda, e precisa existir ao ligar |
| `.rodata` | constantes (`const`, textos entre aspas) | Flash | não muda |
| `.data` | variáveis globais com valor inicial ≠ 0 | **RAM** (com cópia do valor na Flash) | o programa altera, mas o valor inicial precisa vir de algum lugar |
| `.bss` | variáveis globais sem valor inicial (ou = 0) | RAM | o programa altera; começam em 0 (o startup zera) |
| `.isr_vector` | a tabela de vetores (nome escolhido por nós, módulo 02) | início da Flash | o núcleo a procura lá |
| pilha | variáveis locais, endereços de retorno | RAM (no fim) | criada em tempo de execução |

Variáveis **locais** (declaradas dentro de uma função) não aparecem em seção nenhuma: elas são criadas na pilha quando a função é chamada e somem quando ela retorna.

Com as opções `-ffunction-sections -fdata-sections` (usadas no curso), cada função e cada variável ganha a sua própria subseção (`.text.pisca`, `.data.contador`). Assim o linker consegue **jogar fora** o que ninguém usa (`--gc-sections`), deixando o programa menor. Para ver as seções de um `.o`:
```sh
arm-none-eabi-objdump -h build/f4/obj/main.o
```

---

## 3. O linker script, parte por parte

O linker script responde três perguntas: **que memórias existem**, **onde vai cada seção** e **quais endereços o programa precisa saber**. Vamos ao do F072 ([`common/f0/linker.ld`](../common/f0/linker.ld)).

### 3.1 `ENTRY`: o ponto de entrada
```ld
ENTRY(Reset_Handler)
```
Anota no ELF qual é a primeira função, para o depurador saber, e garante que o linker não a descarte. **O hardware ignora isso**: quem decide onde o processador começa é a tabela de vetores (módulo 02).

### 3.2 `MEMORY`: que memórias existem
```ld
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 128K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
}
```
Descreve as memórias físicas do chip: nome, onde começam (`ORIGIN`) e o tamanho (`LENGTH`). Os valores saem do mapa de memória do módulo 01. As letras entre parênteses (`r` leitura, `w` escrita, `x` execução) são informativas.

Se o programa ficar maior que a Flash, o link falha com `region 'FLASH' overflowed by N bytes`. É melhor descobrir isso no PC do que com a placa travando.

### 3.3 Símbolos: endereços que o programa precisa conhecer
```ld
_estack = ORIGIN(RAM) + LENGTH(RAM);     /* 0x20000000 + 16K = 0x20004000 */
```
Cria um **símbolo**, um nome associado a um endereço, que o código C pode usar. `_estack` é o fim da RAM, onde a pilha começa. A pilha do Cortex-M cresce "para baixo" (cada item novo fica num endereço menor), então começar exatamente no fim da RAM aproveita todo o espaço livre.

No C, um símbolo do linker é declarado como `extern` e **usamos o endereço dele**, nunca o valor:
```c
extern uint32_t _estack;
uint32_t topo = (uint32_t)&_estack;     /* 0x20004000 */
```

Dentro do script, o ponto `.` significa "o endereço atual" — como o cursor de um editor de texto, que avança à medida que as coisas vão sendo colocadas.

### 3.4 `SECTIONS`: onde vai cada coisa

A tabela de vetores, primeiro de tudo:
```ld
.isr_vector :
{
    . = ALIGN(4);              /* garante endereço múltiplo de 4 */
    KEEP(*(.isr_vector))       /* pegue a seção .isr_vector de todos os arquivos */
    . = ALIGN(4);
} > FLASH                      /* e coloque na região FLASH */
```
- `*(.isr_vector)`: "de todos os arquivos (`*`), pegue as seções chamadas `.isr_vector`".
- `KEEP(...)`: não descarte, mesmo que ninguém a use no código (quem a usa é o hardware).
- `> FLASH`: coloque na Flash. Como é a primeira seção, ela cai no endereço `0x0800_0000`.

Depois o código e as constantes:
```ld
.text :
{
    *(.text)
    *(.text*)          /* .text.main, .text.uart_init, ... */
    _etext = .;        /* símbolo: fim do código */
} > FLASH

.rodata : { *(.rodata) *(.rodata*) } > FLASH
```

### 3.5 O truque do `.data`: um endereço para gravar, outro para usar

Aqui está a parte que mais confunde. A variável `int contador = 100;`:
- **durante a execução**, precisa estar na **RAM** (o programa a altera);
- mas o valor `100` precisa ser **gravado na Flash**, senão se perderia ao desligar.

Então a seção `.data` tem **dois endereços**:

| Nome técnico | Significado | Onde |
|---|---|---|
| **VMA** (*Virtual Memory Address*) | endereço em que ela é **usada** | RAM |
| **LMA** (*Load Memory Address*) | endereço em que ela é **gravada** | Flash, logo depois das constantes |

Analogia: é como uma mudança de casa. Os móveis viajam guardados no caminhão (LMA, a Flash) e são usados na casa nova (VMA, a RAM). Alguém precisa descarregar o caminhão antes de usar os móveis: esse alguém é o startup (módulo 02).

```ld
_sidata = LOADADDR(.data);     /* LMA: onde os valores iniciais estão na Flash */

.data :
{
    _sdata = .;                /* início do .data na RAM */
    *(.data)
    *(.data*)
    *(.ramfunc*)               /* funções que rodam da RAM (seção 4) */
    _edata = .;                /* fim do .data na RAM */
} > RAM AT> FLASH              /* usa na RAM (VMA), grava na Flash (LMA) */
```

O código compilado usa os endereços da RAM; o gravador escreve os bytes na Flash; e o startup copia de `_sidata` para `_sdata.._edata`. Você pode ver os dois endereços com:
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
Nas seções que ficam na Flash, VMA e LMA são iguais; só o `.data` tem os dois diferentes.

### 3.6 `.bss`: só reservar espaço
```ld
.bss (NOLOAD) :
{
    _sbss = .;
    *(.bss) *(.bss*) *(COMMON)
    _ebss = .;
} > RAM
```
`NOLOAD` quer dizer "esta seção ocupa endereços, mas não tem conteúdo a gravar": não faz sentido gravar zeros na Flash, o startup zera a área ao ligar. É por isso que um `uint8_t buffer[10000];` não aumenta o tamanho do arquivo gravado.

### 3.7 `.noinit`: variáveis que sobrevivem ao reset
```ld
.noinit (NOLOAD) : { *(.noinit*) } > RAM
```
Fica **fora** da área que o startup zera. Uma variável ali mantém o valor depois de um reset pelo botão, pelo watchdog ou por software (a RAM continua alimentada), mas tem lixo depois que a energia é desligada. Uso típico: guardar a causa de um erro para ler no próximo boot. No C:
```c
__attribute__((section(".noinit"))) uint32_t reset_count;
```
Como ela começa com lixo após ligar a placa, combine-a com um "número mágico" para saber se o valor é válido (veja o `main.c`).

### 3.8 Pilha e heap
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
Isto **não reserva** a pilha de verdade: só obriga o linker a **reclamar** se, depois de `.data`, `.bss` e `.noinit`, não sobrar pelo menos `_Min_Heap_Size + _Min_Stack_Size` bytes de RAM livre. A pilha real começa em `_estack` e cresce para baixo. O *heap* é a área do `malloc()`; o curso não usa `malloc`.

Como fica a RAM do F072 depois de tudo isso:
```
0x20004000 ┌──────────────┐ <- _estack (SP inicial)
           │   pilha  ↓   │   cresce para baixo a cada chamada de função
           │              │
           │   (livre)    │
           │   heap   ↑   │   (se houver malloc)
           ├──────────────┤ <- end
           │   .noinit    │
           ├──────────────┤ <- _ebss
           │   .bss       │   variáveis zeradas
           ├──────────────┤ <- _sbss = _edata
           │   .data      │   variáveis com valor inicial
0x20000000 └──────────────┘ <- _sdata
```

> Não existe proteção contra *estouro de pilha* (stack overflow) no M0: se a pilha crescer demais, ela simplesmente invade o `.bss` e corrompe variáveis, causando erros aparentemente aleatórios. No M4 dá para usar a MPU para detectar isso.

---

## 4. Executando uma função a partir da RAM

Normalmente o código roda direto da Flash. Às vezes é útil rodar da RAM: ela não tem espera (*wait states*) e o código continua executando enquanto a Flash está ocupada sendo apagada ou gravada (módulo 22).

```c
__attribute__((section(".ramfunc"), noinline, long_call))
uint32_t soma_na_ram(uint32_t a, uint32_t b) { return a + b; }
```

- `section(".ramfunc")`: o linker script inclui `*(.ramfunc*)` dentro do `.data`. A função ganha endereço de uso na RAM e endereço de gravação na Flash, e o startup a copia junto com as variáveis. Nada mais precisa ser feito.
- `long_call`: a instrução normal de chamada de função (`BL`) só alcança ±16 MB de distância. A Flash (`0x0800_0000`) e a RAM (`0x2000_0000`) estão a uns 384 MB uma da outra, então o compilador precisa usar uma chamada "longa" (carregar o endereço num registrador e saltar com `BLX`).
- O endereço impresso (`0x20000011`) é ímpar: é código Thumb (módulo 02).

---

## 5. O map file: o "relatório" do linker

A cada compilação o linker gera `build/<mcu>/<modulo>.map`, mostrando, para cada seção do programa final, de qual arquivo veio cada pedaço, em que endereço e com que tamanho, além do que foi descartado.

```sh
grep -A3 "^ \.text\.main" build/f4/03-linker-script.map              # onde ficou o main
grep -A20 "Discarded input sections" build/f4/03-linker-script.map | head   # o que foi jogado fora
```

Outras ferramentas úteis:
```sh
arm-none-eabi-nm -n -S --size-sort build/f4/03-linker-script.elf   # os maiores símbolos
arm-none-eabi-size -A build/f4/03-linker-script.elf                # tamanho de cada seção
```

---

## 6. A saída do programa deste módulo

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

Compare com o mapa da RAM acima: `.data` começa em `0x20000000`, o `.bss` vem logo depois, o `.noinit` depois dele, e a pilha está lá no fim (`0x2001FFD0`, a poucos bytes de `_estack`).

---

## 7. Mudanças comuns no linker script

### Reservar uma área da Flash para guardar dados (módulo 22)
```ld
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 126K
    DATA  (r)   : ORIGIN = 0x0801F800, LENGTH = 2K     /* última página do F072 */
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
}
```

### Programa que começa depois de um bootloader
```ld
FLASH (rx) : ORIGIN = 0x08008000, LENGTH = 480K       /* bootloader ocupa os primeiros 32 KB */
```
e, no início do programa (F4), `SCB->VTOR = 0x08008000;` (módulo 02).

### Uma estrutura num endereço fixo (ex.: versão do firmware)
```ld
.fw_info 0x08000200 : { KEEP(*(.fw_info)) } > FLASH
```
```c
__attribute__((section(".fw_info"), used))
const struct { uint32_t magic, version; } fw_info = { 0xF1A5C0DE, 0x00010002 };
```

### Memória extra só da CPU (CCM RAM do F407/F429: 64 KB em `0x1000_0000`)
```ld
CCMRAM (rw) : ORIGIN = 0x10000000, LENGTH = 64K
...
.ccmram (NOLOAD) : { *(.ccmram*) } > CCMRAM
```

---

## Exercícios

1. Crie `const uint8_t tabela[4096]` e veja o impacto no `make size`. Tire o `const` e compare (olhe a coluna `data` e o uso de RAM).
2. Abra o `.map` e descubra quantos bytes a função `kprintf` ocupa.
3. Mude `_Min_Stack_Size` do F0 para `0x3C00` e compile. Leia a mensagem de erro e explique.
4. Crie uma seção `.fw_info` em endereço fixo e leia-a no GDB com `x/2xw 0x08000200`.
5. Faça o `reset_count` ser mostrado também depois de um reset pelo watchdog (combine com o módulo 19).
