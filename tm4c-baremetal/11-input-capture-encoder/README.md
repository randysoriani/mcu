# Módulo 11 — Captura de bordas, contador de eventos e encoder (QEI)

No TM4C essas três funções estão em dois blocos: o **GPTM** (captura e contagem) e o **QEI** (encoder em quadratura, periférico próprio).

Código: [`src/main.c`](src/main.c). Gerador de teste: PWM em **PF2** (123: M1PWM6; 129: M0PWM2).

| | Pino de captura | Jumper |
|---|---|---|
| 123 | **T1CCP0 = PB4** (PCTL 7) | PF2 → PB4 |
| 129 | **T0CCP0 = PD0** (PCTL 3) | PF2 → PD0 |

```sh
make clean && make MCU=123 EXTRA=-DDEMO=1 flash   # 1 = edge-time, 2 = edge-count, 3 = QEI
```

---

## 1. GPTM em modo captura

`TAMR.TAMR = 3` (captura). O bit `TACMR` escolhe:

| TACMR | Modo | O que faz |
|---|---|---|
| 1 | **edge-time** | a cada borda, copia o valor do contador para `TAR` e seta `CAERIS` |
| 0 | **edge-count** | cada borda decrementa (ou incrementa) o contador |

`CTL.TAEVENT` escolhe a borda: 0 subida, 1 descida, **3 ambas**.

### 24 bits com o prescaler
Em modo 16 bits (`CFG = 4`) o prescaler (`TAPR`) vira **extensão** do contador em captura: `TAPR = 0xFF` + `TAILR = 0xFFFF` → contador de **24 bits** (até 209 ms a 80 MHz, 140 ms a 120 MHz, com resolução de 1 ciclo). `TAR[23:16]` traz os bits do prescaler.

### DEMO 1 — período e largura com uma única entrada
O GPTM não tem o modo "PWM input" do STM32 (dois canais no mesmo pino com reset automático). Solução: capturar **ambas as bordas** e decidir na ISR qual foi lendo o nível do pino:
```c
void TIMER1A_Handler(void)
{
    TIMER1->ICR = TIMER_INT_CAE;
    uint32_t t = TIMER1->TAR & 0xFFFFFF;
    if (pino_alto)  { period = (t_rise - t) & 0xFFFFFF; t_rise = t; }  /* subida */
    else            { high   = (t_rise - t) & 0xFFFFFF; }              /* descida */
}
```
O contador é **decrescente**, daí `anterior − atual`; a máscara de 24 bits trata a volta.

Limite: em frequências altas (pulsos < ~1 µs) a ISR não acompanha. Para isso, use dois timers (um em cada borda) ou o uDMA.

### DEMO 2 — contador de eventos
Edge-count, descendo de 0xFFFFFF, com `TAMATCHR/TAPMR = 0`: cada borda de subida decrementa. A cada segundo, `anterior − atual` = frequência. Ao atingir o match (16 M bordas), o timer para — reinicie-o se precisar contar mais.

### Registradores (além dos do módulo 09)
| Reg. | Uso aqui |
|---|---|
| `CFG = 4` | 16 bits |
| `TAMR = 3 \| TACMR` | captura, edge-time (DEMO 1) |
| `TAPR = 0xFF` | extensão para 24 bits |
| `CTL.TAEVENT = 3` | ambas as bordas |
| `IMR.CAEIM` (bit 2) | interrupção de evento de captura |
| `TAMATCHR`/`TAPMR` | ponto de parada no edge-count |

---

## 2. QEI — Quadrature Encoder Interface

| | QEI | PhA | PhB | PCTL |
|---|---|---|---|---|
| 123 | QEI1 | PC5 | PC6 | 6 |
| 129 | QEI0 | PL1 | PL2 | 6 |

(No 123 o QEI0 fica em PD6/PD7 — PD7 é protegido —, por isso o curso usa o QEI1.)

| Offset | Reg. | |
|---|---|---|
| 0x000 | `CTL` | 0 ENABLE, 1 SWAP, 2 SIGMODE (1 = pulso/direção), **3 CAPMODE** (1 = conta bordas de A e B → ×4), 4 RESMODE, 5 VELEN, 13 FILTEN, 19:16 FILTCNT |
| 0x004 | `STAT` | 0 ERROR, **1 DIRECTION** |
| 0x008 | **`POS`** | posição |
| 0x00C | `MAXPOS` | limite (dá a volta) |
| 0x010–0x01C | `LOAD`, `TIME`, `COUNT`, `SPEED` | medidor de velocidade |
| 0x020–0x028 | `INTEN`, `RIS`, `ISC` | índice, timer, direção, erro |

```c
QEI1->CTL    = 0;
QEI1->MAXPOS = 0xFFFFFFFF;
QEI1->POS    = 0;
QEI1->CTL    = CAPMODE | FILTEN | (15 << 16) | ENABLE;
int32_t pos = (int32_t)QEI1->POS;      /* / 4 = detents de um encoder comum */
```

O QEI também mede velocidade sozinho: com `VELEN`, ele conta pulsos num período definido em `LOAD` e deixa o resultado em `SPEED`.

---

## Exercícios
1. Meça a frequência do PWM do módulo 10 (servo) e mostre a largura em µs.
2. Use dois timers (A na subida, B na descida) para medir pulsos mais curtos.
3. Habilite `VELEN` no QEI e imprima RPM.
