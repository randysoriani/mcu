# Módulo 19 — Watchdog

Os TM4C têm dois watchdogs iguais:
- **WDT0** — clock = SYSCLK
- **WDT1** — clock = PIOSC (independente do clock principal); toda escrita exige esperar `CTL.WRC`

Os dois são **de dois estágios**: o primeiro timeout gera uma **interrupção** (ou NMI); se ela não for atendida até o segundo timeout, vem o **reset**. É diferente do STM32 (IWDG que só reseta + WWDG com janela).

Código: [`src/main.c`](src/main.c)
- **DEMO 1**: 500 ms por estágio; o laço alimenta a cada 200 ms. **Segure o botão** (SW1 / USR_SW1): o watchdog dispara e o chip reseta em ~1 s, e no boot seguinte imprime "WATCHDOG0".
- **DEMO 2**: nunca alimenta; a ISR do 1º estágio imprime um aviso e o reset vem 500 ms depois.

```sh
make MCU=123 flash && picocom -b 115200 /dev/ttyACM0
make clean && make MCU=129 EXTRA=-DDEMO=2 flash
```

---

## 1. Registradores (base WDTn)

| Offset | Reg. | |
|---|---|---|
| 0x000 | `LOAD` | valor de recarga (32 bits) |
| 0x004 | `VALUE` | contador atual (decrescente) |
| 0x008 | `CTL` | 0 **INTEN** (liga o watchdog — só sai com reset), 1 **RESEN**, 2 INTTYPE (1 = NMI), 31 WRC (WDT1) |
| 0x00C | `ICR` | **qualquer escrita** limpa a interrupção e recarrega o contador |
| 0x010/0x014 | `RIS`/`MIS` | |
| 0x418 | `TEST` | 8 **STALL**: para quando o debugger para a CPU |
| 0xC00 | `LOCK` | escrever `0x1ACCE551` destrava; qualquer outro valor trava |

## 2. Sequência
```c
periph_enable(&SYSCTL_RCGCWD, &SYSCTL_PRWD, 0);
WDT0->LOCK = 0x1ACCE551;
WDT0->LOAD = SYSCLK/1000 * 500;            /* 500 ms por estágio */
WDT0->TEST |= STALL;
WDT0->CTL  = RESEN | INTEN;                /* liga */
WDT0->LOCK = 0;
NVIC_EnableIRQ(WATCHDOG_IRQn);             /* 18 nos dois chips (WDT0 e WDT1 compartilham) */

/* alimentar */
WDT0->LOCK = 0x1ACCE551;  WDT0->ICR = 1;  WDT0->LOCK = 0;
```

Linha do tempo sem alimentação:
```
0 ms ── 500 ms: 1º timeout → interrupção ── 1000 ms: 2º timeout → RESET
```
Atender a interrupção (escrever `ICR`) recarrega o contador — o handler pode ser o próprio ponto de "alimentação", mas isso anula a proteção se o `main` estiver travado. Boa prática: a ISR só alimenta se o `main` sinalizou que está vivo.

## 3. Causa do reset (`SYSCTL_RESC`, 0x400F_E05C)
| Bit | Causa |
|---|---|
| 0 | EXT — pino RST |
| 1 | POR — power-on |
| 2 | BOR — brown-out |
| 3 | **WDT0** |
| 4 | SW — software (`SYSRESETREQ`) |
| 5 | **WDT1** |
| 6 | HIB (129) |
| 16 | MOSCFAIL — o cristal parou |

Os bits acumulam até serem limpos: escreva 0 depois de ler.

## 4. WDT1
```c
WDT1->LOAD = ...;  while (!(WDT1->CTL & WDT_CTL_WRC)) { }
```
Útil para pegar o caso "o PLL/cristal morreu e o SYSCLK parou" — com o WDT0 nesse caso o próprio watchdog pararia.

## 5. NMI
Com `INTTYPE = 1` o 1º estágio gera **NMI**, que não pode ser mascarada nem por `cpsid i` — garante a chance de registrar o erro (em `.noinit`, módulo 03) antes do reset.

---

## Exercícios
1. Guarde em `.noinit` o PC em que o programa estava quando o watchdog avisou (leia o stack frame na ISR).
2. Use o WDT1 e prove que ele continua funcionando com o SYSCLK parado.
3. Implemente "alimentação condicional": a ISR só alimenta se três tarefas diferentes marcaram flags desde a última vez.
