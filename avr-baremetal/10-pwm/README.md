# Módulo 10 — PWM

> **Objetivo deste módulo:** gerar PWM — um sinal digital que simula uma tensão intermediária — para controlar o brilho de um LED e a posição de um servo, usando o hardware dos timers sem gastar CPU.

Código:
- [`m328p/main.c`](m328p/main.c) — Timer2 Fast PWM no pino 11 (LED "respirando"), Timer1 com TOP = ICR1 no pino 9 (servo a 50 Hz)
- [`uc3a/main.c`](uc3a/main.c) — bloco PWM: canal 0 em PB19 (LED) e canal 1 em PB20 (servo)

Montagem no Arduino: LED + resistor de 330 Ω entre o **pino 11** e o GND; servo com o fio de sinal no **pino 9** (alimentação do servo em 5 V separado se ele for grande; GND comum).

---

## 1. O que é PWM

**PWM** (*Pulse Width Modulation*): o pino alterna entre 0 e 1 rápido, numa frequência fixa, e o que muda é **quanto tempo** fica em 1 em cada período — o **duty cycle**.
```
 25%  ‾|___|‾|___|‾|___        média = 25% de 5 V = 1,25 V
 50%  ‾‾|__|‾‾|__|‾‾|__        média = 2,5 V
 75%  ‾‾‾|_|‾‾‾|_|‾‾‾|_        média = 3,75 V
      |<->| período
```
O olho (LED) e um motor respondem à **média**: o LED parece mais fraco ou mais forte. Analogia: acender e apagar uma lâmpada muito rápido; quanto mais tempo acesa, mais clara a sala parece.

Um **servo** é diferente: ele lê a **largura do pulso** (1 ms = um extremo, 1,5 ms = centro, 2 ms = outro extremo), repetido a cada 20 ms (50 Hz).

---

## 2. ATmega328P

### 2.1 Como o timer gera PWM
No modo **Fast PWM**, o contador sobe de 0 a TOP e recomeça. O pino de saída liga no começo e desliga quando o contador passa pelo `OCRnx`:
```
 TOP ┤   /|   /|          TCNT
OCR  ┤--/-|--/-|------    (comparação)
     │ /  | /  |
   0 ┼/───┴/───┴──
 pino ‾‾|_|‾‾|_|           liga no 0, desliga ao passar o OCR
```
Tudo é feito pelo hardware; o programa só escreve um novo `OCR` quando quer mudar o duty.

### 2.2 Bits COM: o que fazer com o pino
| COMnx1:0 | Fast PWM |
|---|---|
| 00 | pino desligado do timer (GPIO normal) |
| 10 | **não invertido**: liga no início, desliga na comparação |
| 11 | invertido |

O pino precisa estar como **saída** no `DDR`.

### 2.3 Timer2: LED (8 bits)
```c
DDRB  |= (1 << 3);                                        /* OC2A = PB3 = pino 11 */
TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20);     /* não invertido, Fast PWM (TOP = 0xFF) */
TCCR2B = (1 << CS22);                                     /* ÷64 */
OCR2A  = 64;                                              /* duty = (64 + 1) / 256 ≈ 25% */
```
`f = 16 MHz / (64 × 256) = 977 Hz`. O brilho percebido não é linear: o exemplo usa uma curva quadrática para a "respiração" parecer suave.

Detalhe: com `OCR = 0`, o Fast PWM ainda gera um pulso estreito de 1 contagem; para desligar de verdade, desligue o COM.

### 2.4 Timer1: servo (16 bits, TOP = ICR1)
O modo 14 usa o **ICR1 como TOP**, deixando o OCR1A livre para o duty:
```c
TCCR1A = (1 << COM1A1) | (1 << WGM11);                   /* WGM13:10 = 1110 */
TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);      /* ÷8 → 2 MHz = 0,5 µs por contagem */
ICR1   = 39999;                                          /* 40 000 × 0,5 µs = 20 ms */
OCR1A  = 3000;                                           /* 3000 × 0,5 µs = 1,5 ms: centro */
```
Resolução: 2000 passos entre 1 e 2 ms.

### 2.5 Modos PWM do ATmega328P
| Modo | Rampa | TOP | Observação |
|---|---|---|---|
| Fast PWM | só sobe | 0xFF, 0x1FF, 0x3FF, OCRA ou ICR1 | frequência dupla do phase correct |
| Phase Correct | sobe e desce | idem | pulso centrado: melhor para motores |
| Phase and Frequency Correct (Timer1) | sobe e desce | OCR1A ou ICR1 | troca de frequência sem glitch |

### 2.6 Os pinos PWM do Arduino
Pinos 3 e 11 (Timer2), 5 e 6 (Timer0 — **usado pelo `millis()` do curso**), 9 e 10 (Timer1).

---

## 3. AT32UC3A: o bloco PWM

Um periférico dedicado (base `0xFFFF_3000`) com **7 canais**, cada um com seu contador. Global:

| Offset | Registrador | Função |
|---|---|---|
| 0x00 | MR | dois divisores de clock extras (CLKA, CLKB) |
| 0x04 / 0x08 / 0x0C | **ENA / DIS / SR** | liga / desliga / estado dos canais (1 bit por canal) |

Por canal (base + 0x200 + 0x20 × n):

| Offset | Registrador | Função |
|---|---|---|
| 0x00 | **CMR** | CPRE (3:0): clock = PBA / 2^CPRE (ou CLKA/B); CALG (8): alinhamento central; **CPOL** (9): nível inicial; CPD (10): o que o CUPD atualiza |
| 0x04 | **CDTY** | duty (em contagens) |
| 0x08 | **CPRD** | período (em contagens) |
| 0x0C | CCNT | contador atual |
| 0x10 | **CUPD** | atualização: o valor entra no **fim do período** (sem glitch) |

```
CPRE = 5 → PBA/32 = 1,5 MHz
LED:   CPRD = 1500   → 1 kHz
servo: CPRD = 30 000 → 50 Hz; CDTY 1500–3000 → 1–2 ms
```
`CDTY` e `CPRD` só podem ser escritos **antes** de ligar o canal; depois, use o `CUPD`.

> Confira no esquemático da EVK1100 qual pino de LED está ligado a um canal PWM e na tabela de multiplexação do datasheet a função de cada pino; a polaridade (CPOL) também define se o duty conta o tempo em alto ou em baixo.

---

## Exercícios
1. Mude o Timer2 para Phase Correct (WGM = 001) e meça a nova frequência.
2. Controle o brilho pela serial (comando `pwm 0-100`).
3. Gere um tom de 440 Hz num buzzer usando o Timer1 em CTC com o pino em modo *toggle* (COM1A = 01).
4. No UC3, faça um LED RGB mudar de cor com três canais.
