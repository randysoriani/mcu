# Módulo 02 — Código de inicialização (startup)

Nenhum código C roda "do nada". Entre o reset e o `main()` alguém precisa: definir a pilha, preparar as variáveis globais e montar a tabela de interrupções. Esse alguém é o **startup**.

Arquivos:

| Arquivo | Conteúdo |
|---|---|
| [`f0/startup_f0.s`](f0/startup_f0.s) | startup em **assembly** para o Cortex-M0 (ARMv6-M) |
| [`f4/startup_f4.s`](f4/startup_f4.s) | startup em **assembly** para o Cortex-M4F (ARMv7E-M) |
| [`../common/f0/startup.c`](../common/f0/startup.c) | o mesmo, em **C** — usado por todos os outros módulos |
| [`../common/f4/startup.c`](../common/f4/startup.c) | idem para o F4 |
| [`src/main.c`](src/main.c) | programa de teste: verifica `.data`/`.bss` e sobrescreve o `SysTick_Handler` |

```sh
make MCU=f0 flash     # LED a 1 Hz = startup OK; 10 Hz = .data/.bss errados
```

---

## 1. A sequência de reset do Cortex-M

1. O hardware lê a word em `0x0000_0000` e carrega no **MSP**.
2. Lê a word em `0x0000_0004` e carrega no **PC** (o bit 0 tem que ser 1 → Thumb).
3. Começa a executar em *Thread mode*, privilegiado, com MSP, interrupções habilitadas no núcleo (PRIMASK = 0) mas **todas desabilitadas no NVIC**.
4. Clock = oscilador interno (HSI: 8 MHz no F0, 16 MHz no F4), todos os periféricos desligados, FPU desligada.

Não existe "BIOS". Tudo que acontece depois é código seu.

---

## 2. O que o startup precisa fazer

```
Reset_Handler:
  [F4] habilita a FPU (CPACR)
  copia .data  (valores iniciais)  FLASH -> RAM
  zera   .bss
  [opcional] chama construtores (__init_array)
  [opcional] SystemInit(): clock, VTOR...
  chama main()
  laço infinito se main retornar
```

### Por que copiar `.data`?

```c
int contador = 100;   /* .data */
int zerado;           /* .bss  */
```

`contador` precisa estar na **RAM** (é modificável), mas a RAM perde o conteúdo sem energia. O valor `100` então é gravado na **Flash**, e o startup o copia para a RAM antes do `main`. O linker script fornece três símbolos (módulo 03):

- `_sidata` — onde está a cópia na Flash (LMA)
- `_sdata`, `_edata` — onde `.data` vive na RAM (VMA)

### Por que zerar `.bss`?

O padrão C garante que globais/estáticas sem inicializador valem 0. Em vez de gravar milhares de zeros na Flash, o linker só reserva o espaço (`.bss`) e o startup o zera (`_sbss` .. `_ebss`). Após um reset a RAM contém lixo — sem esse passo, `zerado` teria um valor aleatório.

### Por que habilitar a FPU (F4)?

Compilamos com `-mfloat-abi=hard`: o compilador usa registradores `s0..s31` e instruções VFP. Após o reset, o coprocessador está **sem acesso** e a primeira instrução de ponto flutuante gera **UsageFault (NOCP)** → HardFault. O registrador `CPACR` (`0xE000_ED88`) libera os coprocessadores CP10 e CP11 (bits 23:20 = `1111`, "full access"). Em seguida `DSB` + `ISB` garantem que a escrita terminou e que o pipeline não tem instruções buscadas antes da mudança.

> Cuidado: o compilador pode usar registradores VFP até para cópias de memória. Por isso a FPU é habilitada como **primeira** coisa do Reset_Handler.

---

## 3. A tabela de vetores

É um array de endereços de 32 bits no início da imagem. Posição = número da exceção:

| Offset | Nº exc. | M0 (F0) | M4 (F4) |
|---|---|---|---|
| 0x00 | — | SP inicial | SP inicial |
| 0x04 | 1 | Reset | Reset |
| 0x08 | 2 | NMI | NMI |
| 0x0C | 3 | HardFault | HardFault |
| 0x10 | 4 | reservado | MemManage |
| 0x14 | 5 | reservado | BusFault |
| 0x18 | 6 | reservado | UsageFault |
| 0x1C–0x28 | 7–10 | reservado | reservado |
| 0x2C | 11 | SVCall | SVCall |
| 0x30 | 12 | reservado | DebugMonitor |
| 0x34 | 13 | reservado | reservado |
| 0x38 | 14 | PendSV | PendSV |
| 0x3C | 15 | SysTick | SysTick |
| 0x40 | 16 | IRQ0 (WWDG) | IRQ0 (WWDG) |
| ... | 16+n | IRQn | IRQn |

- F072: 32 IRQs → tabela de 48 words (192 bytes).
- F446: 97 IRQs → 113 words (452 bytes). A lista vem da tabela "Vector table" do capítulo *Interrupts and events* do RM (RM0091 / RM0390).

A posição de cada IRQ é **fixa no silício**. Um erro de uma linha na tabela faz a interrupção de um periférico chamar o handler de outro — um bug muito difícil de achar. Por isso as tabelas do curso têm o número da IRQ comentado em cada linha.

A seção `.isr_vector` é colocada pelo linker script em `0x0800_0000` com `KEEP()`, para o `--gc-sections` não descartá-la (ninguém a "chama").

---

## 4. Handlers fracos (weak) e `Default_Handler`

Todos os handlers são definidos como **aliases fracos** de `Default_Handler`:

```c
void Default_Handler(void) { for (;;) { } }
void TIM2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
```

```asm
    .weak      TIM2_IRQHandler
    .thumb_set TIM2_IRQHandler, Default_Handler
```

Se o seu código definir `void TIM2_IRQHandler(void) { ... }` (símbolo **forte**), o linker usa a sua versão. Se não, a entrada da tabela aponta para o laço infinito. Vantagens: a tabela pode ser completa sem você implementar 97 funções, e uma interrupção habilitada por engano trava num lugar conhecido (no GDB: `p/x $xpsr & 0x1ff` dá o número da exceção).

**Armadilha clássica**: errar o nome (`USART2_IRQHandle`, `TIM6_IRQHandler` em vez de `TIM6_DAC_IRQHandler`). Compila sem erro, mas a interrupção cai no `Default_Handler`. Confira com:
```sh
arm-none-eabi-nm build/f4/x.elf | grep IRQHandler
```

---

## 5. Startup em assembly, linha a linha (F0)

```asm
    .syntax unified        @ sintaxe UAL
    .cpu    cortex-m0
    .thumb

    .section .text.Reset_Handler, "ax", %progbits   @ "ax" = alocável + executável
    .type    Reset_Handler, %function               @ marca como função Thumb -> bit0 = 1
Reset_Handler:
    ldr   r0, =_estack     @ pseudo-instrução: carrega constante de um literal pool
    mov   sp, r0           @ no ARMv6-M não existe "ldr sp, =..."
```

Cópia de `.data` no M0 (sem pós-incremento, usando offset em r3):

```asm
    ldr   r0, =_sdata
    ldr   r1, =_edata
    ldr   r2, =_sidata
    movs  r3, #0
    b     2f               @ "2f" = rótulo local 2, para frente
1:  ldr   r4, [r2, r3]
    str   r4, [r0, r3]
    adds  r3, r3, #4
2:  adds  r4, r0, r3
    cmp   r4, r1
    bcc   1b               @ branch if carry clear (unsigned <), para trás
```

No M4 (Thumb-2) a mesma cópia fica mais compacta com **pós-incremento** e **bloco IT**:

```asm
1:  cmp   r0, r1
    ittt  lo               @ If-Then-Then-Then: as 3 próximas só executam se "lo"
    ldrlo r3, [r2], #4     @ r3 = *r2; r2 += 4
    strlo r3, [r0], #4     @ *r0 = r3; r0 += 4
    blo   1b
```

A tabela de vetores em assembly é só uma sequência de `.word`:

```asm
    .section .isr_vector, "a", %progbits     @ "a" = só alocável (dados na Flash)
vector_table:
    .word _estack
    .word Reset_Handler
    .word NMI_Handler
    ...
```

---

## 6. Startup em C

Ver `common/f4/startup.c`. Pontos importantes:

```c
extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;
```
São símbolos do linker. **Não são variáveis**: só o *endereço* deles importa (`&_sdata`). Declará-los como `uint32_t` facilita a aritmética de ponteiros na cópia.

```c
__attribute__((section(".isr_vector"), used))
const vector_t vector_table[16 + 97] = {
    (vector_t)&_estack,
    Reset_Handler,
    ...
```
- `section` coloca o array na seção certa; `used` impede o compilador de descartá-lo; `const` o mantém na Flash.
- A entrada 0 é um endereço de dados (SP) convertido para ponteiro de função — só para caber no array.

```c
__attribute__((optimize("no-tree-loop-distribute-patterns")))
void Reset_Handler(void)
```
Com `-O2`, o GCC reconhece o laço de cópia/zeragem e o substitui por `memcpy()`/`memset()` da libc. Funcionaria, mas o atributo mantém o startup independente de bibliotecas (vale o teste: remova-o e procure `bl memcpy` no `make disasm`).

### C ou assembly?

Funcionalmente iguais. O C é mais legível e portátil; o assembly deixa explícito o que acontece e é útil quando nem a pilha é confiável (ex.: bootloaders que precisam reconfigurar o SP). A ST fornece startups em assembly nos pacotes CMSIS; o curso usa o em C a partir daqui.

---

## 7. Relocando a tabela de vetores

- **M4 (F4)**: escreva o novo endereço (alinhado a 512 bytes para 97 IRQs — potência de 2 ≥ tamanho da tabela) em `SCB->VTOR`. Usado por bootloaders: a aplicação começa em `0x0800_8000` e faz `SCB->VTOR = 0x08008000;`.
- **M0 (F0)**: não há VTOR. Copia-se a tabela para o início da SRAM e seleciona-se `SYSCFG->CFGR1.MEM_MODE = 11` (SRAM mapeada em 0x0). O linker precisa reservar os primeiros 192 bytes da RAM.

---

## 8. Depurando o startup

```sh
make MCU=f4 debug
(gdb) monitor reset halt
(gdb) p/x $sp                # = _estack (0x20020000)
(gdb) p/x $pc                # = Reset_Handler
(gdb) si                     # passo a passo em assembly
(gdb) x/4xw &_sdata          # RAM antes da cópia: lixo
(gdb) b main
(gdb) c
(gdb) p/x valor_data         # {0x11111111, 0x22222222, ...}
```

Testado em QEMU (`netduinoplus2`): após o startup em assembly, `valor_data` contém os valores iniciais, `buffer_bss` é zero e `periodo_ms == 500` (teste OK).

---

## Exercícios

1. Comente a cópia de `.data` no `startup_f0.s` e observe o LED piscar rápido. Faça o mesmo com a zeragem de `.bss` (dica: depois de um power-cycle a RAM tem lixo; após um reset por botão, os valores antigos podem sobreviver e mascarar o bug!).
2. No `startup_f4.s`, remova a habilitação da FPU e use um `float` no `main`. Qual exceção ocorre? Leia `SCB->CFSR` no GDB (`p/x *(uint32_t*)0xE000ED28`) e procure o bit NOCP (bit 19).
3. Escreva um `HardFault_Handler` que pisca o LED em um padrão SOS, e provoque um HardFault chamando um ponteiro de função nulo.
4. Acrescente ao startup em C uma rotina que preencha a pilha com `0xDEADBEEF` antes do `main` e, depois, uma função que meça quanto da pilha foi usado ("stack painting").
