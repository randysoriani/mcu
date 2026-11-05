# Módulo 03 — Linker script e organização do programa na memória

> **Objetivo deste módulo:** entender como o seu código C vira bytes gravados em endereços exatos da Flash e da RAM, e quem decide esses endereços. No PC o sistema operacional resolve isso sozinho; no microcontrolador **nós** escrevemos as regras, num arquivo chamado *linker script*.

Arquivos:
- [`../common/123/linker.ld`](../common/123/linker.ld) — TM4C123GH6PM (256 KB de Flash, 32 KB de RAM)
- [`../common/129/linker.ld`](../common/129/linker.ld) — TM4C1294NCPDT (1 MB de Flash, 256 KB de RAM)
- [`src/main.c`](src/main.c) — imprime onde cada coisa foi parar, testa uma variável que sobrevive ao reset e uma função que roda na RAM

```sh
make MCU=123 flash && picocom -b 115200 /dev/ttyACM0
# aperte o botão RESET algumas vezes e veja o contador de resets subir
```

---

## 1. Do código-fonte ao arquivo gravado: compilar e linkar

Quando você digita `make`, acontecem duas etapas bem diferentes:

```
 main.c ──[compilador]──► main.o ─┐
 uart.c ──[compilador]──► uart.o ─┼──[linker + linker.ld]──► programa.elf ──[objcopy]──► programa.bin
 startup.c ─[compilador]► startup.o┘
```

1. **Compilar**: o compilador traduz **cada arquivo `.c` separadamente** em código de máquina, gerando um *arquivo objeto* (`.o`). Nesse momento ele **não sabe em que endereço** cada função ou variável vai morar. Se `main.c` chama `uart_init()`, que está em outro arquivo, o compilador deixa uma anotação: "aqui vai o endereço de `uart_init`, a preencher".
2. **Linkar**: o *linker* junta todos os `.o`, **decide o endereço final** de cada função e variável e preenche essas anotações. As regras de onde colocar cada coisa vêm do **linker script**.

Analogia: os `.o` são as matérias de um jornal, escritas separadamente; o linker é o diagramador, que decide em que página cada matéria entra; o linker script é o modelo de diagramação ("a capa vai na página 1").

O resultado é um arquivo **ELF** (`.elf`): os bytes do programa, o endereço de cada parte e informações para o depurador (nomes, números de linha). O `objcopy` extrai só os bytes crus (`.bin`), que é o que vai fisicamente para a Flash.

---

## 2. Seções: os grupos em que o compilador separa as coisas

O compilador coloca cada coisa que gera num grupo com nome, chamado **seção**:

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
| `.isr_vector` | a tabela de vetores (módulo 02) | **endereço 0x0000_0000** | o núcleo a procura lá |
| pilha | variáveis locais, endereços de retorno | RAM (no fim) | criada em tempo de execução |

Variáveis **locais** (dentro de uma função) não ficam em seção nenhuma: são criadas na pilha quando a função é chamada e somem quando ela retorna.

Com `-ffunction-sections -fdata-sections` (usadas no curso), cada função e variável ganha a sua própria subseção (`.text.pisca`, `.data.contador`), e o linker pode **jogar fora** o que ninguém usa (`--gc-sections`).

---

## 3. O linker script, parte por parte

O linker script responde três perguntas: **que memórias existem**, **onde vai cada seção** e **quais endereços o programa precisa saber**. Vamos ao do TM4C123 ([`common/123/linker.ld`](../common/123/linker.ld)).

### 3.1 `ENTRY`: o ponto de entrada
```ld
ENTRY(Reset_Handler)
```
Anota no ELF qual é a primeira função (para o depurador) e impede que ela seja descartada. **O hardware ignora isso**: quem decide onde o processador começa é a tabela de vetores.

### 3.2 `MEMORY`: que memórias existem
```ld
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 256K     /* 129: 1024K */
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 32K      /* 129: 256K  */
}
```
Nome, endereço inicial (`ORIGIN`) e tamanho (`LENGTH`) de cada memória, tirados do mapa de memória (módulo 01). No TM4C a Flash começa em **zero**. Se o programa ficar maior que a Flash, o link falha com `region 'FLASH' overflowed by N bytes`.

### 3.3 Símbolos: endereços que o programa precisa conhecer
```ld
_estack = ORIGIN(RAM) + LENGTH(RAM);     /* 0x20000000 + 32K = 0x20008000 */
```
Cria um **símbolo**, um nome associado a um endereço, que o C pode usar. `_estack` é o fim da RAM, onde a pilha começa. A pilha do Cortex-M cresce "para baixo", então começar no fim aproveita todo o espaço livre.

No C, um símbolo do linker é declarado como `extern` e **usamos o endereço dele**:
```c
extern uint32_t _estack;
uint32_t topo = (uint32_t)&_estack;     /* 0x20008000 */
```

Dentro do script, o ponto `.` significa "o endereço atual", como o cursor de um editor que avança à medida que as coisas são colocadas.

### 3.4 `SECTIONS`: onde vai cada coisa

A tabela de vetores, primeiro de tudo:
```ld
.isr_vector :
{
    . = ALIGN(4);              /* endereço múltiplo de 4 */
    KEEP(*(.isr_vector))       /* pegue a seção .isr_vector de todos os arquivos */
    . = ALIGN(4);
} > FLASH                      /* e coloque na Flash: como é a primeira, cai em 0x0 */
```
- `*(.isr_vector)`: "de todos os arquivos (`*`), pegue as seções `.isr_vector`".
- `KEEP(...)`: não descarte, mesmo que o código não a use (quem a usa é o hardware).

Depois o código e as constantes:
```ld
.text   : { *(.text) *(.text*) _etext = .; } > FLASH
.rodata : { *(.rodata) *(.rodata*) } > FLASH
```

### 3.5 O truque do `.data`: um endereço para gravar, outro para usar

A variável `int contador = 100;`:
- **durante a execução**, precisa estar na **RAM** (o programa a altera);
- mas o valor `100` precisa ser **gravado na Flash**, senão se perderia ao desligar.

Então a seção `.data` tem **dois endereços**:

| Nome técnico | Significado | Onde |
|---|---|---|
| **VMA** (*Virtual Memory Address*) | endereço em que ela é **usada** | RAM |
| **LMA** (*Load Memory Address*) | endereço em que ela é **gravada** | Flash, logo depois das constantes |

Analogia: uma mudança de casa. Os móveis viajam no caminhão (LMA, a Flash) e são usados na casa nova (VMA, a RAM). Alguém descarrega o caminhão antes: o startup (módulo 02).

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

Para ver os dois endereços:
```sh
arm-none-eabi-objdump -h build/123/03-linker-script.elf
```
Na linha do `.data`, a coluna VMA mostra `2000...` (RAM) e a LMA mostra um endereço baixo, na Flash. Nas demais seções da Flash, VMA e LMA são iguais.

### 3.6 `.bss`: só reservar espaço
```ld
.bss (NOLOAD) : { _sbss = .; *(.bss) *(.bss*) *(COMMON) _ebss = .; } > RAM
```
`NOLOAD`: ocupa endereços, mas não tem conteúdo a gravar. O startup zera a área ao ligar. Por isso um `uint8_t buffer[10000];` não aumenta o tamanho do arquivo gravado.

### 3.7 `.noinit`: variáveis que sobrevivem ao reset
```ld
.noinit (NOLOAD) : { *(.noinit*) } > RAM
```
Fica fora da área que o startup zera. Uma variável ali mantém o valor depois de um reset pelo botão, pelo watchdog ou por software (a RAM continua alimentada), mas tem lixo depois que a energia é desligada.
```c
__attribute__((section(".noinit"))) uint32_t reset_count;
```
Combine com um "número mágico" para saber se o valor é válido (veja o `main.c`). O exemplo também imprime `SYSCTL_RESC`, o registrador que diz a causa do último reset (módulo 19).

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
Não reserva a pilha de verdade: só obriga o linker a **reclamar** se não sobrar RAM suficiente para ela. A pilha real começa em `_estack` e cresce para baixo. O *heap* é a área do `malloc()`, que o curso não usa.

Como fica a RAM:
```
0x20008000 (123) / 0x20040000 (129)  <- _estack (SP inicial)
           │   pilha  ↓   │   cresce para baixo a cada chamada de função
           │   (livre)    │
           ├──────────────┤ <- end
           │   .noinit    │
           ├──────────────┤ <- _ebss
           │   .bss       │   variáveis zeradas
           ├──────────────┤ <- _sbss = _edata
           │   .data      │   variáveis com valor inicial
0x20000000 └──────────────┘ <- _sdata
```

> Se a pilha crescer demais (recursão, vetores locais grandes), ela invade o `.bss` e corrompe variáveis, causando erros aparentemente aleatórios. A MPU do Cortex-M4 pode ser usada para detectar isso.

---

## 4. Executando uma função a partir da RAM

Às vezes é útil rodar código da RAM em vez da Flash, por exemplo para continuar executando enquanto a Flash é apagada ou gravada (módulo 22).

```c
__attribute__((section(".ramfunc"), noinline, long_call))
uint32_t soma_na_ram(uint32_t a, uint32_t b) { return a + b; }
```
- `section(".ramfunc")`: o linker script inclui `*(.ramfunc*)` dentro do `.data`; a função é gravada na Flash e copiada para a RAM pelo startup, junto com as variáveis.
- `long_call`: a instrução normal de chamada (`BL`) só alcança ±16 MB. A Flash (`0x0000_0000`) e a RAM (`0x2000_0000`) estão a 512 MB uma da outra, então o compilador precisa usar uma chamada "longa".

---

## 5. O map file: o relatório do linker

A cada compilação o linker gera `build/<mcu>/<modulo>.map`, mostrando de qual arquivo veio cada pedaço do programa, em que endereço e com que tamanho, e o que foi descartado.

```sh
less build/123/03-linker-script.map
arm-none-eabi-nm -n -S --size-sort build/123/03-linker-script.elf   # os maiores símbolos
arm-none-eabi-size -A build/123/03-linker-script.elf                # tamanho de cada seção
```

---

## 6. Mudanças comuns no linker script

- **Reservar uma área da Flash para dados** (módulo 22): diminua o `LENGTH` da FLASH em 1 KB (123) ou 16 KB (129) e crie uma região `DATA` no espaço liberado.
- **Programa depois de um bootloader**: `ORIGIN = 0x00002800` e, no início do programa, `SCB->VTOR = 0x2800;` (módulo 02).
- **Estrutura em endereço fixo** (ex.: versão do firmware):
  ```ld
  .fw_info 0x00000400 : { KEEP(*(.fw_info)) } > FLASH
  ```
- O 129 tem 256 KB de RAM: dá para criar seções próprias para buffers grandes (Ethernet, áudio), alinhados como o periférico exigir.

---

## Exercícios

1. Crie `const uint8_t tabela[4096]` e veja o impacto no `make size`. Tire o `const` e compare.
2. Descubra no `.map` quantos bytes a função `kprintf` ocupa.
3. Mude `_Min_Stack_Size` do 123 para `0x8000` e leia o erro do linker.
4. Crie uma seção `.fw_info` em endereço fixo e leia-a no GDB com `x/2xw 0x400`.
5. Aperte RESET várias vezes e depois desligue e religue o USB: explique a diferença no contador de resets.
