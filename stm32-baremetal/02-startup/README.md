# Módulo 02 — Código de inicialização (startup)

> **Objetivo deste módulo:** entender tudo o que acontece entre ligar a placa e a primeira linha do seu `main()`. No computador isso fica escondido (BIOS, sistema operacional, carregador de programas). Num microcontrolador **não existe nada disso**. Esse trabalho é feito por um pequeno código chamado *startup*, que você vai ler linha por linha neste módulo e até escrever em assembly.

Arquivos deste módulo:

| Arquivo | O que é |
|---|---|
| [`f0/startup_f0.s`](f0/startup_f0.s) | startup em **assembly** para o STM32F072 (núcleo Cortex-M0) |
| [`f4/startup_f4.s`](f4/startup_f4.s) | startup em **assembly** para o STM32F446 (núcleo Cortex-M4F) |
| [`../common/f0/startup.c`](../common/f0/startup.c) | o mesmo startup escrito em **C**; é o que os outros módulos usam |
| [`../common/f4/startup.c`](../common/f4/startup.c) | idem para o F4 |
| [`src/main.c`](src/main.c) | programa de teste que confere se o startup fez o trabalho direito |

Para testar:
```sh
make MCU=f0 flash      # ou MCU=f4
```
- LED piscando **devagar** (uma vez por segundo): o startup funcionou.
- LED piscando **rápido** (dez vezes por segundo): o startup falhou, porque as variáveis globais não foram preparadas.

---

## 0. Antes de começar: alguns termos

Estas palavras aparecem o tempo todo. Volte aqui sempre que precisar.

| Termo | O que significa |
|---|---|
| **Microcontrolador (MCU)** | Um chip com processador, memória e periféricos (portas de entrada/saída, comunicação serial, timers...) dentro dele. É um "computador inteiro" num chip só. |
| **Núcleo (core)** | O processador propriamente dito, a parte que executa as instruções. Nos STM32 é um núcleo **ARM Cortex-M**, projetado pela ARM e licenciado para a ST. |
| **Flash** | Memória que **não se apaga** quando falta energia. É onde fica o seu programa (como o SSD de um PC). Só é gravada pelo gravador/debugger ou por código especial (módulo 22). |
| **RAM** (SRAM) | Memória rápida que **se apaga** sem energia. É onde ficam as variáveis enquanto o programa roda. |
| **Endereço** | Um número que identifica uma posição de memória. Tudo no chip tem endereço: Flash, RAM e até os periféricos. Escrevemos em hexadecimal, por exemplo `0x08000000`. |
| **Word** | Um bloco de 4 bytes (32 bits), o "tamanho natural" de dado do Cortex-M. |
| **Registrador** | Uma pequena memória dentro do processador (`r0`..`r15`) ou dentro de um periférico. Os do processador guardam os valores com que ele está trabalhando naquele momento. |
| **PC** (*Program Counter*) | Registrador com o **endereço da próxima instrução** a executar. Mudar o PC é pular para outro lugar do programa. |
| **SP** (*Stack Pointer*) | Registrador que aponta para o topo da **pilha**. |
| **Pilha (stack)** | Área da RAM usada automaticamente para variáveis locais, parâmetros e endereços de retorno das funções. Sem pilha, nenhuma função em C funciona. |
| **Reset** | O "começar do zero". Acontece ao ligar a placa, ao apertar o botão RESET ou por software. |
| **Interrupção** | Um aviso do hardware ("chegou um byte na serial", "o timer estourou") que faz o processador **parar o que está fazendo**, executar uma função específica e depois voltar de onde parou. |
| **Exceção** | O nome genérico, na ARM, para tudo o que desvia o processador do fluxo normal: o reset, as interrupções e os erros (falhas). |
| **Handler** | A função que trata uma exceção ou interrupção. Ex.: `SysTick_Handler` trata a interrupção do timer SysTick. |

---

## 1. O que acontece quando você liga a placa

Imagine que o processador acorda sem saber nada: não sabe onde está o programa, onde está a RAM, e não tem pilha. A ARM resolveu isso com uma regra simples e fixa, gravada no próprio silício:

> **Ao sair do reset, o núcleo Cortex-M lê as duas primeiras words da memória:**
> 1. a word no endereço `0x0000_0000` vira o valor inicial do **SP** (onde começa a pilha);
> 2. a word no endereço `0x0000_0004` vira o valor inicial do **PC** (onde começa o programa).

A partir daí ele executa instruções a partir do endereço que leu. Essa primeira função chama-se, por convenção, **`Reset_Handler`**.

O estado do chip logo após o reset explica muita coisa dos próximos módulos:

| Item | Estado após o reset |
|---|---|
| Clock | um oscilador interno simples, o **HSI**: 8 MHz no F0, 16 MHz no F4. É lento, mas sempre funciona. |
| Periféricos | **todos desligados** (sem clock). Para usar a porta do LED, por exemplo, é preciso ligá-la antes (módulo 05). |
| Interrupções | nenhuma habilitada |
| FPU (unidade de ponto flutuante do F4) | **desligada** |
| RAM | conteúdo **aleatório** (lixo) depois de ligar a energia |

> No STM32 a Flash fica no endereço `0x0800_0000`, não em `0x0000_0000`. O chip faz um "espelho": com o pino BOOT0 em nível baixo (como vem na Nucleo), quem lê `0x0000_0000` está, na verdade, lendo `0x0800_0000`. Por isso o seu programa, gravado no início da Flash, é exatamente o que o núcleo encontra.

---

## 2. Por que precisamos de um startup?

Depois do reset, o processador já sabe onde começar, mas o ambiente ainda **não está pronto para um programa em C**. A linguagem C promete algumas coisas que alguém precisa cumprir antes do `main()`:

| A linguagem C promete que... | Mas no hardware, após o reset... | Quem resolve |
|---|---|---|
| `int contador = 100;` (variável global) começa valendo 100 | a RAM tem lixo | o startup **copia** o 100 da Flash para a RAM |
| `int total;` (global sem valor) começa valendo 0 | a RAM tem lixo | o startup **zera** essa área |
| funções podem ser chamadas | é preciso uma pilha | o SP já foi carregado pelo hardware (seção 1) |
| `float` funciona (F4) | a FPU está desligada | o startup **liga** a FPU |
| quando ocorre uma interrupção, a função certa é chamada | o processador precisa saber o endereço de cada handler | a **tabela de vetores**, definida no startup |

O startup, então, é um pequeno trecho de código que faz:

```
Reset_Handler:
  1. (só F4) liga a FPU
  2. copia os valores iniciais das variáveis globais:  Flash -> RAM   (seção .data)
  3. zera as variáveis globais sem valor inicial                       (seção .bss)
  4. (opcional) chama "construtores"
  5. chama main()
  6. se main() retornar, fica parado num laço infinito
```

Além disso, ele **define a tabela de vetores**, explicada a seguir.

---

## 3. A tabela de vetores

### 3.1 O que é

A **tabela de vetores** é uma lista de endereços guardada logo no começo da Flash. Cada posição da lista corresponde a um evento que pode acontecer no chip (o reset, um erro, a interrupção de um periférico). Em cada posição está **o endereço da função que deve ser executada quando aquele evento ocorrer**.

Uma analogia: é a **lista de ramais de emergência** colada na parede de uma empresa.

| Evento | "Ramal" (endereço da função) |
|---|---|
| Ligou a energia (reset) | `Reset_Handler` |
| Erro grave (HardFault) | `HardFault_Handler` |
| Passou 1 ms (SysTick) | `SysTick_Handler` |
| Chegou um byte na serial (USART2) | `USART2_IRQHandler` |
| ... | ... |

Quando o evento acontece, o processador **não consulta ninguém**: vai direto à posição correspondente da tabela, lê o endereço que está lá e salta para ele. Daí o nome "vetor": cada entrada *aponta* para uma função.

### 3.2 Para que serve

Sem essa tabela o processador simplesmente não saberia:
- onde começa o programa (o vetor de reset, visto na seção 1);
- onde começa a pilha (a primeira entrada não é uma função: é o valor inicial do SP);
- o que fazer quando um periférico pede atenção.

Graças a ela o tratamento de interrupções é **rápido e automático**. Em 12 a 16 ciclos de clock o processador já está executando o handler certo, sem nenhum código intermediário procurando "quem chamou". É isso que torna os microcontroladores bons para tempo real.

### 3.3 Como ela é por dentro

Cada entrada ocupa 4 bytes (uma word); a entrada número *n* fica no endereço `início + 4 × n`. A posição de cada evento é **fixa**, definida pela ARM (exceções do núcleo) e pela ST (periféricos):

| Posição | Endereço (offset) | Conteúdo | Quem definiu |
|---|---|---|---|
| 0 | 0x00 | **valor inicial do SP** (não é uma função!) | ARM |
| 1 | 0x04 | Reset | ARM |
| 2 | 0x08 | NMI (interrupção que não pode ser desligada) | ARM |
| 3 | 0x0C | HardFault (erro grave) | ARM |
| 4–6 | 0x10–0x18 | MemManage, BusFault, UsageFault (só no M4; no M0 são reservadas) | ARM |
| 7–10 | 0x1C–0x28 | reservadas (devem conter 0) | ARM |
| 11 | 0x2C | SVCall (chamada de sistema, usada por sistemas operacionais de tempo real) | ARM |
| 12–13 | 0x30–0x34 | DebugMonitor (M4) / reservadas | ARM |
| 14 | 0x38 | PendSV (troca de tarefas em sistemas operacionais) | ARM |
| 15 | 0x3C | SysTick (timer do núcleo, módulo 06) | ARM |
| 16 | 0x40 | **IRQ 0**: primeira interrupção de periférico (WWDG) | ST |
| 17 | 0x44 | IRQ 1 | ST |
| ... | ... | ... | ST |
| 16 + n | 0x40 + 4n | IRQ n | ST |

- As 16 primeiras posições são iguais em **qualquer** Cortex-M, de qualquer fabricante.
- Da posição 16 em diante, cada fabricante decide qual periférico fica onde. O STM32F072 tem 32 interrupções de periférico (tabela de 48 entradas, 192 bytes); o STM32F446 tem 97 (113 entradas, 452 bytes).
- A lista oficial está no Reference Manual, capítulo *Interrupts and events*, tabela *Vector table* (RM0091 para o F072, RM0390 para o F446).

### 3.4 Um exemplo real

Depois de compilar este módulo, veja os primeiros bytes da Flash:

```sh
arm-none-eabi-objdump -s -j .isr_vector build/f4/02-startup.elf | head -6
```
A saída é parecida com esta (os endereços exatos mudam conforme o código):
```
 8000000 00000220 6d020008 c9020008 c9020008  ...
 8000010 c9020008 c9020008 c9020008 00000000  ...
 ...
 8000030 c9020008 00000000 c9020008 c5010008  ...
```

Os bytes estão em ordem *little-endian* (o byte menos significativo vem primeiro). Lendo de 4 em 4 e invertendo a ordem:

| Bytes na memória | Valor | Significado |
|---|---|---|
| `00 00 02 20` | `0x20020000` | posição 0: SP inicial = fim da RAM do F446 (128 KB a partir de `0x20000000`) |
| `6d 02 00 08` | `0x0800026D` | posição 1: endereço do `Reset_Handler` |
| `c9 02 00 08` | `0x080002C9` | posições 2, 3...: `Default_Handler` (nenhum handler próprio foi escrito) |
| `00 00 00 00` | 0 | posições reservadas |
| `c5 01 00 08` | `0x080001C5` | posição 15: o `SysTick_Handler` do nosso `main.c` |

**Por que os endereços são ímpares** (`...6D`, `...C9`)? Os Cortex-M só executam o conjunto de instruções *Thumb*, e a ARM usa o **bit 0 do endereço** como marcador de "isto é código Thumb". A instrução está, de fato, no endereço par (`0x0800026C`); o compilador e o linker colocam o bit 1 sozinhos. Se ele vier 0, o processador gera um erro (HardFault).

### 3.5 Onde a tabela fica e por quê

O núcleo procura a tabela no endereço `0x0000_0000`, que no STM32 é o espelho do início da Flash. Por isso:
1. no código, a tabela vai para uma seção com nome próprio, `.isr_vector`;
2. o *linker script* (módulo 03) manda essa seção para o **primeiro endereço da Flash**, antes de todo o resto do programa;
3. o linker é instruído a **não descartá-la** (`KEEP`), porque nenhuma função do programa "chama" a tabela: quem a usa é o hardware.

### 3.6 O que acontece se a tabela estiver errada

A posição de cada interrupção é fixa no silício. Se uma linha for esquecida, todas as seguintes "andam" uma posição, e a interrupção da serial passa a chamar a função do timer, por exemplo. Isso **não dá erro de compilação** e só aparece quando aquela interrupção específica acontece. Por isso:
- as tabelas do curso têm o número da IRQ comentado em cada linha;
- as versões em C e em assembly foram geradas a partir da mesma lista.

---

## 4. Handlers "fracos" e o `Default_Handler`

A tabela do F446 tem 113 posições, e ninguém quer escrever 113 funções. A solução é declarar todos os handlers como **fracos (weak)** e apontá-los para uma função padrão:

```c
void Default_Handler(void) { for (;;) { } }     /* fica parado para sempre */

void TIM2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
```

Em português: *"existe uma função chamada `TIM2_IRQHandler`; se ninguém definir outra com esse nome, use o `Default_Handler` no lugar dela"*.

- Se o seu código **não** tiver um `TIM2_IRQHandler`, a entrada da tabela aponta para o `Default_Handler`.
- Se tiver `void TIM2_IRQHandler(void) { ... }` (uma definição **forte**), o linker usa a sua versão automaticamente, sem mexer no startup.

É o que o `src/main.c` deste módulo faz com o `SysTick_Handler`.

Por que o `Default_Handler` trava num laço infinito? Porque uma interrupção sem tratamento indica um erro de programação, e travar num lugar conhecido facilita a depuração. No GDB, `p/x $xpsr & 0x1ff` mostra o número da exceção que levou o programa até ali (módulo 24).

**Armadilha clássica:** errar o nome do handler (`USART2_IRQHandle`, ou `TIM6_IRQHandler` em vez de `TIM6_DAC_IRQHandler`). Compila sem erro, mas a interrupção cai no `Default_Handler`. Para conferir:
```sh
arm-none-eabi-nm build/f4/x.elf | grep Handler
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
A linguagem C garante que `total` começa valendo 0. Em vez de gravar milhares de zeros na Flash (desperdício), o linker só **reserva o espaço**, e o startup escreve zeros nele.

### O código em C (de `common/f4/startup.c`)
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

`_sdata` e os outros **não são variáveis**: são "etiquetas" de endereço criadas pelo linker. Por isso usamos `&_sdata` (o endereço da etiqueta), nunca o "valor" dela.

---

## 6. Ligando a FPU (só STM32F4)

O F4 tem uma **FPU** (*Floating Point Unit*), um circuito que faz contas com `float` em hardware, muito mais rápido que em software. Compilamos com `-mfloat-abi=hard`, ou seja, o compilador usa instruções da FPU diretamente. Só que, após o reset, a FPU está **desligada**, e a primeira instrução de ponto flutuante gera um erro (UsageFault, que vira HardFault).

O startup a liga escrevendo no registrador **CPACR** (*Coprocessor Access Control*), no endereço `0xE000_ED88`:

```c
SCB_CPACR |= (0xF << 20);          /* bits 23:20 = 1111: acesso total aos coprocessadores CP10 e CP11 (a FPU) */
__asm volatile ("dsb\n\tisb");     /* garante que a escrita terminou antes de seguir */
```
- `DSB` (*Data Synchronization Barrier*): espera todas as escritas pendentes terminarem;
- `ISB` (*Instruction Synchronization Barrier*): descarta instruções que o processador já tinha buscado antes da mudança.

Isso é feito **antes de tudo**, porque o compilador pode usar registradores da FPU até em cópias de memória.

---

## 7. O startup em assembly, linha a linha (F0)

Assembly é a linguagem que corresponde diretamente às instruções do processador. Ler o startup em assembly mostra que não há mágica: são poucas instruções.

```asm
    .syntax unified        @ estilo de sintaxe moderno da ARM
    .cpu    cortex-m0      @ para qual processador montar
    .thumb                 @ Cortex-M só executa instruções Thumb
```
Linhas que começam com `.` são **diretivas**: ordens para o montador (assembler), não instruções do processador. `@` inicia um comentário.

```asm
    .section .text.Reset_Handler, "ax", %progbits   @ coloca o código numa seção ("ax" = alocável e executável)
    .type    Reset_Handler, %function               @ avisa que é função -> o linker põe o bit 0 = 1
Reset_Handler:                                      @ rótulo: dá nome a este endereço
    ldr   r0, =_estack     @ r0 recebe o endereço do topo da pilha
    mov   sp, r0           @ sp = r0 (o M0 não aceita "ldr sp, =..." direto)
```

A cópia do `.data` no M0, instrução por instrução:

```asm
    ldr   r0, =_sdata      @ r0 = início do .data na RAM (destino)
    ldr   r1, =_edata      @ r1 = fim do .data na RAM
    ldr   r2, =_sidata     @ r2 = início dos valores na Flash (origem)
    movs  r3, #0           @ r3 = deslocamento, começa em 0
    b     2f               @ pula para o rótulo 2 (para frente: "f" de forward)
1:  ldr   r4, [r2, r3]     @ r4 = memória[origem + deslocamento]
    str   r4, [r0, r3]     @ memória[destino + deslocamento] = r4
    adds  r3, r3, #4       @ deslocamento += 4 (uma word)
2:  adds  r4, r0, r3       @ r4 = endereço de destino atual
    cmp   r4, r1           @ compara com o fim
    bcc   1b               @ se for menor, volta ao rótulo 1 ("b" de backward)
```

No M4 (Thumb-2, com mais instruções) a mesma cópia fica mais curta:
```asm
1:  cmp   r0, r1
    ittt  lo               @ "If-Then": as 3 próximas instruções só executam se r0 < r1
    ldrlo r3, [r2], #4     @ r3 = *r2, e depois r2 += 4
    strlo r3, [r0], #4     @ *r0 = r3, e depois r0 += 4
    blo   1b
```

A tabela de vetores em assembly é só uma lista de `.word` (cada uma reserva 4 bytes com o valor indicado):
```asm
    .section .isr_vector, "a", %progbits     @ "a" = só alocável (dados, não código)
vector_table:
    .word _estack                 @ posição 0: SP inicial
    .word Reset_Handler           @ posição 1
    .word NMI_Handler             @ posição 2
    ...
```

E os handlers fracos:
```asm
    .weak      TIM2_IRQHandler                    @ "fraco": pode ser substituído
    .thumb_set TIM2_IRQHandler, Default_Handler   @ se não for, vale o Default_Handler
```

---

## 8. O startup em C

Ver [`common/f4/startup.c`](../common/f4/startup.c). São os mesmos passos, com alguns detalhes:

```c
extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;
```
Diz ao compilador: "esses nomes existem, mas são definidos em outro lugar" (no linker script).

```c
__attribute__((section(".isr_vector"), used))
const vector_t vector_table[16 + 97] = {
    (vector_t)&_estack,      /* posição 0: SP inicial (convertido para caber no array) */
    Reset_Handler,           /* posição 1 */
    NMI_Handler,
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
Com otimização, o GCC reconhece os laços de cópia/zeragem e os troca por chamadas a `memcpy()`/`memset()` da biblioteca C. Funcionaria, mas o atributo deixa o startup independente de qualquer biblioteca. (Teste: remova-o e procure `bl memcpy` com `make disasm`.)

### C ou assembly?
Fazem a mesma coisa. O de C é mais fácil de ler e manter; o de assembly mostra exatamente o que o processador executa e é útil quando nem a pilha é confiável (bootloaders). A ST fornece startups em assembly nos pacotes dela; o curso usa o de C a partir daqui.

---

## 9. O programa de teste (`src/main.c`)

```c
uint32_t valor_data[4] = { 0x11111111, 0x22222222, 0x33333333, 0x44444444 };  /* .data */
uint32_t buffer_bss[16];                                                      /* .bss  */
```
O `main()` confere se `valor_data` tem os valores certos (prova de que a cópia funcionou) e se `buffer_bss` está zerado (prova de que a zeragem funcionou). Depois liga o SysTick; o `SysTick_Handler` definido no `main.c` (forte) substitui o fraco do startup e pisca o LED.

Este teste foi executado no QEMU (máquina `netduinoplus2`, um STM32F405): com o startup em assembly, `valor_data` tem os valores iniciais, `buffer_bss` é zero e o período do pisca ficou no valor de "tudo certo".

---

## 10. Relocando a tabela de vetores (assunto avançado)

Às vezes a tabela precisa ficar em outro lugar, por exemplo quando um *bootloader* (programa que grava outro programa) ocupa o começo da Flash.
- **Cortex-M4 (F4)**: o registrador `SCB->VTOR` (*Vector Table Offset Register*) diz ao núcleo onde está a tabela. Uma aplicação que começa em `0x0800_8000` faz `SCB->VTOR = 0x08008000;`. O endereço precisa ser alinhado ao tamanho da tabela arredondado para potência de 2 (512 bytes no F446).
- **Cortex-M0 (F0)**: não tem VTOR. A solução da ST é copiar a tabela para o início da RAM e mandar o chip espelhar a RAM no endereço 0 (`SYSCFG->CFGR1`, campo MEM_MODE = 11). O linker script precisa reservar os primeiros 192 bytes da RAM.

---

## 11. Depurando o startup com o GDB

```sh
make MCU=f4 debug
(gdb) monitor reset halt      # para o chip logo após o reset
(gdb) p/x $sp                 # 0x20020000: veio da posição 0 da tabela
(gdb) p/x $pc                 # endereço do Reset_Handler: veio da posição 1
(gdb) si                      # executa UMA instrução
(gdb) x/4xw &_sdata           # a RAM antes da cópia: lixo
(gdb) b main
(gdb) c
(gdb) p/x valor_data          # depois do startup: {0x11111111, 0x22222222, ...}
```
Os comandos do GDB estão explicados em detalhe no módulo 24.

---

## Exercícios

1. Comente a cópia de `.data` no `startup_f0.s` e observe o LED piscar rápido. Faça o mesmo com a zeragem de `.bss`. (Dica: depois de apertar RESET a RAM ainda guarda os valores antigos, o que pode esconder o problema; desligue e religue o USB.)
2. No `startup_f4.s`, remova a habilitação da FPU e use uma variável `float` no `main`. O que acontece? No GDB, leia `p/x *(uint32_t*)0xE000ED28` e procure o bit 19 (NOCP, "sem coprocessador").
3. Escreva um `HardFault_Handler` que pisca o LED num padrão diferente e provoque um HardFault chamando um ponteiro de função nulo.
4. Encontre no RM0390 a posição da interrupção da USART2 do F446 e confira no `objdump` qual endereço está gravado nela.
5. Preencha a pilha com `0xDEADBEEF` no startup e, depois de o programa rodar um tempo, descubra no GDB quanto da pilha foi usado.
