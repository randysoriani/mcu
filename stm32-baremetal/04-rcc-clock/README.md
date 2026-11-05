# Módulo 04 — RCC: o sistema de clock

> **Objetivo deste módulo:** entender o que é o clock, de onde ele vem, como o chip passa de 8/16 MHz para 48/180 MHz e por que nenhum periférico funciona antes de "ligar o clock" dele. Tudo isso é controlado por um único periférico, o **RCC** (*Reset and Clock Control*).

Código (autocontido: não usa a biblioteca do curso):
- [`f0/main.c`](f0/main.c) — 8 MHz → 48 MHz, com três fontes possíveis, e mostra o clock no pino PA8
- [`f4/main.c`](f4/main.c) — 16 MHz → 180 MHz, e mostra o clock nos pinos PA8 e PC9

```sh
make MCU=f0 flash                                          # padrão: HSI + PLL
make clean && make MCU=f0 EXTRA=-DCLOCK_SOURCE=3 flash     # HSI48 (só F072)
make clean && make MCU=f4 EXTRA=-DCLOCK_SOURCE=2 flash     # HSE (8 MHz do ST-LINK)
```
O que observar: o LED pisca 5 vezes devagar (clock de reset) e depois, **com exatamente o mesmo laço de atraso no código**, muito mais rápido. A única coisa que mudou foi a velocidade do clock. Com um osciloscópio no pino PA8 (pino D7 da Nucleo) você mede a frequência real.

A versão usada pelos outros módulos está em [`common/f0/clock.c`](../common/f0/clock.c) e [`common/f4/clock.c`](../common/f4/clock.c).

---

## 1. O que é o clock

O processador e os periféricos são circuitos digitais **síncronos**: eles só avançam um passo a cada pulso de um sinal quadrado chamado **clock**.

```
clock   ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐
       ─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─
        1   2   3   4   5   6      <- a cada pulso, o circuito dá um passo
```

Analogia: é o **metrônomo** de uma orquestra. Cada músico (processador, timer, serial...) toca no ritmo dele. Metrônomo mais rápido = música mais rápida.

- A frequência é medida em hertz (pulsos por segundo): 16 MHz = 16 milhões de pulsos por segundo.
- O Cortex-M executa, grosso modo, uma instrução simples por pulso. A 16 MHz isso dá ~16 milhões de instruções por segundo; a 180 MHz, mais de 10 vezes isso.
- Os periféricos também dependem do clock: o baud rate da serial, a contagem dos timers e a velocidade do SPI são **calculados a partir dele**. Se o clock estiver errado, tudo que depende de tempo fica errado.

Por que não usar sempre o máximo? Porque consumo de energia cresce com a frequência. Um aparelho a pilha pode rodar devagar e "acelerar" só quando precisa.

---

## 2. De onde vem o clock: as fontes

Um clock precisa de um **oscilador**, um circuito que gera a onda quadrada. Há dois tipos:

| Tipo | Como funciona | Vantagem | Desvantagem |
|---|---|---|---|
| **RC interno** | resistor + capacitor dentro do chip | sempre disponível, sem peças externas, liga na hora | impreciso (≈1%) e varia com a temperatura |
| **Cristal externo** | um cristal de quartzo vibra numa frequência muito exata | preciso (milionésimos) | precisa de peças na placa e leva alguns milissegundos para estabilizar |

O STM32 tem várias fontes de clock:

| Fonte | F072 | F446 | Para que serve |
|---|---|---|---|
| **HSI** (*High Speed Internal*) | RC de 8 MHz | RC de 16 MHz | o clock após o reset; sempre funciona |
| **HSE** (*High Speed External*) | 4–32 MHz | 4–26 MHz | cristal ou sinal externo preciso. Na Nucleo: **8 MHz fornecidos pelo ST-LINK** (modo *bypass*, sem cristal; ver as *solder bridges* no manual UM1724) |
| **PLL** | multiplica a entrada | multiplica e divide (M, N, P, Q, R) | gera as frequências altas (seção 3) |
| **HSI48** | RC de 48 MHz | — | permite USB sem cristal no F072 |
| **HSI14** | RC de 14 MHz | — | dedicado ao conversor ADC |
| **LSI** (*Low Speed Internal*) | ~40 kHz | ~32 kHz | muito impreciso (±50%!); usado pelo watchdog (módulo 19) |
| **LSE** (*Low Speed External*) | cristal de 32,768 kHz | idem | relógio de tempo real (módulo 20). 32 768 = 2¹⁵, então dividir por 2 quinze vezes dá exatamente 1 Hz |
| **PLLI2S / PLLSAI** | — | PLLs auxiliares | clocks de áudio (módulo 17) e USB |

---

## 3. O PLL: como 16 MHz vira 180 MHz

Os osciladores disponíveis são lentos (8 ou 16 MHz). Para chegar a 180 MHz, o chip usa um **PLL** (*Phase-Locked Loop*): um circuito que **multiplica** uma frequência de entrada por um número configurável, mantendo-se "travado" na referência (a precisão da saída é a mesma da entrada).

Analogia: é a **marcha de uma bicicleta**. A pedalada (oscilador) é lenta e constante; a relação das engrenagens (multiplicador do PLL) faz a roda girar muito mais rápido.

No F446 o PLL tem três etapas, e cada uma tem limites que precisam ser respeitados:

```
HSI 16 MHz ──► ÷M ──► 2 MHz ──► ×N ──► 360 MHz ──► ÷P ──► 180 MHz  (SYSCLK)
                      (entrada       (o "VCO":           │
                      do PLL:        100–432 MHz)        ├─► ÷Q ──► USB, SDIO
                      1–2 MHz)                           └─► ÷R ──► áudio
```
Conta do curso: `16 MHz ÷ 8 = 2 MHz × 180 = 360 MHz ÷ 2 = 180 MHz`.

No F072 é mais simples: `(HSI 8 MHz ÷ 2) × 12 = 48 MHz`.

Depois de ligar o PLL, é preciso **esperar ele "travar"** (estabilizar), o que leva uma fração de milissegundo. Um bit no RCC avisa quando está pronto (`PLLRDY`).

---

## 4. A árvore de clock: quem recebe o quê

O clock escolhido (chamado **SYSCLK**) é distribuído pelo chip como os galhos de uma árvore, passando por **divisores** (*prescalers*) que geram frequências menores para cada barramento:

### STM32F072
```
HSI 8 MHz ──┬─────────────────────────────────┐
            └─ ÷2 ─┐                          │
HSE ────── ÷PREDIV ┼─► escolha ─► ×PLLMUL ─► PLLCLK ─┐
HSI48 ─── ÷PREDIV ─┘   (PLLSRC)   (2..16)            │
                                                     ▼
HSI / HSE / HSI48 / PLL ──► escolha (SW) ─► SYSCLK ─► ÷HPRE ─► HCLK  (núcleo, memória, SysTick) ≤ 48 MHz
                                                                 │
                                                                 └► ÷PPRE ─► PCLK (periféricos APB) ≤ 48 MHz
```

### STM32F446
```
HSI 16 MHz ─┐                ┌─ ÷P ─► PLLCLK ≤ 180 MHz
            ├─► escolha ─ ÷M ─┤ ×N (VCO)
HSE ────────┘                ├─ ÷Q ─► 48 MHz (USB/SDIO)
                             └─ ÷R ─► áudio

escolha (SW) ─► SYSCLK ─► ÷HPRE ─► HCLK ≤ 180 ─┬─► ÷PPRE1 ─► PCLK1 ≤ 45 MHz (APB1) ─► timers: 90 MHz
                                                └─► ÷PPRE2 ─► PCLK2 ≤ 90 MHz (APB2) ─► timers: 180 MHz
```

| Nome | O que alimenta | Máximo no F446 |
|---|---|---|
| **SYSCLK** | a saída escolhida (HSI, HSE ou PLL) | 180 MHz |
| **HCLK** | o núcleo, as memórias, o DMA, o barramento AHB (onde estão as portas GPIO) | 180 MHz |
| **PCLK1** | periféricos do APB1 (USART2, I2C, timers 2–7...) | 45 MHz |
| **PCLK2** | periféricos do APB2 (USART1, SPI1, ADC, timer 1...) | 90 MHz |

Os barramentos APB são mais lentos porque os periféricos neles não aguentam 180 MHz. Por isso precisamos dividir.

**Regra dos timers:** se o divisor de um APB for diferente de 1, os timers daquele barramento recebem o **dobro** do PCLK. É por isso que, no F446, os timers do APB1 rodam a 90 MHz mesmo com PCLK1 = 45 MHz (módulo 09).

---

## 5. Wait states: a Flash não acompanha o processador

A Flash é mais lenta que o núcleo: ela leva por volta de 30 ns para entregar um dado. A 180 MHz um ciclo dura 5,6 ns; o processador pediria uma instrução e ela ainda não estaria pronta. Se nada for feito, **ele lê lixo e trava**.

A solução são os **wait states** (estados de espera): ciclos em que o processador espera a Flash responder. Eles são configurados no registrador `FLASH_ACR`, campo `LATENCY`:

| F072 (SYSCLK) | wait states |
|---|---|
| até 24 MHz | 0 |
| 24 a 48 MHz | 1 |

| F446 a 3,3 V (HCLK) | wait states |
|---|---|
| até 30 MHz | 0 |
| até 60 | 1 |
| até 90 | 2 |
| até 120 | 3 |
| até 150 | 4 |
| até 180 | **5** |

Cinco ciclos de espera parece muito, mas o F4 tem o **ART Accelerator** (cache de instruções e dados + *prefetch*, que busca as próximas instruções adiantado): na prática a maior parte do código roda sem esperar. Ele é ligado pelos bits `ICEN`, `DCEN` e `PRFTEN` do `FLASH_ACR`.

> A latência precisa ser aumentada **antes** de subir o clock. Se o clock subir primeiro, por um instante o processador lê a Flash rápido demais.

---

## 6. Ligando o clock de cada periférico ("clock gating")

Além de gerar o clock, o RCC tem uma "chave" por periférico. **Depois do reset, todas estão desligadas**: os periféricos não recebem clock, ficam parados e não gastam energia.

Para usar um periférico, o primeiro passo é sempre ligar o bit dele num registrador `...ENR` (*enable register*) do RCC:

```c
/* F0 */
RCC->AHBENR  |= RCC_AHBENR_GPIOAEN;      /* porta GPIOA (barramento AHB)    */
RCC->APB1ENR |= RCC_APB1ENR_USART2EN;    /* serial USART2 (barramento APB1) */
/* F4 */
RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
(void)RCC->APB1ENR;                      /* errata do F4: esperar 2 ciclos antes do 1º acesso */
```

Como saber qual registrador e qual bit? Pelo barramento em que o periférico está (mapa de memória do módulo 01) e pela tabela do registrador no Reference Manual. Os headers do curso já têm os nomes (`RCC_APB1ENR_USART2EN` etc.).

> Esquecer de ligar o clock é o erro mais comum em embarcados: o código parece certo, compila, e o periférico simplesmente não faz nada.

Para voltar um periférico ao estado inicial sem resetar o chip inteiro: ligar e desligar o bit dele no registrador `...RSTR`.

---

## 7. A ordem dos passos importa

**Para subir o clock:**
1. aumentar os wait states da Flash (no F4: também configurar o regulador de tensão e o *over-drive*, seção 9) — **antes de tudo**;
2. configurar os divisores dos barramentos, para que nenhum barramento passe do seu limite nem por um instante;
3. ligar a fonte (HSE, se usada) e esperar ela ficar pronta (`HSERDY`);
4. configurar o PLL **com ele desligado**, ligá-lo e esperar travar (`PLLRDY`);
5. trocar a fonte do SYSCLK (`SW`) e **conferir** pelo campo `SWS` que a troca aconteceu.

**Para descer o clock:** o contrário — trocar a fonte primeiro, reduzir os wait states depois.

Errar a ordem não dá erro de compilação: dá um chip que trava de vez em quando, o pior tipo de defeito. Por isso o código espera cada bit de "pronto" (`...RDY`). Em produto real, esses laços de espera devem ter *timeout* (o exemplo com HSE faz isso e pisca um erro se o HSE não subir).

---

## 8. O código, passo a passo (F446, `f4/main.c`)

```c
/* 1) Regulador interno em "Scale 1" (necessário acima de 168 MHz) */
RCC->APB1ENR |= RCC_APB1ENR_PWREN;          /* liga o clock do periférico PWR */
PWR->CR = (PWR->CR & ~PWR_CR_VOS_Msk) | (3U << PWR_CR_VOS_Pos);

/* 2) Fonte do PLL: HSI (16 MHz) -> M = 8 -> 2 MHz */
/* 3) PLL desligado; configura M, N, P, Q, R; liga */
RCC->CR &= ~RCC_CR_PLLON;
while (RCC->CR & RCC_CR_PLLRDY) { }
RCC->PLLCFGR = (8 << 0) | (180 << 6) | (0 << 16) | (8 << 24) | (2 << 28);
RCC->CR |= RCC_CR_PLLON;

/* 4) Over-drive (só F42x/F43x/F446): necessário acima de 168 MHz */
PWR->CR |= PWR_CR_ODEN;    while (!(PWR->CSR & PWR_CSR_ODRDY))   { }
PWR->CR |= PWR_CR_ODSWEN;  while (!(PWR->CSR & PWR_CSR_ODSWRDY)) { }

/* 5) Flash: 5 wait states + caches + prefetch */
FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | 5;

/* 6) Divisores: AHB ÷1, APB1 ÷4 (45 MHz), APB2 ÷2 (90 MHz) */
RCC->CFGR = (RCC->CFGR & ~(...)) | (5 << 10) | (4 << 13);

/* 7) Espera o PLL travar e troca o SYSCLK para ele */
while (!(RCC->CR & RCC_CR_PLLRDY)) { }
RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL;
while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL) { }   /* confirma */
```
Cada linha corresponde a um passo da seção 7; os campos são detalhados nas seções 10 e 11.

---

## 9. Regulador de tensão e over-drive (F446)

O núcleo do F446 funciona com 1,2 V, gerados por um regulador interno a partir dos 3,3 V da placa. Para frequências altas o núcleo precisa de um pouco mais de tensão, escolhida pelo campo `VOS` (*Voltage Output Scaling*) do registrador `PWR_CR`. Acima de 168 MHz é preciso ainda o modo **over-drive**:

| VOS (`PWR_CR[15:14]`) | HCLK máx. | com over-drive |
|---|---|---|
| 01 Scale 3 | 120 MHz | — |
| 10 Scale 2 | 144 MHz | 168 MHz |
| 11 Scale 1 | 168 MHz | **180 MHz** |

Sequência do manual (RM0390, *Entering Over-drive mode*): PLL configurado e ligado → `ODEN = 1` → esperar `ODRDY` → `ODSWEN = 1` → esperar `ODSWRDY` → wait states da Flash → trocar SW. O F401/F411 não têm over-drive (máximo de 84/100 MHz); o F407 chega a 168 MHz sem ele.

---

## 10. Registradores — STM32F072 (RM0091, capítulo *Reset and clock control*)

Endereço-base: `RCC = 0x4002_1000`. Cada registrador fica num *offset* (distância) a partir dessa base.

| Offset | Registrador | Para que serve |
|---|---|---|
| 0x00 | `CR` | liga/desliga HSI, HSE e PLL e informa quando estão prontos |
| 0x04 | `CFGR` | escolhe a fonte do SYSCLK, os divisores, a fonte e o multiplicador do PLL e a saída MCO |
| 0x08 | `CIR` | interrupções do RCC |
| 0x0C/0x10/0x28 | `APB2RSTR`/`APB1RSTR`/`AHBRSTR` | reset individual de periféricos |
| 0x14 | `AHBENR` | clock de: DMA, SRAM, CRC, **portas GPIOA–F** |
| 0x18 | `APB2ENR` | clock de: SYSCFG, ADC, TIM1, SPI1, USART1, TIM15–17 |
| 0x1C | `APB1ENR` | clock de: TIM2/3/6/7/14, SPI2, USART2–4, I2C1/2, USB, CAN, PWR, DAC |
| 0x20 | `BDCR` | domínio de backup: LSE e RTC (módulo 20) |
| 0x24 | `CSR` | LSI e **flags da causa do último reset** (módulo 19) |
| 0x2C | `CFGR2` | divisor PREDIV, antes do PLL |
| 0x30 | `CFGR3` | fonte de clock da USART1/2/3, I2C1, CEC, USB |
| 0x34 | `CR2` | HSI14 e HSI48 |

### `RCC_CR`
| Bit | Nome | Significado |
|---|---|---|
| 0 | HSION | liga o HSI |
| 1 | HSIRDY | HSI estável (só leitura) |
| 16 | HSEON | liga o HSE |
| 17 | HSERDY | HSE estável (só leitura) |
| 18 | HSEBYP | 1 = sinal de clock pronto chegando de fora (sem cristal) |
| 19 | CSSON | *Clock Security System*: se o HSE falhar, volta para o HSI e gera uma interrupção NMI |
| 24 | PLLON | liga o PLL |
| 25 | PLLRDY | PLL travado (só leitura) |

### `RCC_CFGR`
| Bits | Nome | Valores |
|---|---|---|
| 1:0 | SW | fonte do SYSCLK: 00 HSI, 01 HSE, 10 PLL, 11 HSI48 |
| 3:2 | SWS | fonte **realmente** em uso (só leitura) — confira sempre depois de mudar o SW |
| 7:4 | HPRE | divisor do AHB: 0xxx = ÷1, 1000 = ÷2 ... 1111 = ÷512 |
| 10:8 | PPRE | divisor do APB: 0xx = ÷1, 100 = ÷2, 101 = ÷4, 110 = ÷8, 111 = ÷16 |
| 16:15 | PLLSRC | entrada do PLL: 00 HSI÷2, 01 HSI÷PREDIV, 10 HSE÷PREDIV, 11 HSI48÷PREDIV |
| 21:18 | PLLMUL | multiplicador: 0000 = ×2, 0001 = ×3, ..., 1010 = **×12**, ..., 1110/1111 = ×16 |
| 27:24 | MCO | o que sai no pino MCO: 0100 = **SYSCLK**, 0101 HSI, 0110 HSE, 0111 PLL ... |
| 30:28 | MCOPRE | divisor do MCO: 000 ÷1 ... 100 **÷16** ... 111 ÷128 |

### `FLASH_ACR` (`0x4002_2000`)
| Bit | Nome | |
|---|---|---|
| 0 | LATENCY | 0 = 0 wait states (até 24 MHz), 1 = 1 wait state (24 a 48 MHz) |
| 4 | PRFTBE | liga o *prefetch* (busca adiantada de instruções) |

---

## 11. Registradores — STM32F446 (RM0390, capítulo *Reset and clock control*)

Endereço-base: `RCC = 0x4002_3800`.

| Offset | Registrador | Para que serve |
|---|---|---|
| 0x00 | `CR` | liga/desliga HSI, HSE, PLL, PLLI2S, PLLSAI; bits de "pronto" |
| 0x04 | `PLLCFGR` | M, N, P, Q, R e entrada do PLL principal |
| 0x08 | `CFGR` | fonte do SYSCLK, divisores, MCO1/MCO2 |
| 0x30 | `AHB1ENR` | clock de **GPIOA–H**, CRC, DMA1/2, USB HS |
| 0x34 | `AHB2ENR` | clock de câmera (DCMI), USB FS |
| 0x38 | `AHB3ENR` | clock de memória externa (FMC, QSPI) |
| 0x40 | `APB1ENR` | clock de TIM2–7/12–14, SPI2/3, USART2/3, UART4/5, I2C1–3, CAN1/2, PWR, DAC |
| 0x44 | `APB2ENR` | clock de TIM1/8, USART1/6, ADC1–3, SPI1/4, SYSCFG, TIM9–11, SAI1/2 |
| 0x50–0x64 | `xxxLPENR` | quais periféricos continuam com clock no modo de baixo consumo |
| 0x70 | `BDCR` | LSE, RTC |
| 0x74 | `CSR` | LSI, causa do reset |
| 0x84 | `PLLI2SCFGR` | PLL de áudio (módulo 17) |
| 0x8C | `DCKCFGR` | clocks do I2S, SAI, timers |

### `RCC_PLLCFGR`
| Bits | Nome | Faixa permitida / significado |
|---|---|---|
| 5:0 | PLLM | 2..63. A entrada do PLL (fonte ÷ M) **deve ficar entre 1 e 2 MHz**; 2 MHz dá menos ruído (*jitter*) |
| 14:6 | PLLN | 50..432. O VCO (entrada × N) **deve ficar entre 100 e 432 MHz** |
| 17:16 | PLLP | 00 = ÷2, 01 = ÷4, 10 = ÷6, 11 = ÷8 → SYSCLK ≤ 180 MHz |
| 22 | PLLSRC | 0 = HSI, 1 = HSE |
| 27:24 | PLLQ | 2..15 → o USB precisa de exatamente 48 MHz |
| 30:28 | PLLR | 2..7 → áudio |

### `RCC_CFGR`
| Bits | Nome | Valores |
|---|---|---|
| 1:0 | SW | 00 HSI, 01 HSE, 10 PLL_P, 11 PLL_R |
| 3:2 | SWS | fonte em uso (só leitura) |
| 7:4 | HPRE | divisor do AHB: 0xxx = ÷1 ... |
| 12:10 | PPRE1 | divisor do APB1: 0xx ÷1, 100 ÷2, **101 ÷4**, 110 ÷8, 111 ÷16 |
| 15:13 | PPRE2 | divisor do APB2: idem (**100 ÷2**) |
| 22:21 | MCO1 | saída no PA8: 00 HSI, 01 LSE, 10 HSE, 11 PLL |
| 26:24 | MCO1PRE | 0xx ÷1, 100 ÷2, 101 ÷3, 110 ÷4, 111 ÷5 |
| 29:27 | MCO2PRE | idem para o MCO2 |
| 31:30 | MCO2 | saída no PC9: 00 SYSCLK, 01 PLLI2S, 10 HSE, 11 PLL |

### `FLASH_ACR` (`0x4002_3C00`)
| Bits | Nome | |
|---|---|---|
| 3:0 | LATENCY | número de wait states (tabela da seção 5) |
| 8 | PRFTEN | prefetch |
| 9 | ICEN | cache de instruções |
| 10 | DCEN | cache de dados |
| 11/12 | ICRST/DCRST | limpa os caches (com eles desligados) |

---

## 12. MCO: vendo o clock com os próprios olhos

O pino **MCO** (*Microcontroller Clock Output*) coloca um clock interno, dividido, num pino do chip. É o jeito definitivo de conferir se a configuração deu certo: basta um osciloscópio ou frequencímetro.

| | Pino | Função alternativa | No exemplo |
|---|---|---|---|
| F072 MCO | PA8 | AF0 | SYSCLK ÷ 16 = 3 MHz |
| F446 MCO1 | PA8 | AF0 | PLL ÷ 5 = 36 MHz |
| F446 MCO2 | PC9 | AF0 | SYSCLK ÷ 5 = 36 MHz |

O pino precisa estar em modo "função alternativa" e com a velocidade máxima (`OSPEEDR = 11`), assuntos do módulo 05. Acima de uns 50 MHz a onda já aparece arredondada no osciloscópio.

---

## 13. Outros chips da família

| Chip | Máximo | Diferenças |
|---|---|---|
| STM32F030 | 48 MHz | PLL só a partir de HSI÷2 ou HSE; sem HSI48 |
| STM32F401 | 84 MHz | PLLCFGR igual, sem PLLR; sem over-drive; APB1 ≤ 42 MHz |
| STM32F411 | 100 MHz | idem; APB1 ≤ 50 MHz; 3 wait states a 100 MHz |
| STM32F407 | 168 MHz | sem over-drive; sem PLLR; APB1 ≤ 42 MHz, APB2 ≤ 84 MHz |

---

## Exercícios

1. Configure o F446 para 84 MHz com PLLP = ÷4 e ajuste os wait states. Confira no MCO.
2. No F072, escolha o `PLLMUL` para 32 MHz a partir do HSI÷2. Quantos wait states são necessários?
3. Ligue o **CSS** com o HSE e escreva um `NMI_Handler` que acende o LED. Teste desligando o HSE pelo depurador.
4. Por que o exemplo usa M = 8 e N = 180, e não M = 16 e N = 360, no F446? (Dica: limites da entrada e do VCO, e ruído.)
5. Calcule o PLLQ para obter 48 MHz para o USB no F446, mantendo o SYSCLK em 168 MHz.
