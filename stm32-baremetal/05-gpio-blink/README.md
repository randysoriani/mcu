# Módulo 05 — GPIO como saída: o "Hello World" (piscar um LED)

> **Objetivo deste módulo:** fazer o primeiro programa com efeito visível, piscar o LED verde **LD2** da Nucleo, e entender cada linha: por que é preciso ligar o clock, como um número escrito num registrador vira tensão num pino e como calcular os bits de cada pino.

Código:
- [`f0/main.c`](f0/main.c) — Nucleo-F072RB
- [`f4/main.c`](f4/main.c) — Nucleo-F446RE

Os dois programas são **autocontidos**: só usam o arquivo de definições de registradores. Não configuram clock nenhum; rodam com o oscilador interno do reset (8 ou 16 MHz).

```sh
make MCU=f0 flash      # ou MCU=f4
```
O LED verde ao lado do botão azul começa a piscar.

---

## 1. O que é GPIO

**GPIO** (*General Purpose Input/Output*, entrada/saída de uso geral) é o periférico que controla os pinos do chip como sinais digitais simples:

- como **saída**, o programa decide se o pino fica em **nível alto** (≈ 3,3 V, "1") ou **baixo** (0 V, "0") — para acender LEDs, acionar relés, gerar sinais;
- como **entrada**, o programa lê se a tensão no pino está alta ou baixa — para ler botões e sensores (módulo 08).

### Nomes dos pinos
Os pinos são agrupados em **portas** de 16 pinos, identificadas por letras: porta A, porta B, porta C... Cada pino tem um número de 0 a 15. O nome **PA5** significa "**P**orta **A**, pino **5**".

Cada porta é um periférico separado, com seu próprio bloco de registradores: GPIOA, GPIOB, GPIOC...

### O LED da Nucleo
O manual da placa (UM1724) mostra que o LED verde LD2 está ligado ao pino **PA5**, assim:

```
PA5 ──[ resistor ]──►|── GND
                     LED
```
- PA5 em nível **alto** (3,3 V): passa corrente pelo LED → **acende**.
- PA5 em nível **baixo** (0 V): não há diferença de tensão → **apaga**.

Então "acender o LED" é "colocar o pino PA5 em 1".

---

## 2. A receita de qualquer periférico

Todo periférico, deste até o último módulo, segue os mesmos três passos:

1. **Ligar o clock** do periférico (no RCC, módulo 04). Depois do reset todos estão desligados; um periférico sem clock ignora tudo que você escreve.
2. **Configurar** o periférico (aqui: dizer que o pino PA5 é uma saída).
3. **Usar** (aqui: escrever 1 e 0 no pino).

---

## 3. O programa, linha por linha (`f0/main.c`)

```c
#include "device.h"
```
Traz as definições de todos os registradores do chip (`RCC`, `GPIOA`, os nomes dos bits...). Elas foram escritas à mão a partir do Reference Manual e ficam em `common/f0/stm32f0.h`.

```c
static void delay(volatile uint32_t n)
{
    while (n--) {
        __asm volatile ("nop");
    }
}
```
Um atraso "à força bruta": conta de `n` até zero, executando uma instrução que não faz nada (`nop`) em cada volta. Explicado na seção 7.

```c
int main(void)
{
    /* 1) Liga o clock do GPIOA */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
```
**Passo 1 da receita.** No F072, as portas GPIO estão no barramento AHB, e o bit que liga o clock da porta A é o bit 17 do registrador `RCC_AHBENR`. `RCC_AHBENR_GPIOAEN` é só um nome para `(1U << 17)`. O `|=` liga esse bit sem mexer nos outros.

```c
    /* 2) PA5 como saída: MODER5 = 01 */
    GPIOA->MODER &= ~(3U << (5 * 2));     /* apaga os 2 bits do pino 5 */
    GPIOA->MODER |=  (1U << (5 * 2));     /* escreve 01 */
```
**Passo 2 da receita.** O registrador `MODER` define o modo de cada pino, usando **2 bits por pino** (detalhes na seção 5). O pino 5 usa os bits 10 e 11. A primeira linha zera esses dois bits; a segunda escreve `01`, que significa "saída".

```c
    for (;;) {
        GPIOA->BSRR = (1U << 5);          /* LED aceso  (PA5 = 1) */
        delay(200000);
        GPIOA->BSRR = (1U << (5 + 16));   /* LED apagado (PA5 = 0) */
        delay(200000);
    }
}
```
**Passo 3 da receita**, para sempre (`for (;;)` é um laço infinito: um microcontrolador nunca "termina" o programa). O registrador `BSRR` liga um pino quando se escreve 1 no bit do pino (bits 0–15) e desliga quando se escreve 1 no bit do pino + 16 (bits 16–31). Ver seção 6.

A versão do F4 (`f4/main.c`) é igual, com duas diferenças:
- o clock do GPIO fica em `RCC->AHB1ENR`, bit 0 (no F4 as portas estão no barramento AHB1);
- ela inverte o pino com `GPIOA->ODR ^= (1U << 5);` em vez de usar o `BSRR`, para mostrar as duas formas.

---

## 4. Como um bit vira tensão no pino

Cada pino tem um pequeno circuito de saída dentro do chip:

```
                    ┌────────────── VDD (3,3 V)
                    │
                [chave de cima]
   valor (ODR) ──►  controle ───────────●── pino PA5 ──► LED
                [chave de baixo]        │
                    │                   │
                   GND        entrada (IDR) ◄──┘
```

- Escrever **1** no bit do pino fecha a chave de cima: o pino é ligado aos 3,3 V.
- Escrever **0** fecha a chave de baixo: o pino é ligado ao GND.
- O registrador **IDR** lê a tensão que está realmente no pino (módulo 08).

Esse jeito de funcionar, com as duas chaves, chama-se **push-pull** (o pino "empurra" para 1 ou "puxa" para 0) e é o padrão. Existe também o **open-drain**, em que só a chave de baixo funciona e o nível 1 vem de um resistor externo (usado no I2C, módulo 13).

---

## 5. Os registradores do GPIO

Cada porta tem o mesmo conjunto de registradores (iguais no F0 e no F4). O endereço de cada um é o endereço-base da porta mais o *offset*:

- endereço-base da GPIOA: F0 `0x4800_0000`, F4 `0x4002_0000`;
- as outras portas vêm em seguida, de `0x400` em `0x400` bytes (GPIOB = base + `0x400`, GPIOC = base + `0x800`...).

| Offset | Registrador | Bits por pino | O que faz |
|---|---|---|---|
| 0x00 | `MODER` | 2 | modo: 00 entrada, **01 saída**, 10 função alternativa (o pino passa a ser controlado por outro periférico, como a serial), 11 analógico |
| 0x04 | `OTYPER` | 1 | tipo de saída: 0 push-pull, 1 open-drain |
| 0x08 | `OSPEEDR` | 2 | velocidade das bordas: 00 baixa, 01 média, 10 alta, 11 muito alta |
| 0x0C | `PUPDR` | 2 | resistor interno: 00 nenhum, 01 pull-up, 10 pull-down (módulo 08) |
| 0x10 | `IDR` | 1 | leitura do pino (só leitura) |
| 0x14 | `ODR` | 1 | valor de saída: 1 = nível alto |
| 0x18 | `BSRR` | — | bits 0–15: escrever 1 **liga** o pino; bits 16–31: escrever 1 **desliga** (só escrita) |
| 0x1C | `LCKR` | 1 | trava a configuração até o próximo reset |
| 0x20 | `AFR[0]` (AFRL) | 4 | qual função alternativa, pinos 0–7 |
| 0x24 | `AFR[1]` (AFRH) | 4 | qual função alternativa, pinos 8–15 |
| 0x28 | `BRR` | 1 | só F0: escrever 1 desliga o pino |

### Achando os bits de um pino

Registradores com **1 bit por pino** (`OTYPER`, `IDR`, `ODR`) são fáceis: o pino *n* é o bit *n*.

Registradores com **2 bits por pino** (`MODER`, `OSPEEDR`, `PUPDR`) dão 2 bits para cada um dos 16 pinos, totalizando os 32 bits:

```
bit:     31 30 | 29 28 | ... | 11 10 | 9 8 | 7 6 | 5 4 | 3 2 | 1 0
pino:      15  |  14   | ... |   5   |  4  |  3  |  2  |  1  |  0
```
O pino *n* ocupa os bits `2n+1` e `2n`. Para o pino 5: bits 11 e 10.

Em C:
- a "máscara" com os dois bits do pino 5 é `3U << (5 * 2)` = `3 << 10` = `0x0C00` (binário `11` nos bits 11:10);
- o valor "saída" (`01`) posicionado no pino 5 é `1U << (5 * 2)` = `0x0400`.

```c
GPIOA->MODER &= ~(3U << 10);   /* bits 11:10 = 00 */
GPIOA->MODER |=  (1U << 10);   /* bits 11:10 = 01 -> saída */
```

### Por que nunca escrever `GPIOA->MODER = 0x400;`

Parece mais simples, mas zera os modos de **todos** os outros pinos da porta A. Os pinos PA13 e PA14 são usados pelo ST-LINK para gravar e depurar o chip (interface SWD); depois do reset eles estão em "função alternativa". Zerá-los corta a conexão com o depurador, e a placa passa a não aceitar novas gravações até que se use o procedimento de "connect under reset" (módulo 00).

Por isso a regra: **mexa só nos bits do seu pino** (apagar e depois escrever, como acima).

Valores do `MODER` logo após o reset:

| Porta | Valor | Por quê |
|---|---|---|
| GPIOA (F0) | `0x2800_0000` | PA13/PA14 em função alternativa (SWD) |
| GPIOA (F4) | `0xA800_0000` | PA13/PA14/PA15 em função alternativa (SWD/JTAG) |
| GPIOB (F4) | `0x0000_0280` | PB3/PB4 em função alternativa (JTAG) |
| demais | `0x0000_0000` | todos os pinos como entrada |

---

## 6. `ODR` × `BSRR`: duas formas de mudar o pino

```c
GPIOA->ODR ^= (1U << 5);        /* inverte o pino 5 */
GPIOA->BSRR = (1U << 5);        /* liga o pino 5 */
GPIOA->BSRR = (1U << 21);       /* desliga o pino 5 (5 + 16) */
```

`ODR ^= ...` funciona, mas por baixo são três passos: ler o `ODR`, inverter o bit, escrever de volta. Se uma interrupção mudar **outro** pino da mesma porta entre a leitura e a escrita, a mudança dela é desfeita quando o valor antigo é escrito de volta.

O `BSRR` não tem esse problema: é só escrita, e os bits em 0 **não fazem nada**. Escrever `1 << 5` mexe apenas no pino 5, numa única operação. Por isso o `BSRR` é o jeito recomendado quando há interrupções mexendo na mesma porta.

---

## 7. O atraso por laço e o otimizador

```c
static void delay(volatile uint32_t n)
{
    while (n--) { __asm volatile ("nop"); }
}
```

O compilador otimiza o código (`-O2`) e elimina tudo que "não tem efeito". Um laço que só conta e não faz mais nada seria removido inteiro, e o LED piscaria rápido demais para ser visto. Duas coisas impedem isso:
- `volatile` no parâmetro obriga o compilador a realmente ler e escrever `n` a cada volta;
- `__asm volatile ("nop")` insere uma instrução que o compilador não pode remover.

O tempo desse atraso depende da frequência do clock, da otimização e da memória, então **ele não serve para medir tempo**: só para este primeiro exemplo. O módulo 06 o substitui por um temporizador de verdade (SysTick).

---

## 8. Velocidade do pino (`OSPEEDR`)

Apesar do nome, não é a velocidade com que o programa pode mudar o pino: é **quão rápidas são as bordas** (a transição de 0 para 1). Bordas rápidas geram mais interferência eletromagnética e consomem mais.
- **baixa**: LEDs, relés, sinais lentos (o padrão serve);
- **alta / muito alta**: comunicação rápida (SPI acima de alguns MHz, a saída de clock MCO do módulo 04).

Frequências máximas aproximadas (F4, 3,3 V): baixa ~2–4 MHz, média ~25 MHz, alta ~50 MHz, muito alta ~100 MHz (datasheet, *I/O AC characteristics*).

---

## 9. Vendo o que o compilador gerou

```sh
make MCU=f0 disasm && less build/f0/05-gpio-blink.lst
```
```
080000c0 <main>:
 80000c0:  2380   movs  r3, #128        @ 0x80
 80000c2:  4a16   ldr   r2, [pc, #88]   @ r2 = 0x40021000 (endereço do RCC)
 80000c4:  029b   lsls  r3, r3, #10     @ r3 = 0x80 << 10 = 1 << 17 (GPIOAEN)
 80000c6:  6951   ldr   r1, [r2, #20]   @ r1 = RCC->AHBENR (offset 0x14 = 20)
 80000ca:  430b   orrs  r3, r1          @ liga o bit 17
 80000ce:  6153   str   r3, [r2, #20]   @ escreve de volta: RCC->AHBENR |= GPIOAEN
 80000cc:  2190   movs  r1, #144        @ 0x90
 80000d0:  05c9   lsls  r1, r1, #23     @ r1 = 0x90 << 23 = 0x48000000 (GPIOA)
 80000d2:  680b   ldr   r3, [r1, #0]    @ lê GPIOA->MODER
```
(saída real do `objdump`, F0, com `-O2`.) Cada linha de C virou poucas instruções: carregar um endereço, ler o registrador (`ldr`), mudar um bit (`orrs`), escrever de volta (`str`). O *offset* do registrador dentro da struct aparece direto na instrução (`#20` = 0x14). O Cortex-M0 não consegue colocar um número de 32 bits numa instrução só, por isso constantes viram `movs` + deslocamento (`lsls`) ou uma leitura de uma tabela de constantes (*literal pool*).

---

## 10. LEDs em outras placas

| Placa | LED | Acende com |
|---|---|---|
| Nucleo-64 (F030R8, F072RB, F401RE, F411RE, F446RE) | PA5 | nível alto |
| STM32F4-Discovery (F407) | PD12 verde, PD13 laranja, PD14 vermelho, PD15 azul | nível alto |
| STM32F0-Discovery (F051) | PC8 azul, PC9 verde | nível alto |
| Black Pill (F401/F411CE) | PC13 | **nível baixo** (o LED vai do 3,3 V ao pino) |
| Blue Pill (F103, fora do escopo) | PC13 | nível baixo |

---

## Exercícios

1. Mude os valores do `delay` e veja a frequência mudar. Depois pisque no padrão SOS do código Morse (· · · − − − · · ·).
2. No F0, apague o LED com o registrador `BRR` em vez do `BSRR`. Por que o F4 não precisa de `BRR`?
3. Configure PA5 como open-drain (`OTYPER`). O LED ainda acende? Explique olhando o circuito da seção 1.
4. Escreva `GPIOA->MODER = 0x400;` de propósito, grave, e tente gravar de novo. Depois recupere a placa com o procedimento de "connect under reset" do módulo 00.
5. Meça com um osciloscópio a frequência máxima do laço `while (1) GPIOA->ODR ^= 1 << 5;` com o clock de reset e depois com o clock do módulo 04.
