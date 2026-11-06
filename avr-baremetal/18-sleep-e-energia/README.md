# Módulo 18 — Sleep e economia de energia

> **Objetivo deste módulo:** fazer o microcontrolador **dormir** quando não há nada a fazer e acordar com um evento, reduzindo o consumo de miliampères para microampères — o que decide se um aparelho a pilha dura dias ou anos.

Código:
- [`m328p/main.c`](m328p/main.c) — menu: `1` = IDLE, `2` = POWER-DOWN (acorda com o botão no pino 2); desliga periféricos com o PRR
- [`uc3a/main.c`](uc3a/main.c) — IDLE acordado pelo botão do joystick

---

## 1. Por que dormir

Um ATmega328P a 16 MHz e 5 V consome ~10 mA rodando. A maior parte dos programas passa quase todo o tempo **esperando** (um botão, o próximo segundo, um byte). Se o chip dorme durante a espera:

| Estado (ATmega328P, 5 V, valores típicos do datasheet) | Consumo |
|---|---|
| ativo a 16 MHz | ~10 mA |
| IDLE | ~2–3 mA |
| POWER-DOWN com watchdog | ~5 µA |
| POWER-DOWN sem watchdog e sem BOD | ~0,1 µA |

Com pilhas de 2000 mAh: 10 mA duram ~8 dias; 5 µA duram décadas (a autodescarga da pilha vence antes).

> **No Arduino Uno a placa consome ~45 mA** (regulador, conversor USB, LED de energia): o sleep do chip quase não muda o total. Para medir de verdade, use um ATmega328P numa protoboard ou um Pro Mini sem o LED e o regulador.

Analogia: modos de sleep são como os estados de um celular — tela apagada (idle), modo avião (standby), desligado mas com despertador (power-down + watchdog).

---

## 2. ATmega328P

### 2.1 Modos (SMCR, 0x53)
| SM2:0 | Modo | O que para | O que acorda |
|---|---|---|---|
| 000 | **IDLE** | só a CPU | **qualquer** interrupção |
| 001 | ADC Noise Reduction | CPU e E/S (o ADC converte em silêncio) | ADC, externas, TWI, Timer2, WDT |
| 010 | **POWER-DOWN** | **todos os clocks**, inclusive o oscilador | INT0/1 por **nível**, **PCINT**, TWI (endereço), **watchdog**, reset |
| 011 | Power-save | idem, mas o Timer2 assíncrono continua | + Timer2 (cristal de 32 kHz) |
| 110 | Standby | como power-down, oscilador ligado | idem (acorda em 6 ciclos) |

Bit **SE** (bit 0): habilita a instrução `SLEEP`.

### 2.2 Entrando no sleep
```c
SMCR = (2 << SM0) | (1 << SE);   /* power-down, habilitado */
sei();                           /* precisa haver interrupção para acordar */
sleep_cpu();                     /* instrução SLEEP: o chip para aqui */
SMCR = 0;                        /* acordou: a ISR já rodou; desabilita o sleep */
```
Ao acordar, a CPU executa a ISR e continua **na instrução seguinte ao SLEEP**.

Cuidados que o exemplo mostra:
1. **Espere a serial terminar** (`TXC0`): no power-down o clock para, e o último byte sairia cortado.
2. **Desligue o tick de 1 ms** antes do IDLE: senão o Timer0 acorda o chip a cada milissegundo e o sleep não serve para nada.
3. **Condição de corrida**: se a interrupção esperada chegar entre a decisão de dormir e o `SLEEP`, o chip dorme para sempre. O truque: `sei()` imediatamente seguido de `sleep` — o AVR garante que a instrução depois do `sei` sempre executa antes de qualquer interrupção.
4. Ao acordar do power-down, o oscilador de cristal leva até 16 000 ciclos (1 ms) para estabilizar (fuses SUT/CKSEL).

### 2.3 PRR: desligando periféricos (0x64)
Cada bit 1 **corta o clock** de um periférico (e ele para de funcionar):

| Bit | 7 | 6 | 5 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|
| | PRTWI | PRTIM2 | PRTIM0 | PRTIM1 | PRSPI | PRUSART0 | PRADC |

O exemplo desliga TWI, SPI, ADC, Timer1 e Timer2. Também economizam:
- `ACSR |= (1 << ACD)`: desliga o comparador analógico;
- `ADCSRA = 0` antes do PRADC (o ADC ligado consome ~300 µA);
- **BOD** desligado durante o sleep (bits BODS/BODSE do MCUCR) ou pelo fuse;
- pinos sem uso como entrada com pull-up (ou saída), nunca flutuando: uma entrada flutuante oscila e consome.

---

## 3. AT32UC3A

### 3.1 Modos (instrução `sleep N`)
| N | Modo | O que para | O que acorda |
|---|---|---|---|
| 0 | **IDLE** | CPU | qualquer interrupção |
| 1 | FROZEN | CPU e barramento HSB | interrupções do PBA/PBB |
| 2 | STANDBY | todos os clocks síncronos (osciladores ligados) | interrupções assíncronas (EIC, RTC...) |
| 3 | STOP | + osciladores principais | idem |
| 5 | DEEPSTOP | + regulador em baixa | idem |
| 6 | STATIC | tudo, exceto o RC de 32 kHz | EIC, RTC, reset |

No curso: `cpu_sleep(0)` (macro de [`uc3a.h`](../common/uc3a/uc3a.h)). Diferente do AVR8, o sleep do UC3 **reabilita as interrupções globais** automaticamente ao dormir.

### 3.2 Máscaras de clock do PM
`PM->CPUMASK`, `HSBMASK`, `PBAMASK`, `PBBMASK`: um bit por periférico; 0 corta o clock. No reset, tudo ligado. O exemplo imprime o `PBAMASK`.

### 3.3 O tick
O COUNT/COMPARE (módulo 06) gera interrupção a cada 1 ms e acordaria o IDLE. O exemplo escreve 0 no COMPARE (desliga) antes de dormir e chama `tick_init()` ao acordar.

---

## Exercícios
1. Meça o consumo (multímetro em série com a alimentação) de um ATmega328P avulso nos modos ativo, IDLE e POWER-DOWN.
2. Use o watchdog em modo interrupção para acordar do power-down a cada 8 s, piscar o LED e dormir de novo (módulo 17).
3. Reduza o clock com o CLKPR (módulo 04) e meça o consumo ativo.
4. No UC3, use o modo STATIC acordando pelo EIC.
