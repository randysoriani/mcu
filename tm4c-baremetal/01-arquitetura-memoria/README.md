# Módulo 01 — Arquitetura Cortex-M4F e organização da memória do TM4C

> **Objetivo deste módulo:** montar o "mapa mental" do microcontrolador: o que tem dentro do chip, como o processador enxerga tudo como endereços de memória e como o código em C conversa com o hardware. Sem isso, os próximos módulos parecem uma sopa de números hexadecimais.

Código: [`src/main.c`](src/main.c) (o mesmo para os dois chips). Ele imprime na serial a identificação do processador e do chip, o tamanho da Flash e da RAM e **onde cada tipo de variável foi parar na memória**, e pisca o LED usando uma técnica chamada *bit-banding*.

```sh
make MCU=123 flash && picocom -b 115200 /dev/ttyACM0
```
(O clock, a serial e o `kprintf` usados aqui são explicados nos módulos 04, 06 e 07. Por enquanto, use-os como caixa preta.)

---

## 0. Revisão rápida: bits, bytes e hexadecimal

Tudo num computador é número binário (0 e 1). Você vai ler e escrever esses números o tempo todo.

| Termo | Tamanho | Faixa (sem sinal) | Tipo em C |
|---|---|---|---|
| **bit** | 1 dígito binário | 0 ou 1 | — |
| **byte** | 8 bits | 0 a 255 | `uint8_t` |
| **halfword** | 16 bits | 0 a 65 535 | `uint16_t` |
| **word** | 32 bits | 0 a 4 294 967 295 | `uint32_t` |

Os bits de uma word são numerados da direita para a esquerda, **de 0 a 31**. "Bit 3" é o quarto a partir da direita.

**Hexadecimal** (base 16, prefixo `0x`) é uma forma compacta de escrever binário: **cada dígito hexa corresponde a exatamente 4 bits**.

| Hexa | Binário | Decimal |
|---|---|---|
| `0` | `0000` | 0 |
| `1` | `0001` | 1 |
| `8` | `1000` | 8 |
| `A` | `1010` | 10 |
| `E` | `1110` | 14 |
| `F` | `1111` | 15 |

Exemplo: `0x08` = `0000 1000` = bit 3 ligado = 8 em decimal. É por isso que `0x08` aparece quando mexemos no pino 3 (o LED verde da TM4C123). Com a prática você "lê" binário olhando o hexa.

> Dica: o Python converte entre bases: `hex(8)`, `bin(0x0E)`, `int("1010", 2)`.

---

## 1. O que tem dentro de um microcontrolador

```
┌──────────────────────────── chip TM4C ─────────────────────────────┐
│                                                                     │
│   ┌──────────────┐        barramentos (as "estradas" internas)     │
│   │ Núcleo ARM   │◄═════════════════╦══════════════╦════════════╗  │
│   │ Cortex-M4F   │                  ║              ║            ║  │
│   │ (processador)│            ┌─────╨────┐   ┌─────╨────┐ ┌─────╨───────┐
│   └──────────────┘            │  Flash   │   │   RAM    │ │ Periféricos │
│                               │(programa)│   │(variáveis│ │ GPIO, UART, │
│                               └──────────┘   └──────────┘ │ timers, ADC │
│                                                           └──────┬──────┘
└──────────────────────────────────────────────────────────────────┼───────┘
                                                                   │
                                                       pinos do chip (LED, botão, sensores...)
```

| Parte | Função | TM4C123GH6PM | TM4C1294NCPDT |
|---|---|---|---|
| **Núcleo** | executa as instruções | Cortex-M4F, até 80 MHz | Cortex-M4F, até 120 MHz |
| **Flash** | guarda o programa (não apaga sem energia) | 256 KB | 1 MB |
| **RAM** | guarda as variáveis durante a execução | 32 KB | 256 KB |
| **EEPROM** | guarda configurações (não apaga, regravável muitas vezes) | 2 KB | 6 KB |
| **Periféricos** | circuitos que conversam com o mundo externo: pinos (GPIO), serial (UART, SPI, I2C), timers, conversor analógico-digital (ADC), CAN, USB... | dezenas | dezenas, + Ethernet |
| **Barramentos** | ligam o núcleo à memória e aos periféricos | APB e AHB | AHB |

A grande diferença para um PC: não há sistema operacional nem drivers. **O seu programa controla diretamente cada periférico**, escrevendo números em registradores. Este curso inteiro é sobre isso.

---

## 2. O núcleo Cortex-M4F

A ARM projeta uma família de núcleos para microcontroladores, os **Cortex-M**. Os fabricantes (TI, ST, NXP...) compram a licença e constroem chips em volta deles. Por isso o que você aprender sobre o núcleo vale para chips de qualquer fabricante.

Os dois TM4C usam o **Cortex-M4F** (arquitetura ARMv7E-M):

| Característica | O que significa na prática |
|---|---|
| Conjunto de instruções Thumb-2 | instruções de 16 e 32 bits, código compacto |
| Divisão por hardware | `a / b` leva de 2 a 12 ciclos |
| Instruções DSP | contas de processamento de sinal (áudio, filtros) rápidas |
| **FPU** (o "F" de M4F) | contas com `float` (32 bits) em hardware |
| MPU de 8 regiões | proteção de memória contra acessos indevidos |
| Bit-banding | acessar um bit isolado como se fosse uma variável (seção 8) |
| NVIC com 8 níveis de prioridade | interrupções mais urgentes interrompem as menos urgentes (a TI implementa 3 bits de prioridade) |
| Registradores de diagnóstico de falha | quando o programa trava, dá para descobrir por quê (módulo 24) |

### Os registradores do processador

Dentro do núcleo há 16 registradores principais de 32 bits. Pense neles como as "mãos" do processador: ele só faz contas com o que está nas mãos, buscando e guardando valores na memória quando precisa.

| Registrador | Apelido | Para que serve |
|---|---|---|
| `r0`–`r3` | | argumentos de funções e valor de retorno; rascunho |
| `r4`–`r11` | | variáveis locais (preservadas entre chamadas de função) |
| `r12` | IP | rascunho |
| `r13` | **SP** | *stack pointer*: topo da pilha |
| `r14` | **LR** | *link register*: endereço de retorno da função atual |
| `r15` | **PC** | *program counter*: endereço da próxima instrução |
| `xPSR` | | flags do último resultado (zero? negativo? estourou?) e número da interrupção em execução |
| `PRIMASK` | | se 1, bloqueia as interrupções (`__disable_irq()`) |
| `BASEPRI`, `FAULTMASK` | | bloqueio seletivo de interrupções |
| `CONTROL` | | modo privilegiado, qual pilha usar |
| `s0`–`s31`, `FPSCR` | | registradores da FPU |

**O bit 0 dos endereços de função.** Os Cortex-M só entendem instruções *Thumb*, e a ARM usa o bit 0 de um endereço de função como marca de "isto é Thumb". Por isso funções aparecem em endereços ímpares (`0x000002CD`): a instrução está em `0x000002CC`. O compilador cuida disso.

### Interrupções
Quando uma interrupção chega, o próprio hardware **guarda automaticamente** na pilha os registradores `r0`–`r3`, `r12`, `LR`, `PC` e `xPSR` (e os da FPU, se estiverem em uso), executa o handler e depois **restaura** tudo. Por isso, no Cortex-M, um handler de interrupção é uma **função C comum**.

---

## 3. O mapa de memória: tudo é endereço

Esta é a ideia mais importante do módulo.

O processador tem um espaço de endereços de 32 bits: de `0x0000_0000` a `0xFFFF_FFFF`, ou seja, 4 GB de "endereços possíveis". O chip não tem 4 GB de memória: a maior parte desses endereços está vazia. Mas **cada coisa que existe** (Flash, RAM e cada registrador de cada periférico) **ocupa um endereço fixo**.

Analogia: uma rua muito longa, com 4 bilhões de números de casa, a maioria lotes vazios. A partir do número `0x0000_0000` fica o "depósito" (a Flash); a partir de `0x2000_0000`, o "escritório" (a RAM); a partir de `0x4000_0000`, as "salas de controle" dos periféricos. Para acender um LED, o programa vai até a casa da porta GPIO e escreve um número lá dentro.

Isso se chama **memory-mapped I/O** (entrada/saída mapeada em memória): ler ou escrever num periférico é **exatamente igual** a ler ou escrever numa variável — só muda o endereço.

| Região | TM4C123GH6PM | TM4C1294NCPDT |
|---|---|---|
| Flash (o programa) | `0x0000_0000`–`0x0003_FFFF` (256 KB) | `0x0000_0000`–`0x000F_FFFF` (1 MB) |
| ROM da TI (bootloader e rotinas prontas gravadas de fábrica) | `0x0100_0000` | `0x0100_0000` |
| SRAM (variáveis) | `0x2000_0000`–`0x2000_7FFF` (32 KB) | `0x2000_0000`–`0x2003_FFFF` (256 KB) |
| Espelho bit-band da SRAM | `0x2200_0000` | `0x2200_0000` |
| Periféricos do chip | `0x4000_0000`–`0x400F_FFFF` | idem, + `0x4400_0000` (criptografia) |
| Espelho bit-band dos periféricos | `0x4200_0000` | `0x4200_0000` |
| Periféricos do núcleo (SysTick, NVIC, SCB) | `0xE000_E000` | `0xE000_E000` |

**Diferença em relação a outros fabricantes**: no TM4C a Flash começa direto no endereço `0x0000_0000`. Não há pino de modo de boot nem "espelho": o início do seu programa é exatamente o que o núcleo lê ao ligar (módulo 02).

### Onde mora cada periférico (iguais nos dois chips, salvo nota)

| Endereço | Periférico |
|---|---|
| `0x4000_0000` / `0x4000_1000` | watchdogs WDT0 / WDT1 |
| `0x4000_4000`–`0x4000_7000`, `0x4002_4000`–`0x4002_5000` | **portas GPIO A–F pelo barramento APB** (só 123) |
| `0x4000_8000`–`0x4000_B000` | SSI0–3 (o "SPI" da TI) |
| `0x4000_C000`–`0x4001_3000` | UART0–7 (serial) |
| `0x4002_0000`–`0x4002_3000` | I2C0–3 |
| `0x4002_8000` / `0x4002_9000` | PWM0 / PWM1 (PWM1 só no 123) |
| `0x4002_C000` / `0x4002_D000` | QEI0 / QEI1, leitor de encoder (QEI1 só no 123) |
| `0x4003_0000`–`0x4003_5000` | TIMER0–5 |
| `0x4003_6000`... | WTIMER0–5, timers de 64 bits (só 123) |
| `0x4003_8000` / `0x4003_9000` | ADC0 / ADC1 |
| `0x4004_0000` / `0x4004_1000` | CAN0 / CAN1 |
| `0x4005_8000`–`0x4006_6000` | **portas GPIO pelo barramento AHB** (123: A–F, opcional; 129: A–Q, a única forma) |
| `0x400A_F000` | EEPROM |
| `0x400F_C000` | módulo de hibernação (relógio de tempo real) |
| `0x400F_D000` | controle da Flash |
| `0x400F_E000` | **System Control**: clock, reset, energia |
| `0x400F_F000` | uDMA (cópia de dados sem a CPU) |

> De onde vêm esses números? Da tabela *Memory Map* do datasheet de cada chip. Os headers do curso (`common/include/tm4c_common.h`, `common/123/tm4c123.h`, `common/129/tm4c129.h`) copiam essas tabelas. Abra-os e procure `GPIOF`.

### APB × AHB para as portas GPIO
Os periféricos ficam pendurados em **barramentos**, as "estradas" internas. O AHB é rápido (um acesso por ciclo); o APB é mais antigo e mais lento. O TM4C123 dá duas "portas de entrada" para cada porta GPIO A–F: pelo APB (o padrão após o reset) e pelo AHB (escolhida porta a porta no registrador `SYSCTL_GPIOHBCTL`). O curso usa o APB no 123. O TM4C129 só tem a entrada pelo AHB. Por isso `GPIOF` tem endereços diferentes nos dois headers — mas o código que usa `GPIOF` é o mesmo.

---

## 4. System Control: o liga/desliga de cada periférico

**Todo periférico começa desligado, sem clock.** Antes de usar um periférico, é preciso "ligá-lo" no bloco **System Control** (`0x400F_E000`). A TI organiza isso de forma bem regular: para cada **tipo** de periférico há um registrador, com **1 bit por unidade** daquele tipo.

| Faixa de offset | Família de registradores | Função |
|---|---|---|
| `0x5xx` | `SRxxx` | reset de um periférico |
| `0x6xx` | **`RCGCxxx`** | liga o clock do periférico (*Run mode Clock Gating Control*) |
| `0x7xx` | `SCGCxxx` | clock durante o modo Sleep |
| `0x8xx` | `DCGCxxx` | clock durante o modo Deep-Sleep |
| `0xAxx` | **`PRxxx`** | o periférico está pronto (só leitura) |

O final do offset (`xx`) indica o tipo: `00` watchdog, `04` timer, `08` GPIO, `0C` DMA, `14` hibernação, `18` UART, `1C` SSI, `20` I2C, `34` CAN, `38` ADC, `40` PWM, `44` QEI, `58` EEPROM, `5C` wide timer.

Exemplo: para ligar a **UART0**, o registrador é `SYSCTL_RCGCUART` (offset `0x618`), bit 0 (UART**0**). Depois, espera-se `SYSCTL_PRUART` (offset `0xA18`), bit 0, ficar em 1:

```c
static inline void periph_enable(volatile uint32_t *rcgc, volatile uint32_t *pr, uint32_t n)
{
    *rcgc |= (1U << n);              /* liga o clock da unidade n            */
    while (!(*pr & (1U << n))) { }   /* espera ela avisar que está pronta    */
}
periph_enable(&SYSCTL_RCGCUART, &SYSCTL_PRUART, 0);   /* UART0 */
```

> No TM4C, mexer num periférico **sem clock gera um erro do processador** (BusFault, que vira HardFault): o programa trava. Em outros chips (como o STM32) a escrita é só ignorada. É o erro mais comum de quem começa.

---

## 5. Conversando com os registradores em C

Um registrador de periférico é só uma word num endereço conhecido. Por exemplo, na TM4C123 o registrador `DIR` da porta F (que diz quais pinos são saída) fica em `0x4002_5400`.

Há três jeitos de escrever "ligue o bit 3 desse endereço", do mais cru ao usado no curso:

```c
/* 1) Ponteiro "na mão" */
*(volatile uint32_t *)0x40025400 |= (1U << 3);

/* 2) Um apelido (macro) para o registrador */
#define GPIOF_DIR (*(volatile uint32_t *)0x40025400)
GPIOF_DIR |= (1U << 3);

/* 3) Uma struct que descreve o bloco inteiro do periférico (estilo usado no curso) */
typedef struct {
    volatile uint32_t DATA_BITS[255];   /* offset 0x000 ... */
    volatile uint32_t DATA;             /* 0x3FC */
    volatile uint32_t DIR;              /* 0x400 */
    /* ... */
} GPIO_TypeDef;
#define GPIOF ((GPIO_TypeDef *)0x40025000)
GPIOF->DIR |= (1U << 3);
```

Lendo a forma 1 de dentro para fora:
- `0x40025400` é só um número;
- `(volatile uint32_t *)` diz ao compilador "trate esse número como o endereço de uma word";
- o `*` da frente acessa o conteúdo desse endereço;
- `|= (1U << 3)` liga o bit 3.

A forma 3 funciona porque cada campo `uint32_t` ocupa 4 bytes e o compilador os coloca em sequência: o campo `DIR` fica exatamente no *offset* 0x400 do manual. Um único endereço-base por periférico, a mesma struct para GPIOA, GPIOB..., e o depurador entende (`p/x *GPIOF`).

### Por que `volatile`?

O compilador é esperto: se você escreve duas vezes seguidas na mesma variável, ele elimina a primeira escrita; se lê várias vezes a mesma variável sem mudá-la, lê uma vez só. Para registradores de hardware isso é um desastre, porque o hardware muda os valores "por fora" do programa:

```c
while (UART0->FR & (1 << 5)) { }    /* espera o hardware liberar espaço na serial */
```
Sem `volatile`, o compilador pode ler o registrador uma única vez e transformar isso num laço infinito.

`volatile` significa: **"cada leitura e escrita que eu escrevi deve acontecer de verdade, na ordem em que escrevi"**. Use em registradores de hardware (os headers do curso já declaram todos assim) e em variáveis compartilhadas entre uma interrupção e o `main`.

### Operações com bits

Quase tudo em embarcados é ligar e desligar bits de um registrador sem mexer nos outros. As quatro operações básicas, com 8 bits para ficar visível:

| Operação | Código | Exemplo (reg = `1010 0001`, n = 2) | Resultado |
|---|---|---|---|
| **ligar** o bit n | `reg \|= (1U << n);` | `1010 0001` OU `0000 0100` | `1010 0101` |
| **desligar** o bit n | `reg &= ~(1U << n);` | `1010 0101` E `1111 1011` | `1010 0001` |
| **inverter** o bit n | `reg ^= (1U << n);` | `1010 0001` OU-exclusivo `0000 0100` | `1010 0101` |
| **testar** o bit n | `if (reg & (1U << n))` | `1010 0101` E `0000 0100` = `0000 0100` | diferente de 0 → ligado |

- `1U << n` cria um número com só o bit n ligado (a "máscara"). `1U << 3` = `0x08`.
- `~` inverte todos os bits da máscara.
- Use sempre `1U` (sem sinal): `1 << 31` com `int` é comportamento indefinido em C.

Para escrever um **campo de vários bits** (por exemplo, os 4 bits que o pino 3 ocupa no registrador `PCTL`, bits 15:12):
```c
reg = (reg & ~(0xFU << 12))   /* 1. apaga os 4 bits do campo */
    | (V << 12);              /* 2. escreve o valor novo     */
```

### Ler-modificar-escrever e interrupções

`reg |= x` parece uma operação, mas são **três** instruções: ler, fazer o OU, escrever de volta. Se uma interrupção alterar o mesmo registrador no meio disso, a alteração dela se perde. Formas de evitar:
- no GPIO do TM4C, usar o **DATA mascarado** (seção 7), que muda só os pinos escolhidos sem ler antes;
- usar bit-banding (seção 8);
- desligar as interrupções em volta do trecho crítico (`__disable_irq()` ... `__enable_irq()`).

---

## 6. Identificando o chip

O programa deste módulo lê registradores que dizem quem é o chip:

| Registrador | Endereço | O que contém |
|---|---|---|
| `SCB->CPUID` | `0xE000_ED00` | qual núcleo ARM: `0x410FC241` = Cortex-M4 revisão r0p1 |
| `SYSCTL_DID0` | `0x400F_E000` | classe (bits 23:16): `0x05` = TM4C123, `0x0A` = TM4C129; revisão do silício (A0, A1, B1...) |
| `SYSCTL_DID1` | `0x400F_E004` | número da peça (bits 23:16): `0xA1` = TM4C123GH6PM, `0x1F` = TM4C1294NCPDT; nº de pinos, faixa de temperatura, encapsulamento |
| `FLASHPP` | `0x400F_DFC0` | tamanho da Flash = (valor dos bits 15:0 + 1) × 2 KB |
| `SSIZE` | `0x400F_DFC4` | tamanho da RAM = (valor + 1) × 256 bytes |

A revisão do silício importa: a lista de defeitos conhecidos do chip (*errata*) é organizada por revisão.

O programa também imprime onde cada variável foi parar. Você verá a constante na Flash (`0x0000...`), as variáveis globais no começo da RAM (`0x2000...`) e a variável local perto do **fim** da RAM, porque a pilha começa no fim e cresce para baixo. Os módulos 02 e 03 explicam por quê.

---

## 7. O registrador DATA mascarado (exclusividade da TI)

No GPIO do TM4C, o registrador de dados (`DATA`) não ocupa um endereço só: ele ocupa **256 endereços** (offsets `0x000` a `0x3FC`). Os bits 9 a 2 do endereço usado funcionam como uma **máscara**: só os pinos cujo bit está ligado na máscara são lidos ou escritos.

```
endereço = base_da_porta + (máscara << 2)
```

| Escrita | Endereço | Efeito |
|---|---|---|
| `GPIOF->DATA_BITS[0x08] = 0xFF;` | base + `0x020` | só o pino 3 (máscara `0000 1000`) vai para 1 |
| `GPIOF->DATA_BITS[0x0E] = 0x04;` | base + `0x038` | pinos 1, 2 e 3: PF2 = 1, PF1 = PF3 = 0 |
| `GPIOF->DATA = 0x08;` | base + `0x3FC` | máscara `1111 1111`: todos os pinos |

É uma escrita **atômica** (uma única operação) que muda só os pinos escolhidos, sem precisar ler antes. Resolve o problema do ler-modificar-escrever da seção 5. O módulo 05 usa isso para piscar o LED.

---

## 8. Bit-banding

No Cortex-M4, duas regiões de memória têm um "espelho" especial em que **cada bit ganha um endereço próprio de 32 bits**. Escrever 1 ou 0 nesse endereço liga ou desliga **só aquele bit**, numa única operação.

| Região real | Espelho (alias) |
|---|---|
| SRAM `0x2000_0000`–`0x200F_FFFF` | `0x2200_0000`–`0x23FF_FFFF` |
| Periféricos `0x4000_0000`–`0x400F_FFFF` | `0x4200_0000`–`0x43FF_FFFF` |

```
endereço_alias = base_alias + (endereço_do_byte − base_região) × 32 + bit × 4
```

Exemplo: o bit 3 do registrador `DATA` (offset `0x3FC`) da porta F do 123 (`0x4002_53FC`):
```
0x4200_0000 + 0x0002_53FC × 32 + 3 × 4 = 0x424A_7F8C
```
Veja a macro `BITBAND_PERIPH()` em `src/main.c`. No GPIO do TM4C o DATA mascarado já faz o mesmo papel; o bit-banding é mais útil para variáveis na RAM e para outros periféricos.

---

## 9. Ordem dos bytes e alinhamento

- O Cortex-M dos TM4C é **little-endian**: o byte menos significativo fica no endereço menor. A word `0x12345678` aparece na memória como `78 56 34 12`. Por isso um *dump* de memória parece "de trás para frente".
- **Alinhamento**: uma word "alinhada" começa num endereço múltiplo de 4. O Cortex-M4 aceita leituras desalinhadas na maioria das instruções (mais lentas), mas algumas instruções e o modo de verificação estrita geram erro. Prefira dados alinhados.

---

## Exercícios

1. Converta à mão para binário: `0x0E`, `0xA5`, `0x8000_0000`. Confira com o Python.
2. Escreva em C a linha que liga os bits 1 e 3 de um registrador `r` sem mexer nos outros, e outra que desliga o bit 0.
3. Leia `SYSCTL_DID0` e descubra a revisão do silício do seu chip; procure-a na errata.
4. Comente o `periph_enable` da UART em `common/lib/uart.c`, grave e veja no GDB em que exceção o programa parou (`p/x $xpsr & 0x1ff`).
5. No 123, habilite a entrada AHB da porta F (`SYSCTL_GPIOHBCTL |= 1 << 5`) e pisque o LED pelo endereço `0x4005_D000`.
6. Calcule o endereço bit-band do bit 4 (botão SW1) do `GPIOF->DATA` no 123 e leia o botão por ele.
