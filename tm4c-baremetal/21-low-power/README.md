# Módulo 21 — Modos de baixo consumo

| Modo | Como entra | O que para | Como sai | Estado após |
|---|---|---|---|---|
| **Run** | — | — | — | — |
| **Sleep** | `WFI` com `SLEEPDEEP = 0` | só o clock da CPU | qualquer IRQ habilitada | continua após o WFI |
| **Deep-Sleep** | `WFI` com `SLEEPDEEP = 1` | PLL/MOSC; clock passa ao configurado em `DSLPCLKCFG`; só periféricos em `DCGCxxx` | IRQ de periférico com clock | continua; **clock de Run restaurado pelo hardware** |
| **Hibernação** | `HIB->CTL.HIBREQ` | tudo, exceto o módulo HIB | RTC (`RTCWEN`), pino WAKE, (129) GPIO/tamper | **reset** (só `HIB->DATA` sobrevive) |

Código: [`src/main.c`](src/main.c) — menu pela serial: `1` sleep, `2` deep-sleep, `3` hibernação. Acorda pelo botão (sleep/deep-sleep) ou pelo alarme do RTC após 5 s (hibernação).

```sh
make MCU=123 flash && picocom -b 115200 /dev/ttyACM0
```
Medir corrente: 123 → jumper **JP4** (I_MCU); 129 → jumper **JP2**. Troque o jumper por um amperímetro.

---

## 1. Sleep
```c
SysTick->CTRL &= ~TICKINT;        /* senão o tick acorda a cada 1 ms */
SCB->SCR &= ~SCB_SCR_SLEEPDEEP;
__WFI();
```
`SCGCxxx` (0x7xx) escolhe quais periféricos continuam com clock no Sleep (por padrão, os mesmos de Run — o comportamento depende de `SYSCTL_SYSPROP`/`ACG` no 123; consulte o datasheet).

## 2. Deep-Sleep
```c
SYSCTL_DSLPCLKCFG = (1 << 4);     /* DSOSCSRC = PIOSC */
SYSCTL_DCGCGPIO   = (1 << porta_do_botao);
SCB->SCR |= SCB_SCR_SLEEPDEEP;
__WFI();
```
| Reg. | |
|---|---|
| `DSLPCLKCFG` (0x144) | 6:4 DSOSCSRC (0 MOSC, 1 PIOSC, 3 LFIOSC...), 28:23 DSDIVORIDE (123) |
| `DCGCxxx` (0x8xx) | clock de cada periférico durante o deep-sleep |
| (129) `DSLPPWRCFG` | desliga SRAM/Flash parcialmente |

**Diferença importante do STM32**: no STM32 o Stop volta rodando do HSI e o software precisa religar o PLL. No TM4C, ao sair do deep-sleep o hardware **restaura sozinho** o clock de Run (PLL incluído) — mas o tempo de saída inclui o relock do PLL.

## 3. Hibernação
```c
hib_write(&HIB->DATA[1], MAGIC);              /* "estou hibernando de propósito" */
hib_write(&HIB->RTCM0, HIB->RTCC + 5);
hib_write(&HIB->CTL, HIB->CTL | RTCWEN);     /* acorda no alarme */
hib_write(&HIB->CTL, HIB->CTL | HIBREQ);     /* desliga */
```
Consumo típico: µA. Ao acordar, o chip passa por um **reset completo**; o programa reconhece a situação pelo magic em `HIB->DATA[1]` (o 129 também marca `RESC.HIB`).

Fontes de wake-up:
- `RTCWEN` — alarme do RTC
- `PINWEN` — pino **WAKE** (nível baixo; na LaunchPad 123 o pino WAKE vai a um header, no 129 está ligado a um botão "WAKE")
- (129) GPIOs com retenção e detector de tamper

## 4. Dicas gerais de consumo
- Pinos não usados: entrada com pull-down, ou saída em nível fixo — nunca flutuando.
- Desligue clocks de periféricos não usados (`RCGCxxx`).
- Reduza o SYSCLK quando possível (o 123 a 16 MHz sem PLL consome bem menos que a 80 MHz).
- O ICDI e os LEDs da LaunchPad consomem mais que o MCU em hibernação — meça só pelo jumper do MCU.

## 5. Depurando
Deep-sleep e hibernação desconectam o debugger. Se o chip hibernar logo após o reset em laço, a placa fica "inacessível": segure RESET ao conectar ou use `stellaris recover` (módulo 00).

---

## Exercícios
1. Meça a corrente em Run (80/120 MHz), Run a 16 MHz, Sleep, Deep-Sleep e Hibernação.
2. Deep-sleep acordando pela UART0: `DCGCUART`, UART com clock PIOSC (`CC = 5`) e interrupção de RX.
3. Data logger: hibernar 60 s, acordar, ler o sensor de temperatura (módulo 14), guardar em `HIB->DATA[]`, hibernar de novo.
