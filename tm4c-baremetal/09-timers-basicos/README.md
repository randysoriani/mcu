# Módulo 09 — GPTM: timers básicos

O **General-Purpose Timer Module** do TM4C é diferente dos TIMx do STM32: cada bloco tem dois timers de 16 bits (**A** e **B**) que podem ser **concatenados em um de 32 bits**. PWM "de verdade" fica num módulo separado (módulo 10) e encoders também (QEI, módulo 11).

| | TM4C123 | TM4C129 |
|---|---|---|
| Timers 16/32 bits | TIMER0–5 | TIMER0–7 |
| Timers "wide" 32/64 bits | WTIMER0–5 | — |
| Clock | SYSCLK (80 MHz) | SYSCLK ou ALTCLK (`GPTMCC`) |

Código: [`src/main.c`](src/main.c)
- **TIMER0**: 32 bits periódico, interrupção a 2 Hz → pisca o LED
- **TIMER2**: 32 bits livre, **contando para cima** → base de tempo em ciclos
- **TIMER1**: 32 bits **one-shot** → `timer1_delay_us()`; o TIMER2 mede o atraso real

```sh
make MCU=123 flash && picocom -b 115200 /dev/ttyACM0
```

---

## 1. Registradores principais (base TIMERn)

| Offset | Reg. | Descrição |
|---|---|---|
| 0x000 | `CFG` | **0 = 32 bits** (A+B concatenados), 1 = RTC, **4 = 16 bits** (A e B separados) |
| 0x004 | `TAMR` | modo do timer A (abaixo) |
| 0x008 | `TBMR` | modo do timer B |
| 0x00C | `CTL` | 0 **TAEN**, 1 TASTALL, 3:2 TAEVENT, 4 RTCEN, 5 TAOTE (dispara ADC), 6 TAPWML, 8 TBEN... |
| 0x018 | `IMR` | 0 **TATOIM** (timeout), 1 CAMIM, 2 CAEIM, 4 TAMIM (match), 8 TBTOIM... |
| 0x01C/0x020 | `RIS`/`MIS` | |
| 0x024 | `ICR` | escreve 1 → limpa |
| 0x028 | `TAILR` | valor de recarga (interval load) |
| 0x030 | `TAMATCHR` | comparação |
| 0x038 | `TAPR` | prescaler (só em modo 16 bits!) |
| 0x048 | `TAR` | valor do timer / valor capturado |
| 0x050 | `TAV` | contador "vivo" (pode ser escrito) |
| 0x06C | `DMAEV` | (129) eventos que disparam uDMA |
| 0x070 | `ADCEV` | (129) eventos que disparam o ADC |

### `TAMR`
| Bits | Nome | |
|---|---|---|
| 1:0 | TAMR | 1 one-shot, **2 periódico**, 3 captura |
| 2 | TACMR | captura: 0 edge-count, 1 edge-time |
| 3 | TAAMS | 1 = modo PWM |
| 4 | **TACDIR** | 0 conta para baixo, 1 para cima |
| 5 | TAMIE | interrupção de match |
| 8 | TAILD | atualiza TAILR só no próximo timeout |

## 2. Contas

Modo 32 bits **não tem prescaler**: 1 tick = 1 ciclo de SYSCLK.
```
TAILR = SYSCLK / f - 1         (2 Hz no 123: 39 999 999)
```
32 bits a 80 MHz = 53,7 s de alcance; a 120 MHz = 35,8 s.

Modo 16 bits com prescaler de 8 bits (`TAPR`): até 24 bits efetivos. Em modo captura, o prescaler vira **extensão** dos bits [23:16] do contador (usado no módulo 11).

## 3. Sequência
```c
SYSCTL_RCGCTIMER |= 1;  while (!(SYSCTL_PRTIMER & 1)) { }
TIMER0->CTL   = 0;                    /* desliga */
TIMER0->CFG   = 0;                    /* 32 bits */
TIMER0->TAMR  = 2;                    /* periódico, para baixo */
TIMER0->TAILR = SystemCoreClock/2 - 1;
TIMER0->ICR   = 1;                    /* limpa TATO */
TIMER0->IMR   = 1;                    /* TATOIM */
TIMER0->CTL   = TAEN | TASTALL;
NVIC_EnableIRQ(TIMER0A_IRQn);         /* em 32 bits, a IRQ é a do timer A */

void TIMER0A_Handler(void) { TIMER0->ICR = 1; ... }
```

`TASTALL`: o timer para quando o debugger para a CPU (senão um breakpoint vira uma enxurrada de interrupções).

## 4. One-shot
`TAMR = 1`: conta até 0, seta TATORIS e **desliga sozinho** (TAEN volta a 0). O exemplo espera por polling em `RIS`, sem interrupção.

## 5. Contando para cima
`TACDIR = 1` com `TAILR = 0xFFFFFFFF` dá um contador livre crescente; diferenças `b - a` em `uint32_t` tratam o overflow sozinhas.

## 6. Outros modos (não usados aqui)
- **RTC**: `CFG = 1`, conta 1 Hz a partir do pino 32 kHz (CCP0).
- **Wait-for-trigger** (`TAWOT`) e encadeamento entre timers.
- **Disparo do ADC** (`TAOTE`) — amostragem a taxa fixa sem CPU.

---

## Exercícios
1. Divida o TIMER0 em dois de 16 bits (`CFG = 4`) com prescaler: A a 10 Hz e B a 1 Hz.
2. Use o TIMER1 one-shot com interrupção para apagar o LED 300 ms após um clique (módulo 08).
3. No 123, use o WTIMER0 em 64 bits (`CFG = 0` num wide timer) como contador livre que nunca dá a volta na prática.
