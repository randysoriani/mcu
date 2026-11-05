# Módulo 04 — Clock (System Control)

O TM4C123 e o TM4C129 têm **sistemas de clock bem diferentes**, apesar de serem da mesma família:

| | TM4C123 | TM4C129 |
|---|---|---|
| Registradores | `RCC` + `RCC2` (estilo Stellaris) | `RSCLKCFG`, `PLLFREQ0/1`, `PLLSTAT`, `MEMTIM0` |
| PLL | VCO fixo em 400 MHz | VCO programável (fração), 480 MHz no curso |
| Wait states da Flash | **automáticos** | **programados pelo software** (`MEMTIM0`) |
| Máximo | 80 MHz | 120 MHz |
| Saída de clock | não tem | `DIVSCLK` (PQ4) |

Código autocontido (não usa a biblioteca):
- [`123/main.c`](123/main.c) — 16 MHz → 400/`PLL_DIV` MHz (padrão 80)
- [`129/main.c`](129/main.c) — 16 MHz → 480/(`PSYSDIV`+1) MHz (padrão 120), com `MEMTIM0` calculado e `DIVSCLK` = SYSCLK/12 em PQ4

```sh
make MCU=123 flash
make clean && make MCU=129 EXTRA=-DPSYSDIV=7 flash      # 60 MHz
```
O LED pisca 5 vezes devagar (PIOSC) e depois, com o mesmo laço, muito mais rápido.

Versões usadas pelos demais módulos: [`common/123/clock.c`](../common/123/clock.c), [`common/129/clock.c`](../common/129/clock.c).

---

## 1. Fontes de clock

| Fonte | Freq. | Uso |
|---|---|---|
| **PIOSC** | 16 MHz ±1% (calibrado) | clock após o reset; ADC/UART podem usá-lo direto |
| **MOSC** | cristal 4–25 MHz (123) / 5–25 MHz (129) | fonte precisa do PLL. LaunchPads: **16 MHz** (123), **25 MHz** (129) |
| LFIOSC | ~30 kHz (impreciso) | deep-sleep, WDT1 alternativo |
| Hibernate 32,768 kHz | cristal | RTC (módulo 20) |
| **PLL** | 400 MHz (123) / VCO até 480 MHz (129) | SYSCLK alto |

---

## 2. TM4C123: RCC e RCC2 (base `0x400F_E000`)

```
                     ┌──── /2 ────┐
MOSC 16 MHz ──► PLL 400 MHz ──────┴─ DIV400 ─► /(SYSDIV2[:LSB] + 1) ─► SYSCLK
PIOSC ───────────────────────────────── (BYPASS=1) ────────────────►
```

### `RCC` (0x060) — campos usados
| Bits | Nome | |
|---|---|---|
| 0 | MOSCDIS | 1 = oscilador principal desligado |
| 5:4 | OSCSRC | fonte (substituído por OSCSRC2 quando USERCC2=1) |
| 10:6 | **XTAL** | frequência do cristal: `0x15` = **16 MHz**, `0x19` = 25 MHz |
| 11 | BYPASS | 1 = PLL ignorado |
| 13 | PWRDN | 1 = PLL desligado |
| 19:17 | PWMDIV | divisor do clock PWM (/2../64) |
| 20 | USEPWMDIV | usa o divisor do PWM (módulo 10) |
| 22 | USESYSDIV | usa o divisor do sistema |

### `RCC2` (0x070)
| Bits | Nome | |
|---|---|---|
| 6:4 | OSCSRC2 | 000 MOSC, 001 PIOSC, 010 PIOSC/4, 011 LFIOSC, 111 32 kHz |
| 11 | BYPASS2 | |
| 13 | PWRDN2 | |
| 22 | SYSDIV2LSB | bit menos significativo extra (com DIV400) |
| 28:23 | SYSDIV2 | divisor |
| 30 | DIV400 | 1 = divide os 400 MHz direto (permite 80 MHz) |
| 31 | **USERCC2** | 1 = campos do RCC2 têm precedência |

### Sequência (datasheet, *Initialization and Configuration*)
1. `RCC2.USERCC2 = 1`, `BYPASS2 = 1` (roda direto do oscilador durante a mudança)
2. `RCC.XTAL = 16 MHz`, `MOSCDIS = 0`, `RCC2.OSCSRC2 = MOSC`
3. `PWRDN2 = 0` (liga o PLL)
4. `USESYSDIV = 1`, `DIV400 = 1`, `{SYSDIV2,SYSDIV2LSB} = 4` → 400/5 = **80 MHz**
5. espera `RIS.PLLLRIS` (bit 6) — PLL travado
6. `BYPASS2 = 0`

Divisor = `{SYSDIV2,LSB} + 1`: 4 → 80 MHz, 5 → 66,7, 7 → 50, 9 → 40 MHz. Abaixo de 5 passaria do máximo.

A Flash do 123 é de 40 MHz e o controlador insere wait states e prefetch automaticamente — não há registrador de latência como no STM32.

---

## 3. TM4C129: RSCLKCFG, PLLFREQ e MEMTIM0

```
MOSC 25 MHz ─► /(N+1)=5 ─► 5 MHz ─► × (MINT + MFRAC/1024) = 96 ─► VCO 480 MHz ─► /(PSYSDIV+1)=4 ─► 120 MHz
```

| Registrador | Offset | Campos |
|---|---|---|
| `MOSCCTL` | 0x07C | 2 NOXTAL (1 = sem cristal), 3 PWRDN, 4 OSCRNG (1 = cristal > 10 MHz) |
| `RIS` | 0x050 | 8 MOSCPUPRIS (MOSC estabilizou) |
| `RSCLKCFG` | 0x0B0 | 9:0 PSYSDIV, 23:20 OSCSRC, 27:24 PLLSRC (3 = MOSC), 28 USEPLL, 30 NEWFREQ, 31 MEMTIMU |
| `PLLFREQ0` | 0x160 | 9:0 MINT, 19:10 MFRAC, 23 PLLPWR |
| `PLLFREQ1` | 0x164 | 4:0 N, 12:8 Q |
| `PLLSTAT` | 0x168 | 0 LOCK |
| `MEMTIM0` | 0x0C0 | 3:0 FWS, 9:6 FBCHT (Flash); 19:16 EWS, 25:22 EBCHT (EEPROM); bits 4 e 20 = 1 |
| `DIVSCLK` | 0x148 | 7:0 DIV, 17:16 SRC (0 SYSCLK, 1 PIOSC, 2 MOSC), 31 EN |

### Wait states (tabela do datasheet "MEMTIM0 Register Configuration versus Frequency")

| SYSCLK | FWS/EWS | FBCHT/EBCHT |
|---|---|---|
| ≤ 16 MHz | 0 | 0 (0,5 ciclo) |
| ≤ 40 MHz | 1 | 2 (1,5) |
| ≤ 60 MHz | 2 | 3 (2) |
| ≤ 80 MHz | 3 | 4 (2,5) |
| ≤ 100 MHz | 4 | 5 (3) |
| ≤ 120 MHz | 5 | 6 (3,5) |

### Sequência
1. `MOSCCTL`: NOXTAL = 0, PWRDN = 0, OSCRNG = 1; espera `MOSCPUPRIS`
2. `RSCLKCFG`: OSCSRC = PLLSRC = MOSC
3. `PLLFREQ1 = N`, `PLLFREQ0 = PLLPWR | MINT`
4. `MEMTIM0` com os valores da frequência **final**
5. `RSCLKCFG |= NEWFREQ`; espera `PLLSTAT.LOCK`
6. `RSCLKCFG = MEMTIMU | USEPLL | PSYSDIV | fontes` — a troca de clock e dos wait states acontece junta

`MEMTIMU` é o que torna seguro: o novo `MEMTIM0` só é aplicado no momento da troca.

---

## 4. Clock dos periféricos

No TM4C todos os periféricos (GPTM, UART, SSI, I2C, PWM...) recebem **SYSCLK** — não existem barramentos APB com divisores próprios como no STM32. Exceções configuráveis:
- UART/SSI: registrador `CC` pode escolher o PIOSC (permite mudar o SYSCLK sem recalcular o baud rate)
- ADC: PIOSC ou PLL/n (`ADCCC` no 129)
- PWM: divisor `/2../64` (RCC no 123, `PWMCC` no 129)
- WDT1: PIOSC

---

## Exercícios
1. 123 a 50 MHz (`PLL_DIV=8`): confirme pelo tempo do pisca e calcule o novo `IBRD/FBRD` para 115200.
2. 129 a 60 MHz: confira no `DIVSCLK` (PQ4) com osciloscópio.
3. Por que no 123 basta mudar o divisor, enquanto no 129 é obrigatório mexer no `MEMTIM0`?
4. Configure o 123 para rodar direto do MOSC (16 MHz, PLL desligado) e compare o consumo (jumper JP4).
