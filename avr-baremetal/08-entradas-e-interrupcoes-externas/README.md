# Módulo 08 — Entradas digitais e interrupções externas

> **Objetivo deste módulo:** ler botões e sinais digitais de forma confiável — com pull-up, debounce e interrupções — em vez de perguntar o estado do pino o tempo todo.

Código:
- [`m328p/main.c`](m328p/main.c) — **INT0** no pino 2 (borda de descida) e **PCINT** (mudança de pino) no pino 8
- [`uc3a/main.c`](uc3a/main.c) — interrupção do GPIO no botão do joystick (PA20)

Montagem no Arduino: um botão entre o **pino 2** e o **GND**; um fio do **pino 8** que você encosta no GND.

---

## 1. Ler um pino

### 1.1 O pino "flutuando"
Um pino de entrada desligado de tudo **não** vale 0: ele capta ruído e lê valores aleatórios. Um botão só liga o pino ao GND quando apertado; solto, o pino ficaria flutuando. A solução é o **resistor de pull-up**: puxa o pino para 1 quando nada mais o força.
```
 VCC ──[pull-up ~30-50 kΩ]──┬── pino ─► lê 1 (solto) ou 0 (apertado)
                            │
                         [botão]
                            │
                           GND
```
O botão funciona "ao contrário": **apertado = 0** (ativo em baixo).

### 1.2 Bounce (trepidação)
Os contatos de um botão quicam por alguns milissegundos ao fechar e abrir:
```
 ideal:   ‾‾‾‾‾‾|____________
 real:    ‾‾‾‾‾‾|_|‾|_||‾|___________     <- 1 aperto, várias bordas
```
Sem tratamento, um aperto conta como vários. **Debounce** por software: depois de aceitar uma borda, ignore as seguintes por ~50 ms (é o que os exemplos fazem com `millis()`). Um capacitor de 100 nF no botão também ajuda.

### 1.3 Polling × interrupção
- *Polling*: ler o pino no laço. Simples, mas pode perder pulsos curtos se o laço demora.
- Interrupção: o hardware detecta a borda e chama a ISR. Também é a forma de **acordar o chip do sleep** (módulo 18).

---

## 2. ATmega328P

### 2.1 Pino como entrada
```c
DDRD  &= ~(1 << 2);       /* PD2 entrada */
PORTD |=  (1 << 2);       /* com o pino em entrada, PORT = 1 liga o pull-up */
if (!(PIND & (1 << 2))) { /* apertado */ }
```
| `DDRxn` | `PORTxn` | Pino |
|---|---|---|
| 0 | 0 | entrada flutuante |
| 0 | 1 | entrada com pull-up |
| 1 | 0 | saída em 0 |
| 1 | 1 | saída em 1 |

O bit **PUD** do `MCUCR` desliga todos os pull-ups de uma vez.

### 2.2 INT0 e INT1: interrupções externas completas
Só dois pinos: **INT0 = PD2** (pino 2) e **INT1 = PD3** (pino 3). Escolhem o tipo de evento:

| Registrador | Bits |
|---|---|
| **EICRA** (0x69) | ISC01:ISC00 (INT0) e ISC11:ISC10 (INT1): `00` nível baixo, `01` qualquer mudança, `10` **descida**, `11` subida |
| **EIMSK** (0x3D) | INT0 (bit 0), INT1 (bit 1): habilita |
| **EIFR** (0x3C) | INTF0, INTF1: pendente. **Limpa escrevendo 1** |

```c
EICRA = (1 << ISC01);     /* descida */
EIFR  = (1 << INTF0);     /* descarta um pedido antigo */
EIMSK = (1 << INT0);
sei();
ISR(INT0_vect) { ... }
```
No sleep profundo (power-down), só o modo **nível baixo** consegue acordar o chip.

### 2.3 PCINT: mudança em qualquer pino
**Todos** os pinos têm interrupção de mudança (*Pin Change*), agrupados por porta:

| Grupo | Pinos | Vetor | Máscara |
|---|---|---|---|
| PCINT0..7 | PB0–PB7 | `PCINT0_vect` | **PCMSK0** (0x6B) |
| PCINT8..14 | PC0–PC6 | `PCINT1_vect` | **PCMSK1** (0x6C) |
| PCINT16..23 | PD0–PD7 | `PCINT2_vect` | **PCMSK2** (0x6D) |

**PCICR** (0x68) habilita os grupos (bits 0, 1, 2); **PCIFR** (0x3B) tem as flags.

Limitações: dispara nas **duas bordas**, e todos os pinos de um grupo usam o **mesmo vetor**. A ISR precisa ler o `PINx` e descobrir o que mudou, comparando com a leitura anterior. Em troca, funciona em qualquer pino e acorda do power-down.

### 2.4 Atomicidade
`g_pcint_count` tem 16 bits: o laço principal lê com as interrupções desligadas (módulo 06).

---

## 3. AT32UC3A

### 3.1 Entrada
```c
p->GPER.SET = m;          /* GPIO controla o pino */
p->ODER.CLR = m;          /* entrada */
p->PUER.SET = m;          /* pull-up */
if (!(p->PVR.VAL & m))    /* apertado (ativo em baixo) */
```

### 3.2 Interrupção de pino
No UC3A, **todo pino** do GPIO pode gerar interrupção, configurada no próprio GPIO:

| Offset | Registrador | Função |
|---|---|---|
| 0x90 | **IER** | 1 = interrupção habilitada no pino |
| 0xA0 / 0xB0 | **IMR0 / IMR1** | modo: `00` qualquer mudança, `01` subida, `10` **descida** (IMR1 = 1, IMR0 = 0) |
| 0xC0 | **GFER** | filtro de glitch: ignora pulsos menores que alguns ciclos |
| 0xD0 | **IFR** | flags; limpa escrevendo 1 em `IFR.CLR` (IFRC) |

No INTC, o GPIO é o **grupo 2**, com uma linha para cada 8 pinos. A ISR verifica o `IFR` para saber qual pino disparou.

(O UC3A também tem o **EIC**, grupo 1, com 8 pinos de interrupção externa que funcionam até no sleep profundo.)

---

## Exercícios
1. Troque `ISC01:00` para `01` (qualquer mudança) e conte apertos e solturas.
2. Retire o debounce e conte quantas bordas um aperto gera.
3. Use PCINT nos 6 pinos da porta C (A0–A5) e imprima qual mudou.
4. No UC3, use os quatro sentidos do joystick (PA25–PA28) e mostre a direção.
