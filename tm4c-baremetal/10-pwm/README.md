# Módulo 10 — PWM (módulo PWM dedicado)

O TM4C tem **módulos PWM dedicados** (motor control PWM), separados dos timers. Cada módulo tem **4 geradores**; cada gerador tem um contador de 16 bits e produz **2 saídas** (A e B) → 8 saídas por módulo (MnPWM0..7).

| | TM4C123 | TM4C129 |
|---|---|---|
| Módulos | PWM0, PWM1 | PWM0 |
| Divisor do clock PWM | `SYSCTL_RCC.PWMDIV` + `USEPWMDIV` | `PWM0->CC` (`PWMCC`) |

Código: [`src/main.c`](src/main.c)

| | fade 1 kHz | servo 50 Hz |
|---|---|---|
| 123 | **M1PWM6 = PF2 (LED azul)**, PWM1 gen3 A, PCTL 5 | M0PWM0 = PB6, PWM0 gen0 A, PCTL 4 |
| 129 | **M0PWM0 = PF0 (LED D4)**, PWM0 gen0 A, PCTL 6 | M0PWM2 = PF2, PWM0 gen1 A, PCTL 6 |

```sh
make MCU=123 flash
```

---

## 1. Estrutura

```
              ┌── gerador 0 ──► PWM0 (A), PWM1 (B)
PWM clock ────┼── gerador 1 ──► PWM2, PWM3
(SYSCLK/div)  ├── gerador 2 ──► PWM4, PWM5
              └── gerador 3 ──► PWM6, PWM7      ──► ENABLE[n] ──► pino (AFSEL+PCTL)
```

Mapeamento: saída `MnPWMk` = módulo n, gerador `k/2`, saída A se k par, B se ímpar. Bit de habilitação em `PWMn->ENABLE` = k.

## 2. Registradores

### Do módulo (base PWMn)
| Offset | Reg. | |
|---|---|---|
| 0x000 | `CTL` | sincronização global |
| 0x008 | **`ENABLE`** | bit k = liga a saída MnPWMk no pino |
| 0x00C | `INVERT` | inverte saídas |
| 0x010 | `FAULT` | estado em falha |
| 0xFC8 | `CC` | (129) bit 8 USEPWM, [2:0] PWMDIV |

### Do gerador g (base + 0x40 + 0x40·g)
| Offset | Reg. | |
|---|---|---|
| +0x00 | `CTL` | 0 ENABLE, 1 MODE (0 = conta p/ baixo, 1 = up/down centrado), 2 DEBUG |
| +0x10 | **`LOAD`** | período − 1 (16 bits) |
| +0x14 | `COUNT` | contador (r) |
| +0x18 | **`CMPA`** | comparador A |
| +0x1C | `CMPB` | comparador B |
| +0x20 | **`GENA`** | ações da saída A |
| +0x24 | `GENB` | ações da saída B |
| +0x28 | `DBCTL` | dead-band (pares complementares para pontes H) |

### GENA/GENB: o que fazer em cada evento
Cada evento tem 2 bits: `00` nada, `01` inverte, `10` → 0, `11` → 1.

| Bits | Evento |
|---|---|
| 1:0 | contador = 0 |
| 3:2 | contador = LOAD |
| 5:4 | = CMPA subindo |
| 7:6 | = CMPA descendo |
| 9:8 | = CMPB subindo |
| 11:10 | = CMPB descendo |

Configuração do curso (down-count): `GENA = 0x8C` → **nível 1 no LOAD, nível 0 em CMPA descendo**:
```
LOAD ──┐
       │\        saída = 1 de LOAD até CMPA
CMPA - │ \──────  saída = 0 de CMPA até 0
   0 ──┘  
duty = (LOAD − CMPA) / (LOAD + 1)
```
Para B: `GENB = 0x80C` (CMPB descendo).

## 3. Clock e contas

| | SYSCLK | /64 | 1 kHz → LOAD | 50 Hz → LOAD |
|---|---|---|---|---|
| 123 | 80 MHz | 1,25 MHz | 1249 | 24 999 |
| 129 | 120 MHz | 1,875 MHz | 1874 | 37 499 |

Sem divisor, 16 bits a 80 MHz alcançam só 1,2 kHz como frequência mínima — por isso o divisor.

```c
/* 123 */ SYSCTL_RCC |= USEPWMDIV | (5 << 17);      /* /64 */
/* 129 */ PWM0->CC = (1 << 8) | 5;                  /* /64 */
```
Divisores: 0 /2, 1 /4, 2 /8, 3 /16, 4 /32, 5 /64.

## 4. Sequência
```c
periph_enable(&SYSCTL_RCGCPWM, &SYSCTL_PRPWM, 1);   /* PWM1 */
gpio_af(GPIOF, 2, 5);                              /* PF2 = M1PWM6 */
PWM1->GEN[3].CTL  = 0;
PWM1->GEN[3].LOAD = 1249;
PWM1->GEN[3].CMPA = 1249 - alto;
PWM1->GEN[3].GENA = 0x8C;
PWM1->GEN[3].CTL  = 1;              /* liga o gerador */
PWM1->ENABLE     |= 1 << 6;         /* liga a saída   */
```
`CMPA`/`LOAD` escritos com o gerador rodando são aplicados no fim do período (sem glitch), a menos que se configure atualização imediata.

## 5. Outros recursos
- **Up/down** (`MODE = 1`): PWM centrado, para motores.
- **Dead-band**: gera A e A' complementares com tempo morto (ponte H, inversores).
- **Fault**: entrada que força as saídas a um estado seguro em hardware.
- **Trigger de ADC** sincronizado ao PWM (`INTEN`).
- Alternativa: PWM pelo GPTM (`TAAMS = 1`), útil em pinos sem módulo PWM.

---

## Exercícios
1. No 123, use as três cores (M1PWM5 = PF1, M1PWM6 = PF2, M1PWM7 = PF3) para um arco-íris suave.
2. Troque para modo up/down e compare no osciloscópio.
3. Configure dead-band de 1 µs entre M0PWM0 e M0PWM1.
