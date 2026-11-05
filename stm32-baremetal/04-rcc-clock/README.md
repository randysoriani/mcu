# Módulo 04 — RCC: Reset and Clock Control

O RCC é o "coração" do chip: escolhe as fontes de clock, gera a frequência do núcleo com o PLL, divide para os barramentos e **liga/desliga o clock de cada periférico**. Também guarda as flags de causa de reset.

Código (autocontido, não usa a biblioteca do curso):
- [`f0/main.c`](f0/main.c) — 8 MHz → 48 MHz (HSI+PLL, HSE+PLL ou HSI48), MCO = SYSCLK/16 em PA8
- [`f4/main.c`](f4/main.c) — 16 MHz → 180 MHz (HSI ou HSE + PLL + over-drive), MCO1 = PLL/5 em PA8, MCO2 = SYSCLK/5 em PC9

```sh
make MCU=f0 flash                                  # HSI + PLL
make clean && make MCU=f0 EXTRA=-DCLOCK_SOURCE=3 flash   # HSI48 (só F072)
make clean && make MCU=f4 EXTRA=-DCLOCK_SOURCE=2 flash   # HSE bypass (8 MHz do ST-LINK)
```
O LED pisca 5 vezes devagar (clock de reset) e depois, com o **mesmo laço de atraso**, bem mais rápido. Com um osciloscópio em PA8 (pino D7 da Nucleo) você mede a frequência real.

A versão "de produção" das mesmas rotinas, usada pelos outros módulos, está em [`common/f0/clock.c`](../common/f0/clock.c) e [`common/f4/clock.c`](../common/f4/clock.c).

---

## 1. Fontes de clock

| Fonte | F072 | F446 | Notas |
|---|---|---|---|
| **HSI** | 8 MHz RC interno | 16 MHz RC interno | Liga no reset. ±1% a 25 °C (calibrado de fábrica). |
| **HSE** | 4–32 MHz cristal ou clock externo | 4–26 MHz | Preciso (ppm). Na Nucleo: **8 MHz vindo do MCO do ST-LINK** em modo *bypass* (sem cristal; verifique as solder bridges SB no UM1724). |
| **PLL** | multiplica HSI/2, HSI/PREDIV, HSE/PREDIV ou HSI48/PREDIV | M/N/P/Q/R a partir de HSI ou HSE | Gera o clock alto. |
| **HSI48** | 48 MHz RC (com CRS, sincronizável pelo USB SOF) | — | Permite USB sem cristal no F072. |
| **HSI14** | 14 MHz RC dedicado ao ADC | — | |
| **LSI** | ~40 kHz RC | ~32 kHz RC | Impreciso (±50%!). IWDG e RTC. |
| **LSE** | 32,768 kHz cristal | 32,768 kHz cristal | Preciso. RTC. Presente na Nucleo (X2). |
| **PLLI2S / PLLSAI** | — | PLLs auxiliares | Clocks de áudio (módulo 17), SAI, USB 48 MHz, SDIO. |

---

## 2. Árvore de clock simplificada

### STM32F072

```
HSI 8 MHz ──┬─────────────────────────────────┐
            └─ /2 ─┐                          │
HSE ───── /PREDIV ─┼─► PLLSRC ─► xPLLMUL ─► PLLCLK ─┐
HSI48 ── /PREDIV ──┘   (CFGR)    (2..16)            │
                                                     ▼
HSI ─────────────────────────────────────► SW ─► SYSCLK ─► /HPRE ─► HCLK (AHB, núcleo, SysTick) ≤ 48
HSE ─────────────────────────────────────►(CFGR)                       │
HSI48 ───────────────────────────────────►                             └► /PPRE ─► PCLK (APB) ≤ 48
                                                                                     └► TIMxCLK = PCLK×(1 ou 2)
```

### STM32F446

```
HSI 16 MHz ─┐                ┌─ /P ─► PLLCLK (SYSCLK) ≤ 180 MHz
            ├─► PLLSRC ─ /M ─┤VCO = in × N
HSE ────────┘  (1–2 MHz)     ├─ /Q ─► 48 MHz (USB/SDIO), SAI
               ↓             └─ /R ─► I2S, SAI, SPDIF
           /M (PLLI2S) ─ ×N ─ /P /Q /R ─► I2S, SAI, SPDIF (PLLI2S)

SW ─► SYSCLK ─► /HPRE ─► HCLK ≤ 180 ─┬─► /PPRE1 ─► PCLK1 ≤ 45 MHz (APB1) ─► TIMxCLK = 2×PCLK1 = 90
                                      └─► /PPRE2 ─► PCLK2 ≤ 90 MHz (APB2) ─► TIMxCLK = 2×PCLK2 = 180
```

**Regra dos timers**: se o prescaler do APB for 1, `TIMxCLK = PCLKx`; senão, `TIMxCLK = 2 × PCLKx`.

---

## 3. Registradores — STM32F072 (RM0091, cap. *Reset and clock control*)

Base: `RCC = 0x4002_1000`

| Offset | Registrador | Uso |
|---|---|---|
| 0x00 | `CR` | liga/desliga HSI, HSE, PLL e lê os bits *ready* |
| 0x04 | `CFGR` | SW, prescalers, fonte e multiplicador do PLL, MCO |
| 0x08 | `CIR` | interrupções do RCC (ready, CSS) |
| 0x0C/0x10/0x28 | `APB2RSTR`/`APB1RSTR`/`AHBRSTR` | reset individual de periféricos |
| 0x14 | `AHBENR` | clock: DMA, SRAM, FLITF, CRC, **GPIOA–F** |
| 0x18 | `APB2ENR` | clock: SYSCFG, ADC, TIM1, SPI1, USART1, TIM15–17, DBGMCU |
| 0x1C | `APB1ENR` | clock: TIM2/3/6/7/14, WWDG, SPI2, USART2–4, I2C1/2, USB, CAN, CRS, PWR, DAC |
| 0x20 | `BDCR` | backup domain: LSE, RTC (módulo 20) |
| 0x24 | `CSR` | LSI e **flags de causa de reset** (módulo 19) |
| 0x2C | `CFGR2` | PREDIV[3:0] (divisor antes do PLL) |
| 0x30 | `CFGR3` | seleção de clock da USART1/2/3, I2C1, CEC, USB |
| 0x34 | `CR2` | HSI14 e HSI48 |

### `RCC_CR`
| Bit | Nome | |
|---|---|---|
| 0 | HSION | liga o HSI |
| 1 | HSIRDY | HSI estável (r) |
| 16 | HSEON | liga o HSE |
| 17 | HSERDY | HSE estável (r) |
| 18 | HSEBYP | 1 = clock externo pronto (sem cristal) |
| 19 | CSSON | Clock Security System: se o HSE falhar, comuta para HSI e gera NMI |
| 24 | PLLON | liga o PLL |
| 25 | PLLRDY | PLL travado (r) |

### `RCC_CFGR`
| Bits | Nome | Valores |
|---|---|---|
| 1:0 | SW | 00 HSI, 01 HSE, 10 PLL, 11 HSI48 |
| 3:2 | SWS | fonte **efetivamente** em uso (r) — sempre confira após trocar SW |
| 7:4 | HPRE | 0xxx = /1, 1000 = /2 ... 1111 = /512 |
| 10:8 | PPRE | 0xx = /1, 100 = /2, 101 = /4, 110 = /8, 111 = /16 |
| 16:15 | PLLSRC | 00 HSI/2, 01 HSI/PREDIV, 10 HSE/PREDIV, 11 HSI48/PREDIV |
| 17 | PLLXTPRE | bit 0 do PREDIV (espelho de CFGR2) |
| 21:18 | PLLMUL | 0000 = ×2, 0001 = ×3, ..., 1010 = **×12**, ..., 1110/1111 = ×16 |
| 27:24 | MCO | 0000 off, 0001 HSI14, 0010 LSI, 0011 LSE, 0100 **SYSCLK**, 0101 HSI, 0110 HSE, 0111 PLL, 1000 HSI48 |
| 30:28 | MCOPRE | divisor do MCO: 000 /1 ... 100 **/16** ... 111 /128 |
| 31 | PLLNODIV | 1 = MCO recebe PLL sem dividir por 2 |

### `FLASH_ACR` (0x4002_2000)
| Bit | Nome | |
|---|---|---|
| 0 | LATENCY | 0 = 0 WS (SYSCLK ≤ 24 MHz), 1 = 1 WS (24 < SYSCLK ≤ 48 MHz) |
| 4 | PRFTBE | prefetch buffer |

---

## 4. Registradores — STM32F446 (RM0390, cap. *Reset and clock control*)

Base: `RCC = 0x4002_3800`

| Offset | Registrador | Uso |
|---|---|---|
| 0x00 | `CR` | HSI, HSE, CSS, PLL, PLLI2S, PLLSAI |
| 0x04 | `PLLCFGR` | M, N, P, Q, R e fonte do PLL principal |
| 0x08 | `CFGR` | SW, prescalers, MCO1/MCO2, RTCPRE |
| 0x30 | `AHB1ENR` | **GPIOA–H**, CRC, BKPSRAM, DMA1/2, OTG HS |
| 0x34 | `AHB2ENR` | DCMI, OTG FS |
| 0x38 | `AHB3ENR` | FMC, QSPI |
| 0x40 | `APB1ENR` | TIM2–7/12–14, WWDG, SPI2/3, USART2/3, UART4/5, I2C1–3, CAN1/2, PWR, DAC |
| 0x44 | `APB2ENR` | TIM1/8, USART1/6, ADC1–3, SPI1/4, SYSCFG, TIM9–11, SAI1/2 |
| 0x50–0x64 | `xxxLPENR` | clock dos periféricos durante Sleep |
| 0x70 | `BDCR` | LSE, RTC |
| 0x74 | `CSR` | LSI, flags de reset |
| 0x84 | `PLLI2SCFGR` | PLL de áudio |
| 0x8C | `DCKCFGR` | seleção de clock do I2S, SAI, timers (TIMPRE) |

### `RCC_PLLCFGR`
| Bits | Nome | Faixa / significado |
|---|---|---|
| 5:0 | PLLM | 2..63. **Entrada do VCO = fonte/M deve ficar entre 1 e 2 MHz** (2 MHz reduz jitter) |
| 14:6 | PLLN | 50..432. **VCO = entrada × N deve ficar entre 100 e 432 MHz** |
| 17:16 | PLLP | 00 = /2, 01 = /4, 10 = /6, 11 = /8 → SYSCLK ≤ 180 MHz |
| 22 | PLLSRC | 0 = HSI, 1 = HSE (também vale para PLLI2S e PLLSAI) |
| 27:24 | PLLQ | 2..15 → USB/SDIO precisam de 48 MHz |
| 30:28 | PLLR | 2..7 → I2S/SAI/SPDIF |

Conta do curso: `16 MHz / 8 = 2 MHz × 180 = 360 MHz / 2 = 180 MHz`.

### `RCC_CFGR`
| Bits | Nome | Valores |
|---|---|---|
| 1:0 | SW | 00 HSI, 01 HSE, 10 PLL_P, 11 PLL_R |
| 3:2 | SWS | (r) |
| 7:4 | HPRE | 0xxx = /1 ... |
| 12:10 | PPRE1 | 0xx /1, 100 /2, **101 /4**, 110 /8, 111 /16 |
| 15:13 | PPRE2 | idem (**100 /2**) |
| 20:16 | RTCPRE | HSE/n para o RTC |
| 22:21 | MCO1 | 00 HSI, 01 LSE, 10 HSE, 11 PLL |
| 26:24 | MCO1PRE | 0xx /1, 100 /2, 101 /3, 110 /4, 111 /5 |
| 29:27 | MCO2PRE | idem |
| 31:30 | MCO2 | 00 SYSCLK, 01 PLLI2S, 10 HSE, 11 PLL |

### Regulador e over-drive (PWR)

O núcleo do F446 roda a 1,2 V gerados por um regulador interno com três *scales*:

| VOS (PWR_CR[15:14]) | HCLK máx. | Over-drive |
|---|---|---|
| 01 Scale 3 | 120 MHz | — |
| 10 Scale 2 | 144 MHz | 168 MHz com OD |
| 11 Scale 1 | 168 MHz | **180 MHz com OD** |

Sequência de over-drive (RM0390, *Entering Over-drive mode*): PLL configurado e ligado → `ODEN = 1` → espera `ODRDY` → `ODSWEN = 1` → espera `ODSWRDY` → latência da Flash → trocar SW. O F401/F411 não têm over-drive (máx. 84/100 MHz); o F407 vai a 168 MHz sem ele.

### `FLASH_ACR` (0x4002_3C00)
| Bits | Nome | |
|---|---|---|
| 3:0 | LATENCY | wait states |
| 8 | PRFTEN | prefetch |
| 9 | ICEN | cache de instruções (ART) |
| 10 | DCEN | cache de dados |
| 11/12 | ICRST/DCRST | reset dos caches (com o cache desligado) |

Wait states do F446 a 2,7–3,6 V (RM0390, tabela *Number of wait states according to CPU clock*):

| HCLK | WS |
|---|---|
| ≤ 30 MHz | 0 |
| ≤ 60 | 1 |
| ≤ 90 | 2 |
| ≤ 120 | 3 |
| ≤ 150 | 4 |
| ≤ 180 | **5** |

---

## 5. A ordem importa

**Subindo** o clock:
1. aumentar a latência da Flash (e VOS/over-drive no F4) — **antes**
2. configurar prescalers dos barramentos (para não estourar o APB nem por um instante)
3. ligar a fonte (HSE) e esperar `RDY`
4. configurar o PLL **com ele desligado**; ligar; esperar `PLLRDY`
5. trocar `SW` e esperar `SWS` confirmar

**Descendo**: o inverso — trocar SW primeiro, reduzir a latência depois.

Errar a ordem não dá erro de compilação: dá um chip que trava aleatoriamente ou lê instruções erradas da Flash.

### Por que esperar `SWS` e os bits `RDY`?

Oscilador a cristal leva de µs a ms para estabilizar; o PLL precisa travar (~100 µs). A troca do SW só acontece quando a nova fonte está pronta. Em produção, coloque *timeouts* nesses laços (o exemplo HSE faz isso e pisca um erro se o HSE não subir).

---

## 6. Ligando o clock de um periférico

```c
/* F0 */
RCC->AHBENR  |= RCC_AHBENR_GPIOAEN;      /* GPIO no AHB */
RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
/* F4 */
RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
(void)RCC->APB1ENR;                      /* errata: 2 ciclos antes do 1º acesso */
```

Para "resetar" um periférico ao estado inicial sem resetar o chip: setar e limpar o bit em `xxxRSTR`.

---

## 7. MCO — observando o clock

O pino MCO (*Microcontroller Clock Output*) põe um clock interno (dividido) num pino. É a forma definitiva de verificar a configuração:

| | Pino | AF | No exemplo |
|---|---|---|---|
| F072 MCO | PA8 | AF0 | SYSCLK/16 = 3 MHz |
| F446 MCO1 | PA8 | AF0 | PLL/5 = 36 MHz |
| F446 MCO2 | PC9 | AF0 | SYSCLK/5 = 36 MHz |

Configure o pino com velocidade máxima (`OSPEEDR = 11`). Mesmo assim, acima de ~50 MHz a forma de onda fica senoidal.

---

## 8. Diferenças para outros chips

| Chip | Máx. | Observação |
|---|---|---|
| STM32F030 | 48 MHz | PLLSRC só HSI/2 ou HSE/PREDIV; sem HSI48 |
| STM32F401 | 84 MHz | PLLCFGR igual, sem PLLR; sem over-drive; APB1 ≤ 42 MHz |
| STM32F411 | 100 MHz | idem; APB1 ≤ 50 MHz; 3 WS a 100 MHz |
| STM32F407 | 168 MHz | sem over-drive; sem PLLR; APB1 ≤ 42, APB2 ≤ 84 |

---

## Exercícios

1. Configure o F446 para 84 MHz com PLLP = /4 e ajuste a latência. Meça no MCO.
2. No F072, escolha `PLLMUL` para 32 MHz a partir do HSI/2. Quantos wait states?
3. Ligue o **CSS** com HSE e implemente o `NMI_Handler` que acende o LED. Teste removendo a solder bridge do MCO do ST-LINK (avançado!) ou simplesmente desligando o HSE com o debugger.
4. Por que o exemplo usa M = 8 e N = 180 em vez de M = 16 e N = 360 no F446? (Dica: limites do VCO e jitter.)
5. Calcule PLLQ para obter 48 MHz para o USB no F446 mantendo SYSCLK = 168 MHz.
