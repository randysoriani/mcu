# Módulo 05 — GPIO como saída: o "Hello World" (piscar um LED)

> **Objetivo deste módulo:** fazer o primeiro programa com efeito visível e entender, linha por linha, como um número escrito num registrador vira tensão num pino. O AVR de 8 bits tem um dos GPIOs mais simples que existem: três registradores por porta.

Código (autocontido):
- [`m328p/main.c`](m328p/main.c) — pisca o LED "L" do Arduino Uno (pino digital 13 = **PB5**)
- [`uc3a/main.c`](uc3a/main.c) — pisca o LED0 da EVK1100 (**PB27**, aceso em nível baixo)

```sh
make MCU=m328p flash
make MCU=uc3a flash
```

---

## 1. O que é GPIO
**GPIO** (*General Purpose Input/Output*) é o periférico que controla os pinos como sinais digitais:
- **saída**: o programa põe o pino em **1** (tensão de alimentação: 5 V no Uno, 3,3 V na EVK1100) ou **0** (0 V);
- **entrada**: o programa lê se a tensão no pino está alta ou baixa (módulo 08).

### Nomes dos pinos
Os pinos são agrupados em **portas**. No ATmega328P, portas de 8 pinos: **B** (PB0–PB7), **C** (PC0–PC6) e **D** (PD0–PD7). No UC3A, portas de 32 pinos: PA, PB, PC, PX.

**Os números do Arduino não são os do chip.** O "pino 13" do Arduino é o **PB5** do ATmega328P:

| Arduino | Chip | | Arduino | Chip |
|---|---|---|---|---|
| D0, D1 | PD0, PD1 (serial) | | D8–D13 | PB0–PB5 |
| D2–D7 | PD2–PD7 | | A0–A5 | PC0–PC5 |

### O LED do Arduino
```
PB5 ──[resistor]──►|── GND        PB5 = 1: acende      PB5 = 0: apaga
```
(Na verdade, no Uno R3 o LED é acionado por um amplificador ligado ao PB5, mas a lógica é a mesma.)

---

## 2. ATmega328P: DDR, PORT e PIN

Cada porta tem **três registradores de 8 bits**, um bit por pino:

| Registrador (porta B) | Endereço | Função |
|---|---|---|
| **DDRB** (*Data Direction*) | 0x24 | **1 = saída**, 0 = entrada |
| **PORTB** | 0x25 | numa **saída**: o valor (1 = alto). Numa **entrada**: 1 liga o resistor de pull-up interno |
| **PINB** | 0x23 | **leitura** do pino. Escrever 1 num bit **inverte** o pino |

Portas C e D: `PINC`/`DDRC`/`PORTC` em 0x26–0x28, `PIND`/`DDRD`/`PORTD` em 0x29–0x2B.

### O programa, linha por linha
```c
DDRB |= (1U << 5);              /* PB5 vira saída (bit 5 do DDRB = 1) */

for (;;) {                      /* laço infinito: o programa nunca termina */
    PORTB |= (1U << 5);         /* PB5 = 1: LED aceso */
    delay(200000);
    PORTB &= ~(1U << 5);        /* PB5 = 0: LED apagado */
    delay(200000);
}
```
- `1U << 5` = `0b00100000` = `0x20`: um número com só o bit 5 ligado (a "máscara").
- `|=` liga o bit 5 sem mexer nos outros; `&= ~(...)` desliga só ele.

**Sem "ligar o clock":** diferente do STM32/TM4C, no AVR de 8 bits o GPIO está sempre funcionando. Basta escrever no `DDR`.

### Como um bit vira tensão
```
            VDD (5 V)
              │
          [chave de cima] ◄─┐
 PORTB5 ──► controle ───────●── pino PB5
          [chave de baixo]◄─┘         │
              │                 PINB5 ◄┘ (leitura)
             GND
```
Com `DDRB5 = 1`, escrever 1 no `PORTB5` fecha a chave de cima (pino em 5 V); 0 fecha a de baixo (0 V).

### O que o compilador gera
```c
PORTB |= (1U << 5);    /* vira: sbi 0x05, 5   (1 instrução, 2 ciclos) */
```
Os registradores de 0x20 a 0x3F podem ser mexidos bit a bit pelas instruções `SBI`/`CBI` (o endereço curto `0x05` é `0x25 − 0x20`). É uma operação **atômica**: uma interrupção não pode acontecer no meio dela. Confira com `make MCU=m328p disasm`.

### Inverter com o PIN
```c
PINB = (1U << 5);      /* escrever 1 no PINB5 inverte o PB5 */
```
É o jeito mais rápido de alternar um pino no ATmega328P (usado em `led_toggle()` do [`board.h`](../common/m328p/board.h)). Atenção: só os bits escritos com 1 invertem.

### Corrente máxima
Cada pino do ATmega328P fornece até ~20 mA com conforto (40 mA é o máximo absoluto, que não deve ser usado), e há limites por grupo de pinos. LEDs sempre com resistor (220–1 kΩ a 5 V).

---

## 3. AT32UC3A: o GPIO de 32 bits

Cada porta de 32 pinos é um bloco de registradores em `0xFFFF_1000 + 0x100 × porta`. O pino PB27 é o GPIO número 32 + 27 = **59**: porta 1, bit 27.

Quase todo registrador tem **quatro endereços**:

| Endereço | Efeito da escrita |
|---|---|
| registrador (`VAL`) | escreve o valor inteiro |
| + 0x4 (`SET`) | bits em 1 **ligam**, bits em 0 não mudam nada |
| + 0x8 (`CLR`) | bits em 1 **desligam** |
| + 0xC (`TGL`) | bits em 1 **invertem** |

Assim um pino é alterado numa única escrita, sem ler o registrador antes.

| Offset | Registrador | Função |
|---|---|---|
| 0x00 | **GPER** | 1 = o pino é controlado pelo GPIO; 0 = por um periférico (serial, PWM...) |
| 0x10 / 0x20 | PMR0 / PMR1 | qual periférico (função A, B ou C) |
| 0x40 | **ODER** | 1 = saída |
| 0x50 | **OVR** | valor de saída |
| 0x60 | PVR | leitura do pino |
| 0x70 | PUER | pull-up |
| 0x80 | ODMER | open-drain |
| 0x90–0xD0 | IER, IMR0/1, GFER, IFR | interrupções (módulo 08) |

### O programa, linha por linha
```c
GPIO_Port *p = GPIO_PORT(PB(27));   /* 0xFFFF_1100: bloco da porta B */
uint32_t   m = GPIO_MASK(PB(27));   /* 1 << 27 */

p->GPER.SET = m;     /* 1) o GPIO assume o pino */
p->OVR.SET  = m;     /* 2) valor inicial 1 = LED APAGADO (o LED da EVK1100 acende com 0) */
p->ODER.SET = m;     /* 3) saída */

for (;;) {
    p->OVR.TGL = m;  /* inverte o pino */
    delay(3000);
}
```
O LED da EVK1100 vai do 3,3 V ao pino: **acende com nível baixo** ("ativo em baixo"). Por isso o valor inicial é 1.

O programa roda com o clock de reset (~115 kHz), por isso o atraso é pequeno.

---

## 4. Comparando

| | ATmega328P | AT32UC3A | STM32 / TM4C |
|---|---|---|---|
| Ligar clock do GPIO | não precisa | não precisa (ligado no reset) | precisa |
| Direção | `DDRx` | `ODER` | `MODER` / `DIR` |
| Saída | `PORTx` | `OVR` (+ SET/CLR/TGL) | `ODR`, `BSRR` / `DATA` mascarado |
| Pull-up | `PORTx` com o pino em entrada | `PUER` | `PUPDR` / `PUR` |
| Pinos por porta | 8 | 32 | 16 / 8 |

---

## Exercícios
1. Pisque no padrão SOS do código Morse.
2. Use `PINB = (1 << 5)` para inverter o LED e compare o tamanho do código com `PORTB ^= (1 << 5)` (`make size`).
3. Ligue um LED com resistor no pino 8 (PB0) e pisque os dois LEDs alternados com uma única escrita no `PORTB`.
4. No UC3, pisque os LEDs LED0 a LED3 (PB27–PB30) como um contador binário.
