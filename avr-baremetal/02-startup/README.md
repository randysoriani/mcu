# Módulo 02 — Código de inicialização (startup)

> **Objetivo deste módulo:** entender tudo o que acontece entre ligar a placa e a primeira linha do `main()`, e como o processador sabe qual função chamar quando ocorre uma interrupção. Os dois chips fazem isso de formas bem diferentes, e os dois startups do curso estão em assembly, comentados linha a linha.

| Arquivo | O que é |
|---|---|
| [`common/m328p/startup.S`](../common/m328p/startup.S) | startup do ATmega328P (assembly AVR) |
| [`common/uc3a/startup.S`](../common/uc3a/startup.S) | startup do AT32UC3A (assembly AVR32) |
| [`common/uc3a/intc.c`](../common/uc3a/intc.c) | despachante de interrupções do UC3 |
| [`src/main.c`](src/main.c) | teste: confere `.data`/`.bss` e substitui uma rotina de interrupção |

```sh
make MCU=m328p flash      # LED a 1 Hz = startup OK; 10 Hz = erro
```

---

## 0. Termos

| Termo | Significado |
|---|---|
| **Reset** | o "começar do zero": ligar a placa, apertar RESET, watchdog... |
| **PC** | registrador com o endereço da próxima instrução |
| **SP / pilha** | área da RAM para variáveis locais e endereços de retorno; sem ela nenhuma função C funciona |
| **Interrupção** | aviso do hardware que faz a CPU parar, executar uma função e voltar |
| **Vetor / tabela de vetores** | lista que diz qual código executar para cada interrupção |
| **ISR** (*Interrupt Service Routine*) | a função que trata uma interrupção |
| **`.data` / `.bss`** | variáveis globais com valor inicial / sem valor (módulo 03) |

---

## 1. Por que existe um startup

A linguagem C promete coisas que alguém precisa cumprir **antes** do `main()`:

| O C promete... | Mas após o reset... | O startup |
|---|---|---|
| `int x = 100;` começa valendo 100 | a RAM tem lixo | copia o 100 da Flash para a RAM |
| `int y;` começa valendo 0 | a RAM tem lixo | zera essa área |
| funções podem ser chamadas | não há pilha definida (AVR32) / o GCC espera `r1 = 0` (AVR8) | prepara SP e registradores |
| a ISR certa é chamada | a CPU precisa saber onde estão as ISRs | define a tabela de vetores |

---

## 2. ATmega328P

### 2.1 O que acontece no reset
1. O **PC vai para 0** (o começo da Flash) e a CPU executa a instrução que estiver lá.
2. Todos os registradores de E/S voltam ao valor padrão; as interrupções estão desligadas (bit I do SREG = 0).
3. A RAM tem lixo (depois de ligar a energia).

> Com o bootloader do Arduino, o fuse BOOTRST faz o reset ir para o **bootloader** (no fim da Flash) em vez do endereço 0. O bootloader espera um programa novo por meio segundo e, se nada chegar, salta para o endereço 0 — o nosso programa. Para o nosso código, é como se o reset tivesse ido direto para 0.

### 2.2 A tabela de vetores do AVR de 8 bits

**O que é:** no ATmega, a tabela de vetores fica no **início da Flash** e é uma lista de **instruções de salto**, uma para cada evento. A posição 0 é o reset; as seguintes, as interrupções de cada periférico.

**Para que serve:** quando uma interrupção acontece, o hardware salva o endereço de onde parou e **coloca no PC o endereço da posição correspondente da tabela**. A instrução que está lá (`jmp`) leva à ISR. É uma "central telefônica": cada evento tem um ramal fixo, e no ramal há um desvio para quem atende.

**Como é por dentro:** no ATmega328P cada posição ocupa 2 words (4 bytes), espaço exato para uma instrução `jmp` de 32 bits:

| Vetor | Endereço (words) | Evento |
|---|---|---|
| 0 | 0x0000 | **RESET** |
| 1 | 0x0002 | INT0 (pino PD2) |
| 2 | 0x0004 | INT1 (pino PD3) |
| 3–5 | 0x0006–0x000A | PCINT0/1/2 (mudança de pino, portas B/C/D) |
| 6 | 0x000C | Watchdog |
| 7–9 | 0x000E–0x0012 | Timer2: comparação A, B, estouro |
| 10–13 | 0x0014–0x001A | Timer1: captura, comparação A, B, estouro |
| 14–16 | 0x001C–0x0020 | Timer0: comparação A, B, estouro |
| 17 | 0x0022 | SPI |
| 18–20 | 0x0024–0x0028 | USART: recebeu, registrador vazio, transmissão completa |
| 21 | 0x002A | ADC |
| 22 | 0x002C | EEPROM pronta |
| 23 | 0x002E | comparador analógico |
| 24 | 0x0030 | TWI (I2C) |
| 25 | 0x0032 | SPM pronto |

A ordem também é a **prioridade**: se duas interrupções estiverem pendentes ao mesmo tempo, a de vetor menor é atendida primeiro.

No [`startup.S`](../common/m328p/startup.S):
```asm
    .section .vectors, "ax", @progbits
__vectors:
    jmp __init                  ;  0 RESET
    jmp __vector_1              ;  1 INT0
    ...
    jmp __vector_25             ; 25 SPM_READY
```
O linker script coloca a seção `.vectors` no endereço 0 (módulo 03).

### 2.3 Rotinas fracas e a macro `ISR`
```asm
    .weak __vector_14
    .set  __vector_14, __bad_interrupt
```
"Se ninguém definir `__vector_14`, ele vale `__bad_interrupt`" (que reinicia o programa). Para escrever uma ISR, o código C define uma função com esse nome. A macro do curso ([`m328p.h`](../common/m328p/m328p.h)) faz isso:
```c
#define ISR(v) ... void __vector_##v(void) __attribute__((signal, used, externally_visible))
ISR(TIMER0_COMPA_vect) { ... }      /* TIMER0_COMPA_vect = 14 -> define __vector_14 */
```
O atributo `signal` muda o código gerado: a função **salva na pilha todos os registradores que usar** (inclusive o SREG) e termina com **`RETI`** (retorno de interrupção, que religa o bit I) em vez de `RET`.

### 2.4 O código de inicialização, linha a linha
```asm
__init:
    clr     r1              ; r1 = 0: o GCC conta com isso em todo o código
    out     0x3F, r1        ; SREG = 0 (interrupções desligadas)
    ldi     r28, lo8(__stack)
    ldi     r29, hi8(__stack)
    out     0x3E, r29       ; SPH
    out     0x3D, r28       ; SPL: pilha começa no último byte da RAM (0x08FF)
```
O SP do AVR aponta para a **primeira posição livre** e é decrementado **depois** de cada `push`; por isso começa exatamente no último byte.

Cópia do `.data` (valores iniciais na Flash → RAM):
```asm
    ldi     r26, lo8(__data_start)      ; X = destino na RAM
    ldi     r27, hi8(__data_start)
    ldi     r30, lo8(__data_load_start) ; Z = origem na Flash
    ldi     r31, hi8(__data_load_start)
    ldi     r17, hi8(__data_end)
    rjmp    2f
1:  lpm     r0, Z+                      ; r0 = Flash[Z]; Z++   <- LPM: lê a memória de PROGRAMA
    st      X+, r0                      ; RAM[X] = r0; X++
2:  cpi     r26, lo8(__data_end)        ; X chegou ao fim? (16 bits: CPI no byte baixo,
    cpc     r27, r17                    ;  CPC no alto, com o "vai-um" do anterior)
    brne    1b
```
Aqui aparece a arquitetura Harvard: para ler a Flash é obrigatório usar `LPM` com o ponteiro Z.

Zeragem do `.bss`, chamada do `main` e laço final:
```asm
3:  st      X+, r1                      ; r1 vale 0
    ...
    call    main
    cli
5:  rjmp    5b
```

---

## 3. AT32UC3A0512

### 3.1 O que acontece no reset
1. O **PC vai para `0x8000_0000`** (início da Flash) e a CPU executa o que estiver lá. Não há tabela no endereço do reset nem "valor inicial do SP".
2. O clock é o **oscilador RC interno de ~115 kHz** (lento).
3. Exceções e interrupções estão mascaradas no registrador de status.

Na EVK1100, os primeiros 8 KB da Flash são o **bootloader USB**. Ele roda no reset e, se não for chamado a gravar, salta para `0x8000_2000`. Por isso:
- o programa do curso começa em `0x8000_2000`;
- em `0x8000_0000` há um **trampolim** de duas words, que salta para o programa — usado só se você gravar por JTAG e apagar o bootloader:
```asm
_trampoline:
    lddpc   pc, 1f          ; PC = valor guardado logo abaixo (endereço de _start)
1:  .word   _start
```

### 3.2 Inicialização
```asm
_start:
    lddpc   sp, .Lestack    ; SP = fim da SRAM (0x0001_0000)
    lddpc   r0, .Levba
    mtsr    4, r0           ; registrador de sistema EVBA = endereço da tabela de exceções
    csrf    21              ; libera as exceções (bit EM do status)
```
Depois vêm a cópia do `.data` (`ld.w r3, r0++` / `st.w r1++, r3`: uma word de 32 bits por vez) e a zeragem do `.bss`, como no ATmega, e o `rcall main`.

### 3.3 A tabela de exceções (EVBA)

**O que é:** no AVR32 a tabela fica onde o registrador de sistema **EVBA** (*Exception Vector Base Address*) apontar. Para cada **exceção** (erro de barramento, instrução ilegal, acesso desalinhado...) há uma posição fixa a partir de EVBA, e ali fica o **código** a executar.

| Offset | Exceção |
|---|---|
| 0x000 | irrecuperável |
| 0x008 / 0x00C | erro de barramento (dado / instrução) |
| 0x010 | NMI |
| 0x01C | breakpoint |
| 0x020 | instrução ilegal |
| 0x034 / 0x038 | endereço de dado desalinhado (leitura / escrita) |
| 0x100 | chamada de supervisor (`scall`) |

No curso cada uma é um laço infinito (`rjmp` para ela mesma): se o programa travar, o depurador mostra exatamente qual exceção ocorreu.

### 3.4 E as interrupções? O INTC

As interrupções de periféricos passam pelo **INTC** (controlador de interrupções), que funciona assim:
1. Cada periférico pertence a um **grupo** (USART0 = 6, GPIO = 2, TC = 15...).
2. Cada grupo tem um registrador `IPR` com o **nível de prioridade** (0 a 3) e um **autovetor**: a distância, a partir de EVBA, do código a executar.
3. Quando o grupo pede interrupção, a CPU salva `R8`–`R12`, `LR`, `PC` e `SR` e salta para `EVBA + autovetor`.

O curso aponta o autovetor de todos os grupos de um mesmo nível para uma única entrada (`_int0` a `_int3`), que pergunta ao INTC qual grupo foi e chama a função C registrada:
```asm
_int0:  mov     r12, 0               ; nível 0
_int_common:
        rcall   intc_get_handler     ; C: lê INTC->ICR[3 - nível] e devolve a função
        cp.w    r12, 0
        movne   pc, r12              ; salta para a função (ela termina com "rete")
        rete
```
Em C ([`intc.c`](../common/uc3a/intc.c)):
```c
intc_register(IRQ_GRP_USART0, minha_funcao, 1);     /* grupo, função, nível */
ISR_ATTR static void minha_funcao(void) { ... }     /* __attribute__((interrupt)): termina com rete */
```
Vantagem desse esquema: a tabela não precisa listar cada periférico, e os níveis de prioridade permitem que uma interrupção mais urgente interrompa outra.

---

## 4. Comparando os três mundos

| | ATmega328P | AT32UC3A | ARM Cortex-M (cursos STM32/TM4C) |
|---|---|---|---|
| Reset começa em | endereço 0 | `0x8000_0000` | endereço lido da posição 1 da tabela |
| SP inicial | o startup define | o startup define | lido da posição 0 da tabela |
| Tabela contém | instruções `jmp` | código (exceções) + autovetores no INTC | endereços |
| Onde fica a tabela | fixo no início da Flash | onde EVBA apontar | endereço 0 (ou VTOR) |
| Prioridade | ordem fixa da tabela | 4 níveis programáveis | programável por IRQ |
| ISR em C | `__attribute__((signal))` | `__attribute__((interrupt))` | função comum |

---

## Exercícios
1. Remova a cópia do `.data` no `startup.S` do ATmega e observe o LED piscar rápido.
2. Habilite uma interrupção sem ISR (por exemplo `TIMSK2 = 1` com o Timer2 rodando) e veja o programa reiniciar sem parar.
3. Desmonte (`make disasm`) o ATmega e encontre o `push`/`pop` que o atributo `signal` gera na ISR.
4. No UC3, provoque uma leitura desalinhada (`*(volatile uint32_t*)0x2`) e descubra em qual entrada da tabela o programa para.
