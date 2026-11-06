# Módulo 09 — Timers

> **Objetivo deste módulo:** entender o que é um timer (um contador que avança sozinho com o clock), os modos normal e CTC, o prescaler, e calcular frequências exatas. O ATmega328P tem três timers diferentes entre si; o UC3A tem o bloco TC com três canais de 16 bits.

Código:
- [`m328p/main.c`](m328p/main.c) — Timer1 (16 bits) a 2 Hz pisca o LED; Timer2 (8 bits) a 1 kHz
- [`uc3a/main.c`](uc3a/main.c) — canal 0 do TC a 10 Hz; canal 1 livre a 24 MHz mede o `delay_us()`

> Este módulo **não roda no QEMU**: com `-DQEMU` a base de tempo da biblioteca passa a usar o Timer1, o mesmo que o exemplo usa (o linker reclama de `__vector_11` duplicado).

---

## 1. O que é um timer

Um registrador contador que **soma 1 a cada pulso** do seu clock, sem a CPU. Quando chega a um valor escolhido ou estoura, sinaliza (flag, interrupção, mudança num pino).

Analogia: um cronômetro de cozinha que toca sozinho enquanto você faz outra coisa.

```
 contador
  TOP ┤      /|      /|      /|
      │    /  |    /  |    /  |      cada "dente" = um período
      │  /    |  /    |  /    |
    0 ┼/──────┴/──────┴/──────┴──► tempo
              ▲       ▲       ▲   evento (flag / interrupção)
```

### Prescaler
Divide o clock antes do contador, para contar devagar: a 16 MHz um contador de 8 bits estoura em 16 µs; com ÷1024, em 16,4 ms.

### A fórmula
```
f_evento = f_clock / (prescaler × (TOP + 1))
```

---

## 2. ATmega328P: três timers

| | Timer0 | Timer1 | Timer2 |
|---|---|---|---|
| Bits | 8 | **16** | 8 |
| Prescaler | 1, 8, 64, 256, 1024 | 1, 8, 64, 256, 1024 | 1, 8, **32**, 64, **128**, 256, 1024 |
| Clock externo | pino T0 (PD4) | pino T1 (PD5) | cristal de 32 kHz (modo assíncrono) |
| Captura | — | **ICP1** (PB0) | — |
| Saídas PWM | OC0A (PD6), OC0B (PD5) | OC1A (PB1), OC1B (PB2) | OC2A (PB3), OC2B (PD3) |
| Uso no curso | base de tempo `millis()` | — | — |

### 2.1 Registradores (Timer n = 0, 1, 2)
| Registrador | Função |
|---|---|
| **TCNTn** | o contador (Timer1: 16 bits, TCNT1H:L) |
| **OCRnA / OCRnB** | valores de comparação A e B |
| **TCCRnA** | COMnA/COMnB (o que fazer no pino ao comparar), WGM bits baixos |
| **TCCRnB** | WGM bits altos, **CSn2:0** (prescaler: 0 = parado) |
| **TIMSKn** | habilita interrupções: TOIE (estouro), OCIEA, OCIEB, ICIE (captura, Timer1) |
| **TIFRn** | flags; limpa escrevendo 1 (ou automaticamente ao entrar na ISR) |
| ICR1 | registrador de captura do Timer1 (ou TOP em alguns modos) |

### 2.2 Modos (WGM: *Waveform Generation Mode*)
| Modo | Conta até | Uso |
|---|---|---|
| **Normal** | o máximo (0xFF ou 0xFFFF), estoura e volta a 0 | medir tempo, contar |
| **CTC** | **OCRnA** (ou ICR1) e volta a 0 | **frequência exata**, interrupções periódicas |
| Fast PWM | TOP, rampa só subindo | PWM (módulo 10) |
| Phase Correct PWM | sobe até TOP e desce | PWM simétrico (motores) |

### 2.3 Os valores do exemplo
**Timer1 a 2 Hz:**
```
16 MHz / 256 = 62 500 Hz
62 500 / 2 Hz = 31 250 contagens → OCR1A = 31 249
TCCR1B = (1 << WGM12) | (1 << CS12);    /* CTC (modo 4), ÷256 */
```
Um timer de 8 bits não conseguiria (máximo 1024 × 256 = 262 144 ciclos ≈ 61 Hz no mínimo).

**Timer2 a 1 kHz:** `16 MHz / 64 / 250 = 1000` → OCR2A = 249, `TCCR2B = (1 << CS22)`. Atenção: **no Timer2 o código de prescaler é diferente** (CS22 sozinho = ÷64; no Timer0/1 seria ÷256).

### 2.4 Escrevendo registradores de 16 bits
O AVR lê 8 bits por vez. Para o Timer1 há um registrador temporário escondido (TEMP): ao **escrever**, o byte **alto vai primeiro**; ao **ler**, o **baixo primeiro**. O compilador faz isso certo com `OCR1A = 31249;`, mas uma interrupção que também acesse registradores de 16 bits do Timer1 no meio pode corromper o TEMP: proteja a leitura com `cli()` (o exemplo faz isso para ler `TCNT1`).

---

## 3. AT32UC3A: o bloco TC

Um bloco (base `0xFFFF_3800`) com **3 canais** de 16 bits iguais. Cada canal (offset 0x40 × n):

| Offset | Registrador | Função |
|---|---|---|
| 0x00 | **CCR** | CLKEN (0), CLKDIS (1), **SWTRG** (2): zera e inicia |
| 0x04 | **CMR** | modo (veja abaixo) |
| 0x10 | CV | valor atual do contador |
| 0x14 / 0x18 | RA / RB | registradores A e B (comparação ou captura) |
| 0x1C | **RC** | comparação C (TOP no modo "sobe até RC") |
| 0x20 | **SR** | flags: COVFS (0), LDRAS (5), LDRBS (6), **CPCS** (4: CV == RC)... **ler limpa** |
| 0x24 / 0x28 / 0x2C | IER / IDR / IMR | interrupções |

### CMR no modo "waveform" (WAVE = 1, bit 15)
| Campo | Bits | Valores |
|---|---|---|
| TCCLKS | 2:0 | 0 = 32 kHz, 1 = PBA/2, 2 = PBA/8, 3 = PBA/32, 4 = PBA/128 |
| WAVSEL | 14:13 | `00` sobe até 0xFFFF; `10` **sobe até RC** e recomeça (equivale ao CTC) |
| ACPA/ACPC... | 17:16, 19:18 | o que fazer no pino TIOA ao comparar com RA / RC (módulo 10) |

### O exemplo
```
canal 0: PBA/128 = 48 MHz / 128 = 375 kHz; RC = 37 500 → 10 Hz
canal 1: PBA/2 = 24 MHz, livre: mede delay_us() em ciclos
```
O TC é o **grupo 15** do INTC (linhas 0, 1, 2 = canais).

---

## Exercícios
1. Faça o Timer1 gerar 1 Hz com ÷1024 (calcule o OCR1A). É exato?
2. Use o modo normal do Timer2 com interrupção de estouro e meça a frequência resultante.
3. Calcule a menor e a maior frequência possível com o Timer1 em CTC.
4. No UC3, use o canal 2 com o clock de 32 kHz para contar segundos.
