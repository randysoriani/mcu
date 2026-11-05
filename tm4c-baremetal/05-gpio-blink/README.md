# Módulo 05 — GPIO como saída: o "Hello World" (piscar um LED)

> **Objetivo deste módulo:** fazer o primeiro programa com efeito visível, piscar um LED da LaunchPad, e entender cada linha: por que é preciso ligar o clock, por que é preciso habilitar a função digital do pino e como um número escrito num registrador vira tensão no pino.

Código:
- [`123/main.c`](123/main.c) — pisca o LED **verde (PF3)** da EK-TM4C123GXL
- [`129/main.c`](129/main.c) — pisca os LEDs **D1 (PN1)** e **D2 (PN0)** alternados na EK-TM4C1294XL

Os dois programas são **autocontidos**: só usam o arquivo de definições de registradores. Não configuram clock; rodam com o oscilador interno do reset (16 MHz).

```sh
make MCU=123 flash      # ou MCU=129
```

---

## 1. O que é GPIO

**GPIO** (*General Purpose Input/Output*, entrada/saída de uso geral) é o periférico que controla os pinos do chip como sinais digitais simples:

- como **saída**, o programa decide se o pino fica em **nível alto** (≈ 3,3 V, "1") ou **baixo** (0 V, "0") — para acender LEDs, acionar relés, gerar sinais;
- como **entrada**, o programa lê se a tensão no pino está alta ou baixa — para ler botões e sensores (módulo 08).

### Nomes dos pinos
No TM4C os pinos são agrupados em **portas de 8 pinos**, identificadas por letras: porta A, porta B, ..., porta F (no 129, até a porta Q, sem I e O). Cada pino tem um número de 0 a 7. O nome **PF3** significa "**P**orta **F**, pino **3**".

Cada porta é um periférico separado, com seu próprio bloco de registradores: GPIOA, GPIOB, ..., GPIOF.

### Os LEDs das LaunchPads
| Placa | LED | Pino |
|---|---|---|
| EK-TM4C123GXL | LED RGB (três LEDs num só) | PF1 = vermelho, **PF3 = verde**, PF2 = azul |
| EK-TM4C1294XL | quatro LEDs | **D1 = PN1**, D2 = PN0, D3 = PF4, D4 = PF0 |

Todos acendem com o pino em **nível alto**. Ligar PF1 e PF3 ao mesmo tempo no 123 mistura vermelho e verde: amarelo.

---

## 2. A receita de qualquer periférico

Todo periférico, deste até o último módulo, segue os mesmos passos:

1. **Ligar o clock** do periférico (no System Control, módulo 04). Depois do reset todos estão desligados, e no TM4C mexer num periférico desligado trava o programa com um erro.
2. **Configurar** o periférico (aqui: dizer que o pino é saída e habilitar a função digital).
3. **Usar** (aqui: escrever 1 e 0 no pino).

---

## 3. O programa, linha por linha (`123/main.c`)

```c
#include "device.h"
```
Traz as definições de todos os registradores do chip (`SYSCTL_RCGCGPIO`, `GPIOF`...). Elas foram escritas à mão a partir do datasheet e ficam em `common/`.

```c
static void delay(volatile uint32_t n)
{
    while (n--) { __asm volatile ("nop"); }
}
```
Um atraso "à força bruta": conta de `n` até zero, executando uma instrução que não faz nada (`nop`) em cada volta. Explicado na seção 7.

```c
int main(void)
{
    /* 1) Clock da porta F e espera ela ficar pronta */
    SYSCTL_RCGCGPIO |= (1U << 5);
    while (!(SYSCTL_PRGPIO & (1U << 5))) { }
```
**Passo 1 da receita.** O registrador `SYSCTL_RCGCGPIO` tem um bit para cada porta: A = bit 0, B = bit 1, ..., **F = bit 5**. Ligar o bit 5 liga o clock da porta F. O registrador `SYSCTL_PRGPIO` tem a mesma organização e avisa quando a porta está pronta para uso; o `while` espera esse bit ir a 1.

```c
    /* 2) PF3 como saída digital */
    GPIOF->DIR |= (1U << 3);       /* direção: 1 = saída */
    GPIOF->DEN |= (1U << 3);       /* habilita a função digital do pino */
```
**Passo 2 da receita.** Dois registradores com **1 bit por pino**:
- `DIR` (direção): bit em 1 = o pino é saída;
- `DEN` (*Digital Enable*): bit em 1 = a parte digital do pino está ligada.

**Todo pino do TM4C começa com a parte digital desligada.** Sem o `DEN`, o pino não lê nem escreve nada, e nenhum erro aparece: o LED simplesmente não acende. É o erro mais comum com este chip.

```c
    for (;;) {
        GPIOF->DATA_BITS[1U << 3] = 0xFF;      /* acende */
        delay(400000);
        GPIOF->DATA_BITS[1U << 3] = 0x00;      /* apaga  */
        delay(400000);
    }
}
```
**Passo 3 da receita**, para sempre (`for (;;)` é um laço infinito: um microcontrolador nunca "termina" o programa). A escrita usa o **DATA mascarado** da TI (seção 5): o endereço escolhido faz com que só o pino 3 seja afetado. Escrever `0xFF` põe o pino em 1; `0x00`, em 0.

A versão do 129 (`129/main.c`) é igual, com a porta N:
- porta N = **bit 12** do `RCGCGPIO` (A0 B1 C2 D3 E4 F5 G6 H7 J8 K9 L10 M11 N12 P13 Q14);
- os dois LEDs (pinos 0 e 1) são controlados juntos por `GPION->DATA_BITS[0x3]`, e o `^= 0x3` inverte os dois de uma vez.

---

## 4. Como um bit vira tensão no pino

Cada pino tem um pequeno circuito de saída dentro do chip:

```
                    ┌────────────── VDD (3,3 V)
                    │
                [chave de cima]
   valor (DATA) ──► controle ───────────●── pino PF3 ──[resistor]──► LED ──► GND
                [chave de baixo]        │
                    │                   │
                   GND        leitura (DATA) ◄──┘     (só funciona com DEN = 1)
```

- Escrever **1** fecha a chave de cima: o pino é ligado aos 3,3 V, passa corrente pelo LED e ele acende.
- Escrever **0** fecha a chave de baixo: o pino vai a 0 V e o LED apaga.

Esse jeito de funcionar chama-se **push-pull** e é o padrão. Existe também o **open-drain** (registrador `ODR`), em que só a chave de baixo funciona e o nível 1 vem de um resistor externo (usado no I2C, módulo 13).

---

## 5. O registrador DATA mascarado

No GPIO do TM4C, o registrador de dados não ocupa um endereço só: ele ocupa **256 endereços** (offsets `0x000` a `0x3FC`). Os bits 9 a 2 do endereço usado funcionam como uma **máscara**: só os pinos cujo bit está ligado na máscara são lidos ou escritos.

```
endereço = base_da_porta + (máscara << 2)
```

No código, a struct tem um vetor `DATA_BITS[]` cujo índice é a própria máscara:

| Escrita | Máscara | Endereço | Efeito |
|---|---|---|---|
| `DATA_BITS[0x08] = 0xFF` | `0000 1000` | base + `0x020` | PF3 = 1, os outros pinos **não mudam** |
| `DATA_BITS[0x08] = 0x00` | `0000 1000` | base + `0x020` | PF3 = 0 |
| `DATA_BITS[0x0E] = 0x04` | `0000 1110` | base + `0x038` | PF1 = 0, **PF2 = 1**, PF3 = 0 (LED azul) |
| `DATA = 0xFF` | `1111 1111` | base + `0x3FC` | todos os pinos |

Por que isso é útil? Para mudar um pino sem mexer nos outros, o jeito normal seria ler o registrador, alterar um bit e escrever de volta (três operações). Se uma interrupção mexesse em outro pino da mesma porta no meio disso, a alteração dela seria desfeita. Com o DATA mascarado a escrita é **uma operação só** e afeta **apenas** os pinos da máscara.

Na leitura, os bits fora da máscara voltam 0: `GPIOF->DATA_BITS[1 << 4]` vale `0x10` se o pino 4 estiver em 1, e `0` se estiver em 0.

---

## 6. Os registradores do GPIO

Cada porta tem o mesmo conjunto de registradores (iguais no 123 e no 129). O endereço é o endereço-base da porta mais o *offset*:
- 123 (entrada pelo barramento APB): GPIOA `0x4000_4000`, ..., GPIOF `0x4002_5000`;
- 129 (barramento AHB): GPIOA `0x4005_8000`, e as seguintes de `0x1000` em `0x1000` (GPION = `0x4006_4000`).

Todos usam **1 bit por pino** (bit *n* = pino *n*), exceto o `PCTL`, que usa 4.

| Offset | Registrador | O que faz |
|---|---|---|
| 0x000–0x3FC | `DATA` | valor dos pinos, **endereçado por máscara** (seção 5) |
| 0x400 | `DIR` | 1 = saída, 0 = entrada |
| 0x404–0x41C | `IS`, `IBE`, `IEV`, `IM`, `RIS`, `MIS`, `ICR` | interrupções dos pinos (módulo 08) |
| 0x420 | `AFSEL` | 1 = o pino é controlado por outro periférico (serial, PWM...) em vez do GPIO |
| 0x500 / 0x504 / 0x508 | `DR2R` / `DR4R` / `DR8R` | corrente máxima de saída: 2, 4 ou 8 mA (padrão 2 mA) |
| 0x50C | `ODR` | 1 = open-drain |
| 0x510 / 0x514 | `PUR` / `PDR` | resistor interno de pull-up / pull-down (módulo 08) |
| 0x518 | `SLR` | controle da velocidade das bordas (só com 8 mA) |
| 0x51C | **`DEN`** | 1 = função digital habilitada (**obrigatório** para usar o pino) |
| 0x520 / 0x524 | `LOCK` / `CR` | destrava pinos protegidos (módulo 08) |
| 0x528 | `AMSEL` | 1 = modo analógico (para o ADC, módulo 14) |
| 0x52C | `PCTL` | 4 bits por pino: **qual** periférico controla o pino quando `AFSEL` = 1 |
| 0x530 / 0x534 | `ADCCTL` / `DMACTL` | o pino pode disparar o ADC ou o DMA |

### Força de saída
Em vez de "velocidade", o TM4C escolhe a **corrente máxima** que o pino fornece: `DR2R`, `DR4R` ou `DR8R` (ligar o bit num deles desliga nos outros dois). Os LEDs da LaunchPad funcionam com os 2 mA do padrão.

---

## 7. O atraso por laço e o otimizador

O compilador otimiza o código (`-O2`) e elimina tudo que "não tem efeito". Um laço que só conta seria removido inteiro, e o LED piscaria rápido demais para ser visto. Duas coisas impedem isso:
- `volatile` no parâmetro obriga o compilador a realmente ler e escrever `n` a cada volta;
- `__asm volatile ("nop")` insere uma instrução que o compilador não pode remover.

O tempo desse atraso depende do clock e da otimização, então **ele não serve para medir tempo**: só para este primeiro exemplo. O módulo 06 o substitui por um temporizador de verdade (SysTick).

---

## 8. Pinos que exigem cuidado

| Chip | Pinos | Motivo |
|---|---|---|
| 123 e 129 | **PC0–PC3** | são os pinos de gravação e depuração (JTAG). Reconfigurá-los corta a conexão com o depurador |
| 123 | PF0, PD7 | protegidos (função NMI); só podem ser reconfigurados depois de destravar (`LOCK`/`CR`, módulo 08) |
| 129 | PD7, PE7 | idem |
| 123 | PB6/PB7 e PD0/PD1 | ligados entre si por resistores de 0 Ω na LaunchPad |

---

## 9. Comparando com o STM32 (para quem fez o outro curso)

| STM32 | TM4C |
|---|---|
| `RCC->AHB1ENR \|= GPIOAEN` | `SYSCTL_RCGCGPIO \|= 1` e esperar `SYSCTL_PRGPIO` |
| `MODER` = 01 | `DIR` = 1 **e** `DEN` = 1 |
| `BSRR = 1 << n` / `1 << (n + 16)` | `DATA_BITS[1 << n] = 0xFF` / `0x00` |
| `OSPEEDR` (velocidade) | `DR2R`/`DR4R`/`DR8R` (corrente) |
| `AFR` (4 bits por pino) | `AFSEL` + `PCTL` (4 bits por pino) |
| portas de 16 pinos | portas de 8 pinos |

---

## Exercícios

1. Mude o valor do `delay` e veja a frequência mudar. Depois pisque no padrão SOS do código Morse.
2. No 123, faça um ciclo de cores no LED RGB (vermelho, verde, azul, amarelo, ciano, magenta, branco) usando `DATA_BITS[0x0E]`.
3. Apague a linha do `DEN` e confirme que o LED não acende mais. Explique por quê.
4. No 129, pisque os quatro LEDs (PN1, PN0, PF4, PF0) como um contador binário.
5. Meça com um osciloscópio a frequência máxima do laço `while (1) GPIOF->DATA_BITS[0x08] ^= 0xFF;` com `DR2R` e com `DR8R`.
