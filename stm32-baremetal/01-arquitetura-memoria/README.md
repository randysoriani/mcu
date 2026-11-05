# Módulo 01 — Arquitetura Cortex-M e organização da memória

> **Objetivo deste módulo:** montar o "mapa mental" do microcontrolador: o que tem dentro do chip, como o processador enxerga tudo como endereços de memória e como o código em C conversa com o hardware. Sem isso, os próximos módulos parecem uma sopa de números hexadecimais.

Código: [`src/main.c`](src/main.c) (o mesmo para F0 e F4). Ele imprime na serial a identificação do processador e do chip e **onde cada tipo de variável foi parar na memória**; no F4 pisca o LED usando uma técnica chamada *bit-banding*.

```sh
make MCU=f0 flash   # ou MCU=f4
picocom -b 115200 /dev/ttyACM0
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

Os bits de uma word são numerados da direita para a esquerda, **de 0 a 31**. "Bit 5" é o sexto a partir da direita.

**Hexadecimal** (base 16, prefixo `0x`) é só uma forma compacta de escrever binário: **cada dígito hexa corresponde a exatamente 4 bits**.

| Hexa | Binário | Decimal |
|---|---|---|
| `0` | `0000` | 0 |
| `1` | `0001` | 1 |
| `5` | `0101` | 5 |
| `8` | `1000` | 8 |
| `A` | `1010` | 10 |
| `F` | `1111` | 15 |

Exemplo: `0x20` = `0010 0000` em binário = bit 5 ligado = 32 em decimal. Por isso escrevemos `0x20` para mexer no pino 5. Com a prática você "lê" binário olhando o hexa.

> Dica: a calculadora do Linux (`gnome-calculator`, modo "programação") e o Python (`hex(32)`, `bin(0x20)`) convertem entre as bases.

---

## 1. O que tem dentro de um microcontrolador

```
┌──────────────────────────── chip STM32 ───────────────────────────┐
│                                                                    │
│   ┌──────────────┐        barramentos (as "estradas" internas)    │
│   │ Núcleo ARM   │◄═════════════════╦══════════════╦════════════╗ │
│   │ Cortex-M     │                  ║              ║            ║ │
│   │ (processador)│            ┌─────╨────┐   ┌─────╨────┐ ┌─────╨───────┐
│   └──────────────┘            │  Flash   │   │   RAM    │ │ Periféricos │
│                               │(programa)│   │(variáveis│ │ GPIO, UART, │
│                               └──────────┘   └──────────┘ │ timers, ADC │
│                                                           └──────┬──────┘
└──────────────────────────────────────────────────────────────────┼───────┘
                                                                   │
                                                       pinos do chip (LED, botão, sensores...)
```

| Parte | Função | Na Nucleo-F446RE |
|---|---|---|
| **Núcleo** | executa as instruções do programa | Cortex-M4F a até 180 MHz |
| **Flash** | guarda o programa (não apaga sem energia) | 512 KB |
| **RAM** | guarda as variáveis durante a execução (apaga sem energia) | 128 KB |
| **Periféricos** | circuitos especializados que conversam com o mundo externo: pinos (GPIO), comunicação serial (UART, SPI, I2C), temporizadores (timers), conversor analógico-digital (ADC)... | dezenas |
| **Barramentos** | as "estradas" que ligam o núcleo à memória e aos periféricos | AHB, APB1, APB2 |

A grande diferença para um PC: não há sistema operacional nem drivers. **O seu programa controla diretamente cada periférico**, escrevendo números em registradores. Este curso inteiro é sobre isso.

---

## 2. Os dois núcleos do curso: Cortex-M0 × Cortex-M4

A ARM projeta uma família de núcleos para microcontroladores, os **Cortex-M**. Os fabricantes (ST, TI, NXP, Microchip...) compram a licença e constroem chips em volta deles. Por isso o que você aprender sobre o núcleo vale para chips de qualquer fabricante.

| | Cortex-M0 (STM32F0) | Cortex-M4F (STM32F4) | Na prática |
|---|---|---|---|
| Arquitetura | ARMv6-M | ARMv7E-M | o M4 é a geração "maior" |
| Instruções | Thumb (16 bits) + poucas de 32 bits | Thumb-2 completo, DSP | o M4 faz mais coisa por instrução |
| Divisão por hardware | **não** (feita em software pela `libgcc`) | sim, 2–12 ciclos | `a / b` é bem mais lento no M0 |
| FPU | não | sim (`float` de 32 bits) | `float` no M0 é emulado e lento |
| Acesso desalinhado (ler uma word em endereço não múltiplo de 4) | **gera erro (HardFault)** | permitido na maioria das instruções | cuidado ao converter ponteiros no M0 |
| Interrupções de periféricos | até 32 | até 240 (F446 usa 97) | |
| Níveis de prioridade | 4 | 16 (no STM32) | |
| VTOR (mudar a tabela de vetores de lugar) | não | sim | importante para bootloaders |
| Bit-banding | não | sim | seção 7 |
| MPU (proteção de memória) | não | sim (8 regiões) | |
| Registradores de diagnóstico de falha | só HardFault | HardFault, MemManage, BusFault, UsageFault | depurar erros é mais fácil no M4 |
| Contador de ciclos (DWT) | não | sim | medir tempo de código |

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
| `CONTROL` | | modo privilegiado, qual pilha usar |
| (só M4) `s0`–`s31`, `FPSCR` | | registradores da FPU |

**O bit 0 dos endereços de função.** Os Cortex-M só entendem instruções *Thumb*, e a ARM usa o bit 0 de um endereço de função como marca de "isto é Thumb". Por isso o `main` aparece num endereço ímpar como `0x080001d5`: a instrução está em `0x080001d4`. O compilador cuida disso; um ponteiro de função com bit 0 = 0 causa erro (HardFault).

### Modos de execução e interrupções
- **Thread mode**: o código normal (`main` e o que ele chama).
- **Handler mode**: dentro de uma interrupção ou exceção.

Quando uma interrupção chega, o próprio hardware **guarda automaticamente** na pilha os registradores `r0`–`r3`, `r12`, `LR`, `PC` e `xPSR` (e os da FPU, no M4, se estiverem em uso), executa o handler e depois **restaura** tudo. Por isso, no Cortex-M, um handler de interrupção é uma **função C comum**: não precisa de nenhuma palavra-chave especial.

---

## 3. O mapa de memória: tudo é endereço

Esta é a ideia mais importante do módulo.

O processador tem um espaço de endereços de 32 bits: de `0x0000_0000` a `0xFFFF_FFFF`, ou seja, 4 GB de "endereços possíveis". O chip não tem 4 GB de memória: a maior parte desses endereços está vazia. Mas **cada coisa que existe** (Flash, RAM e cada registrador de cada periférico) **ocupa um endereço fixo**.

Analogia: imagine uma rua muito longa, com 4 bilhões de números de casa. A maioria dos lotes está vazia. Nos números `0x0800_0000` em diante fica o "depósito" (a Flash); a partir de `0x2000_0000`, o "escritório" (a RAM); a partir de `0x4000_0000`, as "salas de controle" dos periféricos. Para acender um LED, o programa vai até a casa da porta GPIO e escreve um número lá dentro.

Isso se chama **memory-mapped I/O** (entrada/saída mapeada em memória): ler ou escrever num periférico é **exatamente igual** a ler ou escrever numa variável — só muda o endereço.

A ARM divide os 4 GB em regiões fixas, iguais em todo Cortex-M:

```
0xFFFF_FFFF ┌───────────────────────────┐
            │ Específico do fabricante  │
0xE010_0000 ├───────────────────────────┤
            │ Periféricos do NÚCLEO     │  SysTick 0xE000E010, NVIC 0xE000E100,
0xE000_0000 │ (iguais em todo Cortex-M) │  SCB 0xE000ED00 ...
            ├───────────────────────────┤
            │ Memória/dispositivos      │  (memória externa em chips maiores)
0x6000_0000 │ externos                  │
            ├───────────────────────────┤
            │ Periféricos do CHIP       │  GPIO, UART, timers, RCC... (da ST)
0x4000_0000 │                           │
            ├───────────────────────────┤
            │ SRAM                      │  variáveis
0x2000_0000 │                           │
            ├───────────────────────────┤
            │ Código                    │  Flash (0x0800_0000 no STM32),
0x0000_0000 └───────────────────────────┘  memória do bootloader, espelho de boot
```

### STM32F072RB

| Endereço | Tamanho | O que mora lá |
|---|---|---|
| `0x0000_0000` | — | **espelho** da Flash, da memória de sistema ou da RAM, conforme o pino BOOT0 (seção 5) |
| `0x0800_0000` | 128 KB | Flash principal: o seu programa |
| `0x1FFF_C800` | 12 KB | memória de sistema: um bootloader gravado de fábrica pela ST |
| `0x1FFF_F7AC..` | | dados de fábrica: número de série único, calibração do ADC, tamanho da Flash |
| `0x1FFF_F800` | | *option bytes* (configurações permanentes do chip) |
| `0x2000_0000` | 16 KB | SRAM |
| `0x4000_0000` | | periféricos no barramento APB (timers, USART2, I2C, CAN, DAC, USART1, ADC...) |
| `0x4002_0000` | | periféricos no barramento AHB (DMA, **RCC em `0x4002_1000`**, controle da Flash) |
| `0x4800_0000` | | **portas GPIOA..GPIOF** (ligadas direto ao AHB, acesso em 1 ciclo) |
| `0xE000_0000` | | periféricos do núcleo |

### STM32F446RE

| Endereço | Tamanho | O que mora lá |
|---|---|---|
| `0x0800_0000` | 512 KB | Flash (dividida em setores: 4 de 16 KB, 1 de 64 KB, 3 de 128 KB) |
| `0x1FFF_0000` | 30 KB | memória de sistema (bootloader da ST) |
| `0x1FFF_7800` | 528 B | OTP: área que só pode ser gravada uma vez |
| `0x1FFF_7A10..` | | número de série único, tamanho da Flash, calibração |
| `0x1FFF_C000` | | option bytes |
| `0x2000_0000` | 112 KB | SRAM1 |
| `0x2001_C000` | 16 KB | SRAM2 (logo em seguida: na prática, 128 KB contínuos) |
| `0x4000_0000` | | barramento APB1 (45 MHz): timers 2–7 e 12–14, RTC, watchdogs, SPI2/3, USART2/3, UART4/5, I2C1–3, CAN, PWR, DAC |
| `0x4001_0000` | | barramento APB2 (90 MHz): timers 1/8/9–11, USART1/6, ADC, SPI1/4, SYSCFG, EXTI, SAI |
| `0x4002_0000` | | barramento AHB1 (180 MHz): **GPIOA..H**, CRC, **RCC em `0x4002_3800`**, controle da Flash, DMA1/2 |
| `0x5000_0000` | | AHB2: USB, câmera (DCMI) |
| `0xA000_0000` | | controladores de memória externa (FMC, QUADSPI) |

> De onde vêm esses números? Da tabela *Memory map* do Reference Manual (RM0091 seção 2.2; RM0390 seção 2.2). Os headers do curso (`common/f0/stm32f0.h` e `common/f4/stm32f4.h`) copiam essas tabelas. Abra-os e procure `RCC_BASE`.

---

## 4. Barramentos: as estradas internas

```
             Cortex-M4
        I-Code  D-Code  System          DMA1    DMA2
          │       │       │              │       │
          ▼       ▼       ▼              ▼       ▼
   ┌──────────────── matriz de barramentos AHB ─────────────┐
   │                                                        │
 Flash      SRAM1   SRAM2   AHB1 (GPIO, RCC)   AHB2   memória externa
                               │
                      ┌────────┴────────┐
                    APB1 (÷4)        APB2 (÷2)
                    45 MHz           90 MHz
```

O núcleo não fala direto com cada periférico: os acessos passam por **barramentos** (*buses*). O AHB é rápido (mesma velocidade do núcleo); os APB são mais lentos e ficam "pendurados" no AHB por uma ponte. Na prática:

1. **Todo periférico começa desligado, sem clock.** Antes de usar um periférico, é preciso ligar o clock dele no RCC (módulo 04), num registrador como `RCC->AHB1ENR`. Sem clock, o que você escreve no periférico é ignorado e a leitura retorna 0. É o erro número 1 de quem começa.
2. **Acessos ao APB são mais lentos** que ao AHB. No F0 o GPIO fica direto no AHB justamente para ser rápido.
3. **Errata do F4** (*errata* = lista de defeitos conhecidos do chip): logo após ligar o clock de um periférico, é preciso esperar 2 ciclos antes de usá-lo. Uma leitura "de mentira" do próprio registrador do RCC resolve: `(void)RCC->AHB1ENR;`.
4. O **DMA** (módulo 15) é outro "motorista" nessas estradas: ele copia dados sem a CPU e disputa o barramento com ela.
5. A Flash é mais lenta que o núcleo. Acima de 24 MHz (F0) ou 30 MHz (F4) a CPU precisa esperar ciclos extras (*wait states*) a cada leitura, assunto do módulo 04.

---

## 5. Boot: o que aparece no endereço 0

Como visto no módulo 02, ao sair do reset o núcleo **sempre** lê:

| Endereço | Conteúdo | Vai para |
|---|---|---|
| `0x0000_0000` | valor inicial da pilha | **SP** |
| `0x0000_0004` | endereço do `Reset_Handler` | **PC** |

O STM32 escolhe o que aparece no endereço 0 pelo pino **BOOT0**:

| BOOT0 | O que aparece em 0x0 | Para que serve |
|---|---|---|
| 0 | Flash principal (`0x0800_0000`) | uso normal — a Nucleo tem BOOT0 ligado ao GND |
| 1 | Memória de sistema | bootloader da ST: permite gravar o chip pela serial ou USB, sem ST-LINK |
| 1 (+ configuração BOOT1) | SRAM | executar um programa carregado na RAM |

Por isso o linker script coloca a **tabela de vetores no início da Flash**.

---

## 6. Conversando com os registradores em C

Um registrador de periférico é só uma word num endereço conhecido. Por exemplo, no F4 o registrador `ODR` (*Output Data Register*) da porta A fica em `0x4002_0014`; cada bit dele controla a tensão de um pino. O bit 5 controla o pino PA5, onde está o LED da Nucleo.

Há três jeitos de escrever "ligue o bit 5 desse endereço", do mais cru ao usado no curso:

```c
/* 1) Ponteiro "na mão" */
*(volatile uint32_t *)0x40020014 |= (1U << 5);

/* 2) Um apelido (macro) para o registrador */
#define GPIOA_ODR (*(volatile uint32_t *)0x40020014)
GPIOA_ODR |= (1U << 5);

/* 3) Uma struct que descreve o bloco inteiro do periférico (estilo usado no curso) */
typedef struct {
    volatile uint32_t MODER;    /* offset 0x00 */
    volatile uint32_t OTYPER;   /* offset 0x04 */
    volatile uint32_t OSPEEDR;  /* offset 0x08 */
    volatile uint32_t PUPDR;    /* offset 0x0C */
    volatile uint32_t IDR;      /* offset 0x10 */
    volatile uint32_t ODR;      /* offset 0x14 */
    /* ... */
} GPIO_TypeDef;
#define GPIOA ((GPIO_TypeDef *)0x40020000)
GPIOA->ODR |= (1U << 5);
```

Lendo a forma 1 de dentro para fora:
- `0x40020014` é só um número;
- `(volatile uint32_t *)` diz ao compilador: "trate esse número como o endereço de uma word";
- o `*` da frente acessa o conteúdo desse endereço;
- `|= (1U << 5)` liga o bit 5 (explicado adiante).

A forma 3 funciona porque cada campo `uint32_t` da struct ocupa 4 bytes e o compilador os coloca em sequência: o 6º campo (`ODR`) fica a 0x14 bytes do início, exatamente o *offset* do registrador no manual. As vantagens: um único endereço-base por periférico, a mesma struct serve para GPIOA, GPIOB, GPIOC... e o depurador entende (`p/x *GPIOA` mostra tudo).

### Por que `volatile`?

O compilador é esperto: se você escreve duas vezes seguidas na mesma variável, ele elimina a primeira escrita; se lê várias vezes a mesma variável sem mudá-la, ele lê uma vez só. Para variáveis comuns isso é ótimo. Para registradores de hardware é um desastre, porque o hardware muda os valores "por fora" do programa:

```c
while (!(USART2->SR & (1 << 7))) { }    /* espera o hardware sinalizar "pronto" */
```
Sem `volatile`, o compilador pode ler o registrador uma única vez e transformar isso num laço infinito, ou eliminá-lo.

`volatile` significa: **"cada leitura e escrita que eu escrevi deve acontecer de verdade, na ordem em que escrevi"**. Use em:
- registradores de hardware (os headers do curso já declaram todos assim);
- variáveis compartilhadas entre uma interrupção e o `main`.

### Operações com bits

Quase tudo em embarcados é ligar e desligar bits individuais de um registrador sem mexer nos outros. As quatro operações básicas, com um exemplo de 8 bits para ficar visível:

| Operação | Código | Exemplo (reg = `1010 0001`, n = 2) | Resultado |
|---|---|---|---|
| **ligar** o bit n | `reg \|= (1U << n);` | `1010 0001` OU `0000 0100` | `1010 0101` |
| **desligar** o bit n | `reg &= ~(1U << n);` | `1010 0101` E `1111 1011` | `1010 0001` |
| **inverter** o bit n | `reg ^= (1U << n);` | `1010 0001` OU-exclusivo `0000 0100` | `1010 0101` |
| **testar** o bit n | `if (reg & (1U << n))` | `1010 0101` E `0000 0100` = `0000 0100` | diferente de 0 → ligado |

- `1U << n` cria um número com só o bit n ligado (a "máscara"). `1U << 5` = `0x20`.
- `~` inverte todos os bits da máscara.
- Use sempre `1U` (sem sinal): `1 << 31` com `int` é comportamento indefinido em C.

Muitos registradores têm **campos de vários bits**. Para escrever o valor `V` num campo de 2 bits que começa na posição `P`:
```c
reg = (reg & ~(3U << P))    /* 1. apaga os 2 bits do campo */
    | (V << P);             /* 2. escreve o valor novo     */
```
Exemplo real (módulo 05): o pino 5 ocupa os bits 11:10 do registrador `MODER`; o valor `01` significa "saída":
```c
GPIOA->MODER = (GPIOA->MODER & ~(3U << 10)) | (1U << 10);
```

### Ler-modificar-escrever e interrupções

`reg |= x` parece uma operação, mas são **três** instruções: ler o registrador, fazer o OU, escrever de volta. Se uma interrupção alterar o mesmo registrador bem no meio disso, a alteração dela se perde quando o programa escreve o valor antigo de volta. Formas de evitar:
- usar registradores feitos para isso, como o `BSRR` do GPIO (escrever 1 no bit n liga o pino, sem precisar ler antes);
- flags que se limpam escrevendo 1 (`rc_w1`) ou 0 (`rc_w0`), sem ler antes;
- bit-banding (seção 7, só M4);
- desligar as interrupções em volta do trecho crítico (`__disable_irq()` ... `__enable_irq()`).

Nas tabelas do Reference Manual, cada bit tem uma sigla de acesso: `rw` (leitura e escrita), `r` (só leitura), `w` (só escrita), `rc_w1`/`rc_w0` (limpa escrevendo 1/0), `rs` (o software liga, o hardware desliga).

---

## 7. Bit-banding (só F4)

No Cortex-M4, duas regiões de memória têm um "espelho" especial em que **cada bit ganha um endereço próprio de 32 bits**. Escrever 1 ou 0 nesse endereço liga ou desliga **só aquele bit**, numa única operação, sem o problema do ler-modificar-escrever.

| Região real | Espelho (alias) |
|---|---|
| SRAM `0x2000_0000`–`0x200F_FFFF` | `0x2200_0000`–`0x23FF_FFFF` |
| Periféricos `0x4000_0000`–`0x400F_FFFF` | `0x4200_0000`–`0x43FF_FFFF` |

```
endereço_alias = base_alias + (endereço_do_byte − base_região) × 32 + bit × 4
```

Exemplo: o bit 5 de `GPIOA->ODR` no F4 (registrador em `0x4002_0014`):
```
0x4200_0000 + 0x0002_0014 × 32 + 5 × 4 = 0x4240_0294
```
Escrever 1 em `0x4240_0294` acende o LED; 0 apaga. Veja a macro `BITBAND_PERIPH()` em `src/main.c`. O Cortex-M7 e os STM32 mais novos não têm bit-banding; em código portável, prefira o `BSRR`.

---

## 8. Ordem dos bytes e alinhamento

- O STM32 é **little-endian**: o byte menos significativo fica no endereço menor. A word `0x12345678` aparece na memória como `78 56 34 12`. Isso explica por que um *dump* de memória parece "de trás para frente".
- **Alinhamento**: uma word "alinhada" começa num endereço múltiplo de 4. No **Cortex-M0**, ler um `uint32_t` num endereço não alinhado gera **HardFault**. Cuidado ao converter um `uint8_t buffer[]` em `uint32_t *`; use `memcpy` ou monte o valor byte a byte.
- O compilador insere bytes de enchimento nas structs para manter os campos alinhados. `__attribute__((packed))` remove esse enchimento, mas gera acessos byte a byte (lentos) no M0.

---

## 9. Identificando o chip

O programa deste módulo lê registradores que dizem quem é o chip:

| Registrador | Endereço | O que contém |
|---|---|---|
| `SCB->CPUID` | `0xE000_ED00` | qual núcleo ARM: fabricante `0x41` (ARM), modelo `0xC20` (M0) ou `0xC24` (M4), revisão |
| `DBGMCU->IDCODE` | F0 `0x4001_5800`, F4 `0xE004_2000` | qual chip da ST: `0x448` = F07x, `0x421` = F446; revisão do silício |
| Tamanho da Flash | F0 `0x1FFF_F7CC`, F4 `0x1FFF_7A22` | em KB |
| Unique ID | F0 `0x1FFF_F7AC`, F4 `0x1FFF_7A10` | 96 bits diferentes em cada chip fabricado (número de série) |

Saída típica (F446):
```
CPUID       = 0x410FC241  (implementer 0x41, partno 0xC24 = Cortex-M4, r0p1)
IDCODE      = 0x10006421  (DEV_ID 0x421, REV_ID 0x1000)
Flash       = 512 KB
...
  tabela_const     0x08000c50  (.rodata, FLASH)
  var_inicializada 0x20000000  (.data, RAM) = 0xCAFEBABE
  var_zerada       0x20000004  (.bss,  RAM) = 0
  var_local        0x2001ffe4  (pilha)      = 42
```
Repare: a constante ficou na Flash (`0x08...`), as variáveis globais no começo da RAM (`0x2000...`) e a variável local perto do **fim** da RAM, porque a pilha começa no fim e cresce para baixo. Os módulos 02 e 03 explicam por quê.

---

## Exercícios

1. Converta à mão para binário: `0x0C`, `0xA5`, `0x8000_0000`. Confira com o Python.
2. Escreva em C a linha que liga os bits 3 e 7 de um registrador `r` sem mexer nos outros, e outra que desliga o bit 0.
3. Calcule o endereço de bit-band do bit 13 de `GPIOC->IDR` (`0x4002_0810`) no F446 e leia o botão da Nucleo por ele.
4. No F0, crie `uint8_t buf[8]` e leia `*(uint32_t *)&buf[1]`. O que acontece? Com o GDB, veja em que handler o programa parou.
5. Leia os 96 bits do Unique ID e imprima-os como um número de série hexadecimal.
6. Descubra no RM0390 em qual barramento está o TIM1 e qual a frequência máxima do clock dele.
