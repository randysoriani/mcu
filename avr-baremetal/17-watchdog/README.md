# Módulo 17 — Watchdog e causas de reset

> **Objetivo deste módulo:** usar o watchdog — um temporizador que **reinicia o chip** se o programa travar — e descobrir, no boot, **por que** o chip reiniciou.

Código:
- [`m328p/main.c`](m328p/main.c) — watchdog de 1 s; segure o botão (pino 2 ao GND) para "travar" o programa
- [`uc3a/main.c`](uc3a/main.c) — idem, com o botão do joystick (PA20)

```sh
make MCU=m328p flash && picocom -b 115200 /dev/ttyACM0
```
Saída esperada ao segurar o botão por mais de 1 s:
```
vivo (0)
vivo (5)
travado...
travado...
...
=== Modulo 17: watchdog ===
causa do reset: WATCHDOG (MCUSR = 0x08)
```

---

## 1. Para que serve

Um programa embarcado roda por anos sem ninguém olhando. Se travar (laço infinito, sensor que não responde, ponteiro errado, ruído elétrico), ninguém aperta o RESET. O **watchdog** ("cão de guarda") é um temporizador independente que conta para baixo; o programa precisa **zerá-lo periodicamente** ("alimentar o cão"). Se não fizer isso a tempo, o watchdog **reinicia o chip**.

Analogia: o "botão do homem morto" de um trem — se o maquinista não apertar de tempos em tempos, o trem freia sozinho.

```
alimenta  alimenta  alimenta     (travou)
   │         │         │
   ▼         ▼         ▼
 ──┴─────────┴─────────┴──────────────X RESET
              timeout = 1 s  ───────►
```

Boas práticas:
- Alimente num **único** lugar, no laço principal, depois de todas as tarefas: se qualquer uma travar, o reset acontece. Nunca alimente dentro de uma interrupção periódica (ela continua rodando mesmo com o laço principal travado).
- No boot, verifique a causa do reset e registre (EEPROM, serial).

---

## 2. ATmega328P

### 2.1 O watchdog
- Clock próprio: oscilador de **128 kHz** (independe do clock principal).
- Timeout de **16 ms a 8 s**.
- Três modos: **reset**, **interrupção** (`WDT_vect`) ou interrupção seguida de reset.
- Alimentar: instrução **`wdr`** (macro `wdt_reset()` do curso).

### 2.2 WDTCSR (0x60)
| Bit | Nome | Função |
|---|---|---|
| 7 | WDIF | flag da interrupção |
| 6 | WDIE | modo interrupção |
| 5, 2:0 | **WDP3:0** | timeout |
| 4 | **WDCE** | habilitação de mudança (sequência temporizada) |
| 3 | **WDE** | modo reset |

| WDP3:0 | 0000 | 0001 | 0010 | 0011 | 0100 | 0101 | **0110** | 0111 | 1000 | 1001 |
|---|---|---|---|---|---|---|---|---|---|---|
| Timeout | 16 ms | 32 ms | 64 ms | 125 ms | 250 ms | 0,5 s | **1 s** | 2 s | 4 s | 8 s |

### 2.3 A sequência temporizada
Como o CLKPR (módulo 04), o watchdog só aceita mudanças logo após uma "senha":
```c
cli();
wdt_reset();
WDTCSR = (1 << WDCE) | (1 << WDE);                   /* 1) destrava por 4 ciclos */
WDTCSR = (1 << WDE) | (1 << WDP2) | (1 << WDP1);     /* 2) modo reset, 1 s */
sei();
```

### 2.4 A armadilha do reset pelo watchdog
Depois de um reset causado pelo watchdog, ele **continua ligado**, agora com o **menor timeout (16 ms)**. Se o programa demorar mais que isso para chegar ao código que o desliga, entra num ciclo infinito de resets. Por isso, logo no início:
```c
MCUSR = 0;                                 /* o WDRF ligado FORÇA o WDE = 1: zere primeiro */
WDTCSR = (1 << WDCE) | (1 << WDE);
WDTCSR = 0;                                /* desliga */
```
(O fuse **WDTON** força o watchdog sempre ligado, sem possibilidade de desligar.)

Alguns bootloaders antigos do Arduino não fazem isso e entram em ciclo de reset após um watchdog com timeout curto; o optiboot do Uno trata o caso.

### 2.5 Causa do reset: MCUSR (0x54)
| Bit | Flag | Causa |
|---|---|---|
| 0 | **PORF** | ligou a energia (*power-on*) |
| 1 | **EXTRF** | pino RESET (botão) |
| 2 | **BORF** | queda de tensão (*brown-out*, se o BOD estiver ligado) |
| 3 | **WDRF** | watchdog |

As flags acumulam até serem zeradas: leia e zere no começo do programa.

> No Arduino, o bootloader roda antes do seu programa e pode alterar o `MCUSR` (o optiboot o zera e passa o valor original no registrador `r2`). Por isso, pelo bootloader, a causa pode aparecer como 0. Gravando por ISP (sem bootloader), as flags chegam intactas.

### 2.6 Watchdog como temporizador
Em modo interrupção (WDIE = 1, WDE = 0), o watchdog acorda o chip do **power-down** periodicamente (módulo 18): é a forma de fazer um ATmega acordar a cada 8 s consumindo microampères.

---

## 3. AT32UC3A

### 3.1 WDT (base `0xFFFF_0D30`)
| Offset | Registrador | Função |
|---|---|---|
| 0x00 | **CTRL** | EN (0); **PSEL** (12:8): timeout; **KEY** (31:24) |
| 0x04 | **CLR** | qualquer escrita alimenta o watchdog |

```
timeout = 2^(PSEL + 1) / f_RC         (f_RC ≈ 115 kHz)
PSEL = 16 → 131 072 / 115 000 ≈ 1,1 s
```

### 3.2 Escrita protegida
O `CTRL` só aceita uma configuração escrita **duas vezes seguidas**, a primeira com KEY = 0x55 e a segunda com KEY = 0xAA:
```c
WDT->CTRL = v | (0x55 << 24);
WDT->CTRL = v | (0xAA << 24);
```

### 3.3 Causa do reset: PM->RCAUSE (offset 0x140)
| Bit | Causa |
|---|---|
| 0 | POR (energia) |
| 1 | BOD (queda de tensão) |
| 2 | EXT (pino RESET) |
| 3 | **WDT** |
| 4 | JTAG |
| 7 | CPUERR (erro grave da CPU) |

Diferente do ATmega, o `RCAUSE` mostra **só a causa do último reset** e não precisa ser zerado.

---

## Exercícios
1. Mude o timeout para 125 ms e veja quanto tempo de botão apertado é preciso.
2. Coloque o `wdt_reset()` dentro de uma ISR de timer e mostre que o watchdog deixa de proteger contra um travamento no laço principal.
3. Use o modo interrupção+reset: na interrupção, grave na EEPROM em qual tarefa o programa estava, antes do reset.
4. Conte na EEPROM quantos resets por watchdog já ocorreram.
