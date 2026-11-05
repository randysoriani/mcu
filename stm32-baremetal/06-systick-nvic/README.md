# Módulo 06 — SysTick, exceções e NVIC

O SysTick é um timer de 24 bits **dentro do núcleo** Cortex-M (existe em todo Cortex-M, igual em todos os fabricantes). É a base de tempo padrão: `millis()`, `delay_ms()`, tick de RTOS.

Este módulo também apresenta o **modelo de exceções** e o **NVIC**, usados por todos os periféricos com interrupção daqui em diante.

Código: [`src/main.c`](src/main.c) (F0 e F4) — implementa a base de tempo, uma "multitarefa cooperativa" sem bloqueios, mede ciclos com SysTick (e com o DWT no M4) e ajusta a prioridade da exceção.

```sh
make MCU=f4 flash && picocom -b 115200 /dev/ttyACM0
```
Saída (executada no QEMU, F4 a 168 MHz):
```
=== Modulo 06: SysTick ===
HCLK = 168000000 Hz, LOAD = 167999, CALIB = 0x4003344F
trabalho(): 14994 ciclos (SysTick) = 89250 ns
uptime 1 s (millis = 1000)
uptime 2 s (millis = 2000)
```
(Na placa real os ciclos medidos serão bem menores; o QEMU não é preciso em tempo de instrução.)

A versão usada pelos demais módulos é [`common/lib/systick.c`](../common/lib/systick.c).

---

## 1. SysTick

```
           HCLK ──┬──────────────► CLKSOURCE=1
                  └── /8 ─────────► CLKSOURCE=0
                                       │
   LOAD (24 bits) ──recarrega──► VAL (decrementa a cada clock)
                                       │ chega a 0
                                       ├─► COUNTFLAG = 1
                                       └─► exceção SysTick (se TICKINT)
```

Período = `(LOAD + 1)` clocks. Para 1 ms: `LOAD = HCLK/1000 − 1` (F0: 47 999; F4: 179 999). Limite: 2²⁴ − 1 = 16 777 215 → a 180 MHz, no máximo 93 ms por período.

### Registradores (base `0xE000_E010`)

| Offset | Registrador | Bits |
|---|---|---|
| 0x00 | `SYST_CSR` (`CTRL`) | 0 ENABLE, 1 TICKINT, 2 CLKSOURCE (1 = processador), 16 COUNTFLAG (limpa ao ler) |
| 0x04 | `SYST_RVR` (`LOAD`) | [23:0] valor de recarga |
| 0x08 | `SYST_CVR` (`VAL`) | [23:0] valor atual; **qualquer escrita zera** VAL e COUNTFLAG |
| 0x0C | `SYST_CALIB` | [23:0] TENMS (valor de fábrica para 10 ms), 30 SKEW, 31 NOREF |

Sequência de configuração:
```c
SysTick->CTRL = 0;                         /* para                      */
SysTick->LOAD = SystemCoreClock/1000 - 1;  /* 1 ms                      */
SysTick->VAL  = 0;                         /* começa do zero            */
SysTick->CTRL = CLKSOURCE | TICKINT | ENABLE;
```
```c
volatile uint32_t g_ms;
void SysTick_Handler(void) { g_ms++; }     /* o nome tem que bater com a tabela de vetores */
```

### `millis()` e o *overflow*

`g_ms` (32 bits) dá a volta após ~49,7 dias. Compare **diferenças**, nunca instantes absolutos:

```c
if (millis() - t0 >= 500) { ... }    /* CORRETO: aritmética sem sinal mod 2^32 */
if (millis() >= t0 + 500) { ... }    /* ERRADO: falha perto do overflow        */
```

### `volatile` e atomicidade

`g_ms` é alterada pela ISR e lida pelo `main` → precisa ser `volatile`. Ler um `uint32_t` alinhado é uma única instrução `LDR` (atômica) nos dois núcleos; para um contador de 64 bits seria preciso desabilitar interrupções durante a leitura.

### Multitarefa cooperativa

```c
uint32_t t_led = 0, t_print = 0;
for (;;) {
    uint32_t agora = millis();
    if (agora - t_led   >= 250)  { t_led   += 250;  toggle_led(); }
    if (agora - t_print >= 1000) { t_print += 1000; kprintf(...); }
    __WFI();                      /* dorme até a próxima interrupção */
}
```
Nenhuma "tarefa" bloqueia, então todas rodam no seu ritmo. Somar o período (`t += 250`) em vez de `t = agora` evita acumular atraso. É o padrão "super-loop" usado em boa parte dos firmwares sem RTOS.

### `__WFI()` — Wait For Interrupt

Para o núcleo até chegar uma interrupção (Sleep mode, módulo 21). Reduz o consumo e não muda a lógica: o tick de 1 ms acorda a CPU. Observação: com o debugger conectado, alguns ST-LINK perdem a conexão em WFI se `DBGMCU_CR.DBG_SLEEP = 0`.

---

## 2. Medindo tempo de execução

**Com SysTick** (M0 e M4): ler VAL antes e depois. Como ele **decrementa**:
```c
uint32_t a = SysTick->VAL;  f();  uint32_t b = SysTick->VAL;
uint32_t ciclos = (a >= b) ? a - b : a + (LOAD + 1) - b;   /* trata uma volta */
```
Vale para intervalos menores que um período (1 ms).

**Com DWT->CYCCNT** (só M3/M4/M7): contador de ciclos de 32 bits livre.
```c
CoreDebug->DEMCR |= (1 << 24);   /* TRCENA, 0xE000EDFC */
DWT->CYCCNT = 0;                 /* 0xE0001004 */
DWT->CTRL  |= 1;                 /* CYCCNTENA, 0xE0001000 */
```

---

## 3. Exceções do Cortex-M

| Nº | Exceção | Prioridade | Observação |
|---|---|---|---|
| 1 | Reset | −3 (fixa) | |
| 2 | NMI | −2 (fixa) | no STM32: Clock Security System |
| 3 | HardFault | −1 (fixa) | qualquer falha no M0; falhas escaladas no M4 |
| 4 | MemManage | configurável | M4: violação da MPU |
| 5 | BusFault | configurável | M4: acesso inválido ao barramento |
| 6 | UsageFault | configurável | M4: instrução indefinida, divisão por 0 (se habilitado), desalinhado, FPU desligada |
| 11 | SVCall | configurável | instrução `SVC` (chamadas de sistema de RTOS) |
| 14 | PendSV | configurável | troca de contexto de RTOS |
| 15 | SysTick | configurável | |
| 16+ | IRQ0..IRQn | configurável | periféricos, via NVIC |

**Número menor = prioridade maior.** Uma exceção de prioridade maior interrompe (preempta) uma de prioridade menor.

### Entrada e saída de uma exceção

1. O hardware empilha r0–r3, r12, LR, PC, xPSR (e registradores da FPU no M4, se usados).
2. Carrega o PC com o endereço da tabela de vetores; LR recebe `EXC_RETURN` (`0xFFFFFFF9`, `...F1`, `...FD`, `...E9`...).
3. O handler roda (função C normal).
4. Ao executar `BX LR` com `EXC_RETURN`, o hardware desempilha tudo.

Latência: 16 ciclos (M0) / 12 ciclos (M4). Otimizações do M4: *tail-chaining* (vai direto para a próxima interrupção pendente sem desempilhar) e *late arrival*.

---

## 4. NVIC — Nested Vectored Interrupt Controller

Habilita/desabilita, marca como pendente e define a prioridade de cada **IRQ de periférico** (exceções ≥ 16). Fica em `0xE000_E100`.

| Registrador | Endereço | M0 | M4 | Uso |
|---|---|---|---|---|
| `ISER[n]` | 0xE000_E100 | 1 word | 8 words | escreve 1 → **habilita** IRQ (bit `irq % 32` da word `irq / 32`) |
| `ICER[n]` | 0xE000_E180 | 1 | 8 | escreve 1 → desabilita |
| `ISPR[n]` | 0xE000_E200 | 1 | 8 | escreve 1 → força pendente (dispara por software) |
| `ICPR[n]` | 0xE000_E280 | 1 | 8 | escreve 1 → limpa pendente |
| `IABR[n]` | 0xE000_E300 | — | 8 | 1 = IRQ ativa (r) |
| `IPR` | 0xE000_E400 | 8 words, **só acesso de 32 bits** | 240 bytes, acesso por byte | prioridade (8 bits por IRQ, só os bits altos implementados) |
| `STIR` | 0xE000_EF00 | — | sim | dispara IRQ por software |

Os registradores *set/clear* separados (`ISER`/`ICER`) existem justamente para evitar read-modify-write: escrever 0 não tem efeito.

```c
NVIC->ISER[irq >> 5] = 1U << (irq & 31);     /* NVIC_EnableIRQ */
```

### Prioridades

- **F0 (M0)**: 2 bits → níveis 0, 1, 2, 3, armazenados nos bits [7:6] de cada byte. Valor escrito = `prio << 6`.
- **F4 (M4)**: 4 bits (STM32) → níveis 0..15, nos bits [7:4]. Valor escrito = `prio << 4`.

No M4 os 4 bits podem ser divididos entre **preempção** (grupo) e **subprioridade** pelo campo PRIGROUP do `SCB->AIRCR`. Subprioridade só decide a ordem entre interrupções pendentes ao mesmo tempo; não causa preempção. O padrão de reset (PRIGROUP = 0) dá 4 bits de preempção no STM32.

As funções `NVIC_EnableIRQ`, `NVIC_SetPriority`, ... estão no final de `common/*/stm32f*.h` — compare as versões M0 e M4 (o M0 precisa ler-modificar-escrever a word inteira do `IPR`).

### Prioridade das exceções do sistema (SCB->SHPR)

SysTick, PendSV e SVCall não passam pelo NVIC: a prioridade fica em `SCB->SHPR2/SHPR3` (`0xE000_ED1C`/`ED20`):

| Byte | Exceção |
|---|---|
| SHPR2[31:24] | SVCall (11) |
| SHPR3[23:16] | PendSV (14) |
| SHPR3[31:24] | **SysTick (15)** |

Ver `set_systick_priority()` em `src/main.c`.

### Mascarando interrupções

```c
__disable_irq();   /* cpsid i: PRIMASK = 1, bloqueia tudo exceto NMI e HardFault */
/* seção crítica */
__enable_irq();    /* cpsie i */
```
No M4 também existe `BASEPRI` (bloqueia só prioridades ≥ N) — usado por RTOS para não bloquear interrupções críticas.

---

## 5. Fluxo completo de uma interrupção de periférico

```
Periférico (ex.: TIM6)       NVIC                      Núcleo
  evento ─► flag (UIF) ─► IE (UIE) ─► IRQ54 pendente ─► habilitada (ISER)?
                                                       prioridade > atual?
                                                            │ sim
                                                            ▼
                                                   TIM6_DAC_IRQHandler()
                                                   ... limpa UIF !!! ...
```

Três "chaves" precisam estar ligadas: a **flag de interrupção** no periférico, o **enable** no periférico e o **enable no NVIC**. E o handler **precisa limpar a flag** de origem — senão, ao sair, a IRQ continua pendente e o handler roda de novo para sempre.

> Detalhe: a limpeza da flag passa pelo barramento e pode demorar alguns ciclos. Se ela for a **última** instrução do handler, a IRQ pode ser vista ainda ativa e reentrar uma vez. Limpe no início do handler ou leia a flag de volta depois de limpá-la.

---

## Exercícios

1. Mude o SysTick para 10 kHz (100 µs) e faça `micros()`. Qual o custo de CPU da interrupção (meça com DWT)?
2. Implemente `delay_ms()` **sem** interrupção, usando `COUNTFLAG`.
3. Adicione uma terceira tarefa cooperativa que lê o botão PC13 a cada 20 ms.
4. Faça uma ISR de SysTick demorada (laço de 2 ms) e observe o efeito no `millis()`. Por que ticks são perdidos?
5. No F4, use `NVIC->STIR` para disparar a IRQ do TIM7 por software e trate-a num handler.
