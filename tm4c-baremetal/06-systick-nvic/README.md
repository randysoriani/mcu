# Módulo 06 — SysTick, exceções e NVIC

O SysTick e o NVIC são do **núcleo ARM**: funcionam exatamente como no STM32. A única diferença é que a TI implementa **3 bits** de prioridade (8 níveis, bits [7:5]) em vez dos 4 do STM32F4.

Código: [`src/main.c`](src/main.c) — base de tempo própria, multitarefa cooperativa, medição de ciclos com SysTick e DWT, prioridade do SysTick. A versão da biblioteca é [`common/lib/systick.c`](../common/lib/systick.c).

```sh
make MCU=129 flash && picocom -b 115200 /dev/ttyACM0
```

---

## 1. SysTick (`0xE000_E010`)

| Registrador | Bits |
|---|---|
| `CTRL` | 0 ENABLE, 1 TICKINT, 2 CLKSOURCE (1 = clock do sistema; 0 = PIOSC/4 no TM4C), 16 COUNTFLAG |
| `LOAD` | [23:0] recarga |
| `VAL` | [23:0] valor atual (escrever zera) |
| `CALIB` | |

Período = `LOAD + 1` ciclos. 1 ms: `LOAD = 79 999` (123) ou `119 999` (129). Máximo 2²⁴ ciclos = 210 ms (123) / 140 ms (129).

> No TM4C, `CLKSOURCE = 0` usa o **PIOSC/4** (4 MHz), útil para manter o tick constante quando o SYSCLK muda.

```c
volatile uint32_t g_ms;
void SysTick_Handler(void) { g_ms++; }
if (millis() - t0 >= 500) ...     /* compare diferenças: imune a overflow */
```

## 2. Multitarefa cooperativa
```c
for (;;) {
    uint32_t agora = millis();
    if (agora - t_led   >= 250)  { t_led   += 250;  toggle(); }
    if (agora - t_print >= 1000) { t_print += 1000; kprintf(...); }
    __WFI();
}
```

## 3. Medindo tempo
- **SysTick**: `VAL` antes/depois (contador decrescente).
- **DWT->CYCCNT** (`0xE000_1004`): habilite `DEMCR.TRCENA` (bit 24 de `0xE000_EDFC`) e `DWT_CTRL.CYCCNTENA`.

## 4. Exceções
| Nº | Exceção | Prioridade |
|---|---|---|
| 1 | Reset | −3 |
| 2 | NMI | −2 (no TM4C pode vir de pino NMI = PF0/PD7) |
| 3 | HardFault | −1 |
| 4–6 | MemManage, BusFault, UsageFault | configuráveis |
| 11 | SVCall | |
| 14 | PendSV | |
| 15 | SysTick | |
| 16+ | IRQ0.. | via NVIC |

## 5. NVIC (`0xE000_E100`)

| Registrador | Uso |
|---|---|
| `ISER[n]` / `ICER[n]` | habilita / desabilita (1 bit por IRQ) |
| `ISPR[n]` / `ICPR[n]` | força / limpa pendente |
| `IABR[n]` | ativa |
| `IP[irq]` | prioridade (byte; só bits [7:5] no TM4C) |
| `STIR` (`0xE000_EF00`) | dispara IRQ por software |

```c
NVIC->ISER[irq >> 5] = 1U << (irq & 31);
NVIC->IP[irq] = prio << 5;                    /* 0..7 */
SCB->SHP[11]  = prio << 5;                    /* SysTick */
```

O TivaWare usa "prioridade 0x20, 0x40, ..., 0xE0" — é o mesmo valor já deslocado.

## 6. Três chaves de uma interrupção
1. flag/máscara no periférico (`xxxIM`)
2. habilitar no NVIC
3. o handler **limpa a flag** (`xxxICR`)

Detalhe da TI: **limpar a flag como última instrução do handler** pode fazer a IRQ reentrar uma vez (a escrita ainda está a caminho pelo barramento). Limpe no início, ou leia o registrador de volta depois.

---

## Exercícios
1. SysTick com `CLKSOURCE = 0` (PIOSC/4): calcule o `LOAD` para 1 ms e mostre que ele não muda com o SYSCLK.
2. `delay_ms()` por polling de `COUNTFLAG`.
3. Use `NVIC->STIR` para disparar a IRQ do TIMER1A por software.
