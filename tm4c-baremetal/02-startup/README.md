# Módulo 02 — Código de inicialização (startup)

> **Objetivo deste módulo:** entender tudo o que acontece entre ligar a placa e a primeira linha do seu `main()`. No computador isso fica escondido (BIOS, sistema operacional, carregador de programas). Num microcontrolador **não existe nada disso**. Esse trabalho é feito por um pequeno código chamado *startup*, que você vai ler linha por linha neste módulo e até escrever em assembly.

Arquivos deste módulo:

| Arquivo | O que é |
|---|---|
| [`123/startup_123.s`](123/startup_123.s) | startup em **assembly** para o TM4C123GH6PM |
| [`129/startup_129.s`](129/startup_129.s) | startup em **assembly** para o TM4C1294NCPDT |
| [`../common/123/startup.c`](../common/123/startup.c), [`../common/129/startup.c`](../common/129/startup.c) | o mesmo startup em **C**; é o que os outros módulos usam |
| [`src/main.c`](src/main.c) | programa de teste que confere se o startup fez o trabalho direito |

Para testar:
```sh
make MCU=123 flash      # ou MCU=129
```
- LED piscando **devagar** (uma vez por segundo): o startup funcionou.
- LED piscando **rápido** (dez vezes por segundo): o startup falhou, porque as variáveis globais não foram preparadas.

O LED é o verde (PF3) na EK-TM4C123GXL e o D1 (PN1) na EK-TM4C1294XL.

---

## 0. Antes de começar: alguns termos

Estas palavras aparecem o tempo todo. Volte aqui sempre que precisar.

| Termo | O que significa |
|---|---|
| **Microcontrolador (MCU)** | Um chip com processador, memória e periféricos (portas de entrada/saída, comunicação serial, timers...) dentro dele. É um "computador inteiro" num chip só. |
| **Núcleo (core)** | O processador propriamente dito. Nos TM4C é um **ARM Cortex-M4F**, projetado pela ARM e licenciado para a Texas Instruments. |
| **Flash** | Memória que **não se apaga** quando falta energia. É onde fica o seu programa. |
| **RAM** (SRAM) | Memória rápida que **se apaga** sem energia. É onde ficam as variáveis enquanto o programa roda. |
| **Endereço** | Um número que identifica uma posição de memória. Tudo no chip tem endereço: Flash, RAM e até os periféricos. Escrevemos em hexadecimal, por exemplo `0x20000000`. |
| **Word** | Um bloco de 4 bytes (32 bits), o "tamanho natural" de dado do Cortex-M. |
| **Registrador** | Uma pequena memória dentro do processador (`r0`..`r15`) ou de um periférico. |
| **PC** (*Program Counter*) | Registrador com o **endereço da próxima instrução** a executar. |
| **SP** (*Stack Pointer*) | Registrador que aponta para o topo da **pilha**. |
| **Pilha (stack)** | Área da RAM usada automaticamente para variáveis locais, parâmetros e endereços de retorno das funções. Sem pilha, nenhuma função em C funciona. |
| **Reset** | O "começar do zero": ao ligar a placa, ao apertar o botão RESET ou por software. |
| **Interrupção** | Um aviso do hardware ("chegou um byte na serial", "o timer estourou") que faz o processador **parar o que está fazendo**, executar uma função específica e depois voltar. |
| **Exceção** | O nome genérico, na ARM, para tudo o que desvia o processador do fluxo normal: o reset, as interrupções e os erros (falhas). |
| **Handler** | A função que trata uma exceção ou interrupção. Ex.: `SysTick_Handler`, `TIMER0A_Handler`. |

---

## 1. O que acontece quando você liga a placa

O processador acorda sem saber nada: não sabe onde está o programa, onde está a RAM, e não tem pilha. A ARM resolveu isso com uma regra fixa, gravada no silício e igual em todo Cortex-M:

> **Ao sair do reset, o núcleo lê as duas primeiras words da memória:**
> 1. a word no endereço `0x0000_0000` vira o valor inicial do **SP** (onde começa a pilha);
> 2. a word no endereço `0x0000_0004` vira o valor inicial do **PC** (onde começa o programa).

A partir daí ele executa instruções a partir do endereço que leu. Essa primeira função chama-se, por convenção, **`Reset_Handler`**.

No TM4C a **Flash começa exatamente no endereço `0x0000_0000`**: o começo do seu programa gravado é o que o núcleo lê. (Se a Flash estiver apagada, a word no endereço 4 vale `0xFFFFFFFF` e o chip entra num bootloader gravado de fábrica na ROM.)

O estado do chip logo após o reset explica muita coisa dos próximos módulos:

| Item | Estado após o reset |
|---|---|
| Clock | **PIOSC**, um oscilador interno de 16 MHz. Funciona sempre, sem peças externas. |
| Periféricos | **todos desligados** (sem clock). No TM4C, mexer num periférico desligado **gera um erro** (HardFault) — é preciso ligá-lo antes (módulo 05). |
| Pinos | sem a função digital habilitada (módulo 05) |
| Interrupções | nenhuma habilitada |
| FPU (unidade de ponto flutuante) | **desligada** |
| RAM | conteúdo **aleatório** (lixo) depois de ligar a energia |

---

## 2. Por que precisamos de um startup?

Depois do reset o processador já sabe onde começar, mas o ambiente ainda **não está pronto para um programa em C**. A linguagem C promete algumas coisas que alguém precisa cumprir antes do `main()`:

| A linguagem C promete que... | Mas no hardware, após o reset... | Quem resolve |
|---|---|---|
| `int contador = 100;` (variável global) começa valendo 100 | a RAM tem lixo | o startup **copia** o 100 da Flash para a RAM |
| `int total;` (global sem valor) começa valendo 0 | a RAM tem lixo | o startup **zera** essa área |
| funções podem ser chamadas | é preciso uma pilha | o SP já foi carregado pelo hardware (seção 1) |
| `float` funciona | a FPU está desligada | o startup **liga** a FPU |
| quando ocorre uma interrupção, a função certa é chamada | o processador precisa saber o endereço de cada handler | a **tabela de vetores**, definida no startup |

O startup é um pequeno trecho de código que faz:

```
Reset_Handler:
  1. liga a FPU
  2. copia os valores iniciais das variáveis globais:  Flash -> RAM   (seção .data)
  3. zera as variáveis globais sem valor inicial                       (seção .bss)
  4. (opcional) chama "construtores"
  5. chama main()
  6. se main() retornar, fica parado num laço infinito
```

Além disso, ele **define a tabela de vetores**.

---

## 3. A tabela de vetores

### 3.1 O que é

A **tabela de vetores** é uma lista de endereços guardada logo no começo da Flash. Cada posição corresponde a um evento que pode acontecer no chip (o reset, um erro, a interrupção de um periférico). Em cada posição está **o endereço da função que deve ser executada quando aquele evento ocorrer**.

Uma analogia: é a **lista de ramais de emergência** colada na parede de uma empresa.

| Evento | "Ramal" (endereço da função) |
|---|---|
| Ligou a energia (reset) | `Reset_Handler` |
| Erro grave (HardFault) | `HardFault_Handler` |
| Passou 1 ms (SysTick) | `SysTick_Handler` |
| Apertaram o botão (interrupção da porta GPIO F) | `GPIOF_Handler` |
| Chegou um byte na serial (UART0) | `UART0_Handler` |
| ... | ... |

Quando o evento acontece, o processador **não consulta ninguém**: vai direto à posição correspondente da tabela, lê o endereço que está lá e salta para ele. Daí o nome "vetor": cada entrada *aponta* para uma função.

### 3.2 Para que serve

Sem essa tabela o processador não saberia:
- onde começa o programa (o vetor de reset, visto na seção 1);
- onde começa a pilha (a primeira entrada não é uma função: é o valor inicial do SP);
- o que fazer quando um periférico pede atenção.

Graças a ela o tratamento de interrupções é **rápido e automático**: em 12 ciclos de clock o processador já está executando o handler certo, sem nenhum código intermediário. É isso que torna os microcontroladores bons para tempo real.

### 3.3 Como ela é por dentro

Cada entrada ocupa 4 bytes; a entrada número *n* fica no endereço `4 × n`. A posição de cada evento é **fixa**, definida pela ARM (exceções do núcleo) e pela TI (periféricos):

| Posição | Endereço | Conteúdo | Quem definiu |
|---|---|---|---|
| 0 | 0x00 | **valor inicial do SP** (não é uma função!) | ARM |
| 1 | 0x04 | Reset | ARM |
| 2 | 0x08 | NMI (interrupção que não pode ser desligada) | ARM |
| 3 | 0x0C | HardFault (erro grave) | ARM |
| 4–6 | 0x10–0x18 | MemManage, BusFault, UsageFault (erros específicos) | ARM |
| 7–10 | 0x1C–0x28 | reservadas (devem conter 0) | ARM |
| 11 | 0x2C | SVCall (chamada de sistema, usada por sistemas operacionais de tempo real) | ARM |
| 12 | 0x30 | DebugMonitor | ARM |
| 13 | 0x34 | reservada | ARM |
| 14 | 0x38 | PendSV (troca de tarefas em sistemas operacionais) | ARM |
| 15 | 0x3C | SysTick (timer do núcleo, módulo 06) | ARM |
| 16 | 0x40 | **IRQ 0**: primeira interrupção de periférico (GPIO porta A) | TI |
| 17 | 0x44 | IRQ 1 (GPIO porta B) | TI |
| ... | ... | ... | TI |
| 16 + n | 0x40 + 4n | IRQ n | TI |

- As 16 primeiras posições são iguais em **qualquer** Cortex-M, de qualquer fabricante (as mesmas do STM32, por exemplo).
- Da posição 16 em diante, cada fabricante decide. O TM4C123 tem 139 posições de interrupção (tabela de 155 entradas, 620 bytes); o TM4C129 tem 114 (130 entradas, 520 bytes). Muitas são reservadas (periféricos que só existem em outros chips da família).
- A lista oficial está no datasheet de cada chip, capítulo *Interrupts*, tabela *Interrupts*.

As interrupções mais usadas no curso. **Atenção: a partir da posição 38 os números são diferentes nos dois chips.**

| Periférico | IRQ no 123 | IRQ no 129 |
|---|---|---|
| GPIO portas A..E | 0..4 | 0..4 |
| UART0 | 5 | 5 |
| ADC0, sequenciador 3 | 17 | 17 |
| Watchdog | 18 | 18 |
| TIMER0A / 1A / 2A | 19 / 21 / 23 | 19 / 21 / 23 |
| GPIO porta F | 30 | 30 |
| CAN0 | **39** | **38** |
| Hibernação | **43** | **41** |
| uDMA (software / erro) | **46 / 47** | **44 / 45** |
| GPIO porta J | — | 51 |
| GPIO porta N | — | 73 |
| FPU | 106 | 67 |
| PWM1 geradores 0..3 | 134..137 | — |

Os headers do curso definem os números (`TIMER0A_IRQn`...) e o startup define os nomes das funções (`TIMER0A_Handler`...).

### 3.4 Um exemplo real

Depois de compilar o módulo 05, veja os primeiros bytes da Flash:

```sh
arm-none-eabi-objdump -s -j .isr_vector build/123/05-gpio-blink.elf | head -6
```
```
 0000 00800020 cd020000 c9020000 c9020000  ................
 0010 c9020000 c9020000 c9020000 00000000  ................
```
(saída real do TM4C123; os endereços exatos mudam conforme o código)

Os bytes estão em ordem *little-endian* (o byte menos significativo vem primeiro). Lendo de 4 em 4 e invertendo:

| Bytes | Valor | Significado |
|---|---|---|
| `00 80 00 20` | `0x20008000` | posição 0: SP inicial = fim da RAM do 123 (32 KB a partir de `0x20000000`) |
| `cd 02 00 00` | `0x000002CD` | posição 1: endereço do `Reset_Handler` |
| `c9 02 00 00` | `0x000002C9` | posições 2, 3...: `Default_Handler` (nenhum handler próprio foi escrito) |
| `00 00 00 00` | 0 | posição reservada |

**Por que os endereços são ímpares** (`...CD`, `...C9`)? Os Cortex-M só executam instruções *Thumb*, e a ARM usa o **bit 0 do endereço** como marcador de "isto é código Thumb". A instrução está no endereço par (`0x2CC`); o compilador e o linker colocam o bit 1 sozinhos. Se ele vier 0, o processador gera um erro (HardFault).

### 3.5 Onde a tabela fica e por quê

O núcleo procura a tabela no endereço `0x0000_0000`, o início da Flash do TM4C. Por isso:
1. no código, a tabela vai para uma seção com nome próprio, `.isr_vector`;
2. o *linker script* (módulo 03) manda essa seção para o **primeiro endereço da Flash**, antes de todo o resto;
3. o linker é instruído a **não descartá-la** (`KEEP`), porque nenhuma função do programa "chama" a tabela: quem a usa é o hardware.

### 3.6 O que acontece se a tabela estiver errada

A posição de cada interrupção é fixa no silício. Se uma linha for esquecida, todas as seguintes "andam" uma posição, e a interrupção da serial passa a chamar a função do timer, por exemplo. Isso **não dá erro de compilação** e só aparece quando aquela interrupção específica acontece. Por isso:
- as tabelas do curso têm o número da IRQ comentado em cada linha;
- as versões em C e em assembly foram geradas automaticamente a partir da mesma lista.

---

## 4. Handlers "fracos" e o `Default_Handler`

A tabela do 123 tem 155 posições, e ninguém quer escrever 155 funções. A solução é declarar todos os handlers como **fracos (weak)** e apontá-los para uma função padrão:

```c
void Default_Handler(void) { for (;;) { } }     /* fica parado para sempre */

void TIMER0A_Handler(void) __attribute__((weak, alias("Default_Handler")));
```

Em português: *"existe uma função chamada `TIMER0A_Handler`; se ninguém definir outra com esse nome, use o `Default_Handler` no lugar dela"*.

- Se o seu código **não** tiver um `TIMER0A_Handler`, a entrada da tabela aponta para o `Default_Handler`.
- Se tiver `void TIMER0A_Handler(void) { ... }` (uma definição **forte**), o linker usa a sua versão automaticamente, sem mexer no startup.

É o que o `src/main.c` deste módulo faz com o `SysTick_Handler`.

Por que o `Default_Handler` trava num laço infinito? Uma interrupção sem tratamento indica um erro de programação, e travar num lugar conhecido facilita a depuração: no GDB, `p/x $xpsr & 0x1ff` mostra o número da exceção que levou o programa até ali (módulo 24).

Convenção do curso: `<PERIFÉRICO>_Handler` (`UART0_Handler`, `GPIOF_Handler`, `TIMER0A_Handler`, `ADC0SS3_Handler`...). Os exemplos da TI (TivaWare) usam outros nomes porque lá a tabela é editada à mão em cada projeto.

**Armadilha clássica:** errar o nome do handler. Compila sem erro, mas a interrupção cai no `Default_Handler`. Para conferir:
```sh
arm-none-eabi-nm build/123/x.elf | grep Handler
#   T = definido por você (forte)    W = ainda é o padrão (fraco)
```

---

## 5. Preparando as variáveis: `.data` e `.bss`

O compilador separa as variáveis globais em grupos chamados **seções**:

```c
int contador = 100;    /* tem valor inicial diferente de zero -> seção .data */
int total;             /* sem valor inicial (ou = 0)          -> seção .bss  */
const int tabela[3] = {1, 2, 3};   /* constante, só leitura   -> seção .rodata (fica na Flash) */
```

### `.data`: copiar da Flash para a RAM
`contador` precisa estar na RAM, porque o programa vai alterá-lo. Mas a RAM perde tudo sem energia. Então o valor `100` é gravado na Flash junto com o programa, e o startup o **copia** para a RAM antes do `main()`.

```
         Flash (não apaga)                    RAM (apaga)
   ┌───────────────────────┐          ┌───────────────────────┐
   │ código do programa    │          │ contador = 100 ...    │  <- .data
   │ ...                   │  cópia   │  ^ _sdata    ^ _edata │
   │ [100][...valores...]  │ ───────► │                       │
   │  ^ _sidata            │          │ total = 0 ...         │  <- .bss (zerada)
   └───────────────────────┘          │  ^ _sbss     ^ _ebss  │
                                      └───────────────────────┘
```

Como o startup sabe **onde** copiar? O linker script (módulo 03) cria símbolos com esses endereços:
- `_sidata`: onde os valores iniciais estão **na Flash**;
- `_sdata` e `_edata`: começo e fim da área de `.data` **na RAM**;
- `_sbss` e `_ebss`: começo e fim da área de `.bss`.

### `.bss`: zerar
A linguagem C garante que `total` começa valendo 0. Em vez de gravar zeros na Flash (desperdício), o linker só **reserva o espaço**, e o startup escreve zeros nele.

### O código em C (de `common/123/startup.c`)
```c
uint32_t *src = &_sidata;          /* origem: Flash */
uint32_t *dst = &_sdata;           /* destino: RAM  */
while (dst < &_edata) {            /* até o fim do .data */
    *dst++ = *src++;               /* copia 4 bytes e avança */
}

dst = &_sbss;
while (dst < &_ebss) {
    *dst++ = 0;                    /* zera 4 bytes e avança */
}
```

`_sdata` e os outros **não são variáveis**: são "etiquetas" de endereço criadas pelo linker. Por isso usamos `&_sdata`, nunca o "valor" dela.

---

## 6. Ligando a FPU

Os dois TM4C têm uma **FPU** (*Floating Point Unit*), um circuito que faz contas com `float` em hardware. Compilamos com `-mfloat-abi=hard`: o compilador usa instruções da FPU diretamente. Só que, após o reset, a FPU está **desligada**, e a primeira instrução de ponto flutuante gera um erro (UsageFault, que vira HardFault).

O startup a liga escrevendo no registrador **CPACR** (*Coprocessor Access Control*), em `0xE000_ED88`:

```c
SCB_CPACR |= (0xF << 20);          /* bits 23:20 = 1111: acesso total aos coprocessadores CP10 e CP11 (a FPU) */
__asm volatile ("dsb\n\tisb");     /* garante que a escrita terminou antes de seguir */
```
- `DSB` (*Data Synchronization Barrier*): espera todas as escritas pendentes terminarem;
- `ISB` (*Instruction Synchronization Barrier*): descarta instruções que o processador já tinha buscado antes da mudança.

Isso é feito **antes de tudo**, porque o compilador pode usar registradores da FPU até em cópias de memória.

---

## 7. O startup em assembly, linha a linha

Assembly é a linguagem que corresponde diretamente às instruções do processador. Ler o startup em assembly mostra que não há mágica.

```asm
    .syntax unified        @ estilo de sintaxe moderno da ARM
    .cpu    cortex-m4      @ para qual processador montar
    .fpu    fpv4-sp-d16    @ qual FPU ele tem
    .thumb                 @ Cortex-M só executa instruções Thumb
```
Linhas que começam com `.` são **diretivas**: ordens para o montador (assembler), não instruções do processador. `@` inicia um comentário.

```asm
    .section .text.Reset_Handler, "ax", %progbits   @ coloca o código numa seção ("ax" = alocável e executável)
    .type    Reset_Handler, %function               @ avisa que é função -> o linker põe o bit 0 = 1
Reset_Handler:                                      @ rótulo: dá nome a este endereço
    ldr   sp, =_estack            @ SP = topo da pilha
```

Ligando a FPU:
```asm
    ldr   r0, =0xE000ED88         @ r0 = endereço do CPACR
    ldr   r1, [r0]                @ r1 = valor atual do CPACR
    orr   r1, r1, #(0xF << 20)    @ liga os bits 23:20
    str   r1, [r0]                @ escreve de volta
    dsb
    isb
```

A cópia do `.data`, instrução por instrução:
```asm
    ldr   r0, =_sdata      @ r0 = destino (RAM)
    ldr   r1, =_edata      @ r1 = fim do destino
    ldr   r2, =_sidata     @ r2 = origem (Flash)
1:  cmp   r0, r1           @ chegou ao fim?
    ittt  lo               @ "If-Then": as 3 próximas instruções só executam se r0 < r1
    ldrlo r3, [r2], #4     @ r3 = memória[r2], e depois r2 += 4
    strlo r3, [r0], #4     @ memória[r0] = r3, e depois r0 += 4
    blo   1b               @ volta ao rótulo 1 ("b" de backward)
```

A tabela de vetores em assembly é só uma lista de `.word` (cada uma reserva 4 bytes com o valor indicado):
```asm
    .section .isr_vector, "a", %progbits     @ "a" = só alocável (dados, não código)
vector_table:
    .word _estack                 @ posição 0: SP inicial
    .word Reset_Handler           @ posição 1
    .word NMI_Handler             @ posição 2
    ...
    .word GPIOA_Handler           @ IRQ 0
    ...
```

E os handlers fracos:
```asm
    .weak      TIMER0A_Handler                    @ "fraco": pode ser substituído
    .thumb_set TIMER0A_Handler, Default_Handler   @ se não for, vale o Default_Handler
```

---

## 8. O startup em C

Ver [`common/123/startup.c`](../common/123/startup.c). São os mesmos passos:

```c
extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;
```
Diz ao compilador: "esses nomes existem, mas são definidos em outro lugar" (no linker script).

```c
__attribute__((section(".isr_vector"), used))
const vector_t vector_table[16 + 139] = {
    (vector_t)&_estack,      /* posição 0: SP inicial */
    Reset_Handler,           /* posição 1 */
    ...
};
```
- `section(".isr_vector")`: põe o array na seção que o linker manda para o início da Flash;
- `used`: impede o compilador de descartar o array (o código não o usa diretamente);
- `const`: fica na Flash, não na RAM.

```c
__attribute__((optimize("no-tree-loop-distribute-patterns")))
void Reset_Handler(void)
```
Com otimização, o GCC reconhece os laços de cópia/zeragem e os troca por `memcpy()`/`memset()` da biblioteca C. O atributo evita isso e deixa o startup independente de qualquer biblioteca.

### C ou assembly?
Fazem a mesma coisa. O de C é mais fácil de ler e manter; o de assembly mostra exatamente o que o processador executa. O curso usa o de C a partir daqui.

---

## 9. O programa de teste (`src/main.c`)

```c
uint32_t valor_data[4] = { 0x11111111, 0x22222222, 0x33333333, 0x44444444 };  /* .data */
uint32_t buffer_bss[16];                                                      /* .bss  */
```
O `main()` confere se `valor_data` tem os valores certos (a cópia funcionou) e se `buffer_bss` está zerado (a zeragem funcionou). Depois liga o SysTick; o `SysTick_Handler` do `main.c` (forte) substitui o fraco do startup e pisca o LED.

---

## 10. Relocando a tabela de vetores (assunto avançado)

Às vezes a tabela precisa ficar em outro lugar, por exemplo quando um *bootloader* (programa que grava outro programa) ocupa o começo da Flash. O registrador `SCB->VTOR` (*Vector Table Offset Register*) diz ao núcleo onde está a tabela; o endereço precisa ser alinhado a 1 KB (o tamanho da tabela arredondado para potência de 2). Uma aplicação gravada em `0x0000_2800` faz `SCB->VTOR = 0x2800;`.

---

## 11. Depurando o startup com o GDB

```sh
make MCU=123 debug
(gdb) monitor reset halt      # para o chip logo após o reset
(gdb) p/x $sp                 # 0x20008000: veio da posição 0 da tabela
(gdb) p/x $pc                 # endereço do Reset_Handler: veio da posição 1
(gdb) si                      # executa UMA instrução
(gdb) b main
(gdb) c
(gdb) p/x valor_data          # {0x11111111, 0x22222222, ...}
```
Os comandos do GDB estão explicados no módulo 24.

---

## Exercícios

1. Comente a cópia de `.data` no `startup_123.s` e observe o LED piscar rápido. (Depois de apertar RESET a RAM ainda guarda os valores antigos, o que pode esconder o problema; desligue e religue o USB.)
2. Remova a habilitação da FPU, use uma variável `float` no `main` e leia no GDB `p/x *(uint32_t*)0xE000ED28`; procure o bit 19 (NOCP, "sem coprocessador").
3. Escreva um `HardFault_Handler` que pisca o LED vermelho (PF1, no 123) num padrão diferente.
4. Por que um erro na posição de uma IRQ na tabela só aparece quando aquela interrupção é habilitada?
5. Ache no datasheet do seu chip a posição da interrupção da UART1 e confira no `objdump` o endereço gravado lá.
