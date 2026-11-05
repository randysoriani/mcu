# Módulo 04 — Clock (System Control)

> **Objetivo deste módulo:** entender o que é o clock, de onde ele vem, como o chip passa dos 16 MHz do reset para 80 MHz (TM4C123) ou 120 MHz (TM4C129) e por que nenhum periférico funciona antes de "ligar o clock" dele. Tudo isso fica no bloco **System Control** (`0x400F_E000`).

Código (autocontido: não usa a biblioteca do curso):
- [`123/main.c`](123/main.c) — 16 MHz → 400 ÷ `PLL_DIV` MHz (padrão: 80 MHz)
- [`129/main.c`](129/main.c) — 16 MHz → 480 ÷ (`PSYSDIV` + 1) MHz (padrão: 120 MHz), e coloca o clock dividido por 12 no pino PQ4

```sh
make MCU=123 flash
make clean && make MCU=129 EXTRA=-DPSYSDIV=7 flash      # 60 MHz
```
O que observar: o LED pisca 5 vezes devagar (clock de reset) e depois, **com exatamente o mesmo laço de atraso no código**, muito mais rápido. A única coisa que mudou foi a velocidade do clock.

A versão usada pelos outros módulos está em [`common/123/clock.c`](../common/123/clock.c) e [`common/129/clock.c`](../common/129/clock.c).

---

## 1. O que é o clock

O processador e os periféricos são circuitos digitais **síncronos**: só avançam um passo a cada pulso de um sinal quadrado chamado **clock**.

```
clock   ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐
       ─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─
        1   2   3   4   5   6      <- a cada pulso, o circuito dá um passo
```

Analogia: é o **metrônomo** de uma orquestra. Cada músico (processador, timer, serial...) toca no ritmo dele. Metrônomo mais rápido, música mais rápida.

- A frequência é medida em hertz (pulsos por segundo): 80 MHz = 80 milhões de pulsos por segundo.
- O Cortex-M executa, grosso modo, uma instrução simples por pulso.
- Os periféricos também dependem do clock: o baud rate da serial, a contagem dos timers e a velocidade do SPI são **calculados a partir dele**. Se o clock estiver errado, tudo que depende de tempo fica errado.

Por que não rodar sempre no máximo? Porque o consumo de energia cresce com a frequência.

---

## 2. De onde vem o clock: as fontes

Um clock precisa de um **oscilador**, um circuito que gera a onda quadrada. Há dois tipos:

| Tipo | Como funciona | Vantagem | Desvantagem |
|---|---|---|---|
| **RC interno** | resistor + capacitor dentro do chip | sempre disponível, sem peças externas | menos preciso (≈1%), varia com a temperatura |
| **Cristal externo** | um cristal de quartzo vibra numa frequência muito exata | muito preciso | precisa de peças na placa e de alguns milissegundos para estabilizar |

As fontes do TM4C:

| Fonte | Frequência | Para que serve |
|---|---|---|
| **PIOSC** (*Precision Internal Oscillator*) | 16 MHz, ±1% (calibrado de fábrica) | o clock após o reset; a UART e o ADC também podem usá-lo direto |
| **MOSC** (*Main Oscillator*) | cristal externo: **16 MHz** na LaunchPad do 123, **25 MHz** na do 129 | fonte precisa para o PLL |
| LFIOSC | ~30 kHz, impreciso | baixo consumo, watchdog |
| Cristal de 32,768 kHz do módulo de hibernação | 32 768 Hz | relógio de tempo real (módulo 20). 32 768 = 2¹⁵: dividindo por 2 quinze vezes dá exatamente 1 Hz |
| **PLL** | 400 MHz (123) / até 480 MHz (129) | gera as frequências altas (seção 3) |

---

## 3. O PLL: como 16 MHz vira 80 ou 120 MHz

Para chegar a frequências altas, o chip usa um **PLL** (*Phase-Locked Loop*): um circuito que **multiplica** uma frequência de entrada, mantendo a mesma precisão dela.

Analogia: é a **marcha de uma bicicleta**. A pedalada (o cristal) é lenta e constante; a relação das engrenagens (o PLL) faz a roda girar muito mais rápido.

A saída do PLL é alta demais para o processador, então ela passa por um **divisor** antes de virar o clock do sistema (**SYSCLK**):

```
TM4C123:  cristal 16 MHz ──► PLL (fixo em 400 MHz) ──► ÷5 ──► 80 MHz
TM4C129:  cristal 25 MHz ──► ÷5 ──► 5 MHz ──► ×96 ──► 480 MHz ──► ÷4 ──► 120 MHz
```

Depois de ligar o PLL, é preciso **esperar ele "travar"** (estabilizar). Um bit de status avisa quando ele está pronto.

---

## 4. Wait states: a Flash não acompanha o processador

A Flash é mais lenta que o núcleo: ela precisa de algumas dezenas de nanossegundos para entregar um dado. Acima de certa frequência, o processador pediria uma instrução e ela ainda não estaria pronta. A solução são os **wait states** (estados de espera): ciclos em que o processador aguarda a Flash.

- **TM4C123**: a Flash e o seu controlador ajustam os wait states e a busca adiantada de instruções (*prefetch*) **sozinhos**. Não há nada a configurar.
- **TM4C129**: o software precisa configurar os wait states no registrador **`MEMTIM0`**, de acordo com a frequência (seção 7).

---

## 5. Ligando o clock de cada periférico

Além de gerar o clock, o System Control tem uma "chave" por periférico. **Depois do reset todas estão desligadas.** Para usar um periférico, o primeiro passo é sempre ligar o bit dele no registrador `RCGC...` correspondente e esperar o bit no registrador `PR...` (módulo 01):

```c
SYSCTL_RCGCGPIO |= (1U << 5);                 /* liga o clock da porta F */
while (!(SYSCTL_PRGPIO & (1U << 5))) { }      /* espera ela ficar pronta */
```

No TM4C, mexer num periférico sem clock **trava o programa** com um erro (HardFault). É o erro mais comum de quem começa.

Diferente de outros microcontroladores, no TM4C **todos os periféricos recebem o próprio SYSCLK**: não há barramentos com divisores separados. As exceções configuráveis:
- UART e SSI podem usar o PIOSC em vez do SYSCLK (registrador `CC` do periférico), o que permite mudar o SYSCLK sem recalcular o baud rate;
- o ADC pode usar o PIOSC (registrador `ADCCC` no 129);
- o PWM tem um divisor próprio de ÷2 a ÷64 (no `RCC` do 123, no `PWMCC` do 129);
- o watchdog WDT1 usa o PIOSC.

---

## 6. TM4C123: registradores RCC e RCC2

O 123 herdou o esquema da família anterior da TI (Stellaris): dois registradores, `RCC` e `RCC2`, em que o segundo tem campos maiores e, quando habilitado, tem precedência.

```
                      ┌── ÷2 ──┐
MOSC 16 MHz ──► PLL 400 MHz ───┴── DIV400 ──► ÷(SYSDIV2:SYSDIV2LSB + 1) ──► SYSCLK
PIOSC / MOSC ─────────────────────── (BYPASS = 1: ignora o PLL) ─────────►
```

### `RCC` (offset `0x060`), campos usados
| Bits | Nome | Significado |
|---|---|---|
| 0 | MOSCDIS | 1 = oscilador principal (cristal) desligado |
| 5:4 | OSCSRC | fonte do oscilador (substituído pelo OSCSRC2 quando USERCC2 = 1) |
| 10:6 | **XTAL** | frequência do cristal da placa: `0x15` = **16 MHz**, `0x19` = 25 MHz |
| 11 | BYPASS | 1 = o PLL é ignorado |
| 13 | PWRDN | 1 = PLL desligado |
| 19:17 | PWMDIV | divisor do clock do PWM (módulo 10) |
| 20 | USEPWMDIV | usa o divisor do PWM |
| 22 | USESYSDIV | usa o divisor do sistema |

### `RCC2` (offset `0x070`)
| Bits | Nome | Significado |
|---|---|---|
| 6:4 | OSCSRC2 | 000 MOSC (cristal), 001 PIOSC, 010 PIOSC÷4, 011 LFIOSC, 111 32 kHz |
| 11 | BYPASS2 | 1 = PLL ignorado |
| 13 | PWRDN2 | 1 = PLL desligado |
| 22 | SYSDIV2LSB | bit extra do divisor (vale quando DIV400 = 1) |
| 28:23 | SYSDIV2 | divisor do sistema |
| 30 | DIV400 | 1 = o divisor age direto sobre os 400 MHz (permite chegar a 80 MHz) |
| 31 | **USERCC2** | 1 = os campos do RCC2 valem no lugar dos do RCC |

### O código, passo a passo (`123/main.c`)
```c
SYSCTL_RCC2 |= SYSCTL_RCC2_USERCC2;                 /* 1. usa o RCC2 */
SYSCTL_RCC2 |= SYSCTL_RCC2_BYPASS2;                 /* 2. roda direto do oscilador enquanto configura */
SYSCTL_RCC   = (SYSCTL_RCC & ~(XTAL | MOSCDIS))      /* 3. cristal de 16 MHz, ligado */
             | SYSCTL_RCC_XTAL_16MHZ;
SYSCTL_RCC2 &= ~SYSCTL_RCC2_OSCSRC2_Msk;            /*    fonte = cristal (MOSC) */
SYSCTL_RCC2 &= ~SYSCTL_RCC2_PWRDN2;                 /* 4. liga o PLL */
SYSCTL_RCC  |= SYSCTL_RCC_USESYSDIV;                /* 5. usa o divisor... */
SYSCTL_RCC2 |= SYSCTL_RCC2_DIV400;                  /*    ...direto sobre os 400 MHz */
SYSCTL_RCC2  = (SYSCTL_RCC2 & ~SYSDIV2_Msk) | (4 << 22);   /*    valor 4 -> ÷5 -> 80 MHz */
while (!(SYSCTL_RIS & SYSCTL_RIS_PLLLRIS)) { }      /* 6. espera o PLL travar */
SYSCTL_RCC2 &= ~SYSCTL_RCC2_BYPASS2;                /* 7. passa a usar o PLL */
```

O passo 2 (BYPASS) é o que torna a troca segura: durante a configuração o chip continua rodando do oscilador; só no passo 7, com o PLL travado, a saída dele passa a ser usada.

Divisor = valor + 1: 4 → 80 MHz, 5 → 66,7 MHz, 7 → 50 MHz, 9 → 40 MHz. Valores abaixo de 4 passariam do máximo do chip.

---

## 7. TM4C129: RSCLKCFG, PLLFREQ e MEMTIM0

O 129 tem um sistema de clock novo, com PLL programável e wait states configurados pelo software.

```
MOSC 25 MHz ─► ÷(N+1) = ÷5 ─► 5 MHz ─► ×(MINT + MFRAC/1024) = ×96 ─► VCO 480 MHz ─► ÷(PSYSDIV+1) = ÷4 ─► 120 MHz
```
O VCO (*Voltage-Controlled Oscillator*) é a parte do PLL que gera a frequência alta.

| Registrador | Offset | Campos |
|---|---|---|
| `MOSCCTL` | 0x07C | bit 2 NOXTAL (1 = não há cristal), bit 3 PWRDN (1 = desligado), bit 4 OSCRNG (1 = cristal acima de 10 MHz) |
| `RIS` | 0x050 | bit 8 MOSCPUPRIS (o cristal estabilizou) |
| `RSCLKCFG` | 0x0B0 | 9:0 PSYSDIV (divisor), 23:20 OSCSRC, 27:24 PLLSRC (3 = cristal), 28 USEPLL, 30 NEWFREQ, 31 MEMTIMU |
| `PLLFREQ0` | 0x160 | 9:0 MINT (parte inteira do multiplicador), 19:10 MFRAC (parte fracionária), 23 PLLPWR (liga o PLL) |
| `PLLFREQ1` | 0x164 | 4:0 N, 12:8 Q (divisores de entrada) |
| `PLLSTAT` | 0x168 | bit 0 LOCK (PLL travado) |
| `MEMTIM0` | 0x0C0 | 3:0 FWS e 9:6 FBCHT (Flash); 19:16 EWS e 25:22 EBCHT (EEPROM); bits 4 e 20 devem ser 1 |
| `DIVSCLK` | 0x148 | 7:0 DIV, 17:16 SRC (0 SYSCLK, 1 PIOSC, 2 cristal), 31 EN: põe um clock no pino PQ4 |

### Wait states (tabela "MEMTIM0 Register Configuration versus Frequency" do datasheet)

| SYSCLK | FWS / EWS (ciclos de espera) | FBCHT / EBCHT |
|---|---|---|
| até 16 MHz | 0 | 0 |
| até 40 MHz | 1 | 2 |
| até 60 MHz | 2 | 3 |
| até 80 MHz | 3 | 4 |
| até 100 MHz | 4 | 5 |
| até 120 MHz | 5 | 6 |

### O código, passo a passo (`129/main.c`)
```c
/* 1. Liga o cristal (> 10 MHz) e espera estabilizar */
SYSCTL_MOSCCTL = (SYSCTL_MOSCCTL & ~(NOXTAL | PWRDN)) | OSCRNG;
while (!(SYSCTL_RIS & SYSCTL_RIS_MOSCPUPRIS)) { }

/* 2. Cristal como fonte do oscilador e do PLL */
SYSCTL_RSCLKCFG = OSCSRC_MOSC | PLLSRC_MOSC;

/* 3. Multiplicador do PLL: 25 MHz ÷ 5 × 96 = 480 MHz; liga o PLL */
SYSCTL_PLLFREQ1 = 4;                    /* N = 4 -> ÷5 */
SYSCTL_PLLFREQ0 = PLLPWR | 96;          /* MINT = 96 */

/* 4. Wait states para a frequência FINAL (ainda não aplicados) */
SYSCTL_MEMTIM0 = memtim0_for(120000000);

/* 5. Aplica a configuração nova do PLL e espera travar */
SYSCTL_RSCLKCFG |= NEWFREQ;
while (!(SYSCTL_PLLSTAT & LOCK)) { }

/* 6. Passa a usar o PLL com divisor ÷4 e aplica os wait states AO MESMO TEMPO */
SYSCTL_RSCLKCFG = MEMTIMU | USEPLL | OSCSRC_MOSC | PLLSRC_MOSC | (3 << 0);
```

O bit `MEMTIMU` do passo 6 é o detalhe importante: os novos wait states só valem no instante em que o clock muda. Assim a Flash nunca fica com wait states insuficientes para a frequência em uso.

### DIVSCLK: vendo o clock no pino
O 129 pode colocar um clock interno, dividido, no pino **PQ4** (registrador `DIVSCLK`). O exemplo coloca SYSCLK ÷ 12 = 10 MHz, que pode ser medido com um osciloscópio. O 123 não tem esse recurso; a medida indireta é o tempo do pisca do LED ou o baud rate da serial.

---

## Exercícios

1. No 123, rode a 50 MHz (`PLL_DIV=8`), confira pelo tempo do pisca e calcule os novos valores do baud rate da serial (módulo 07) para 115200.
2. No 129, rode a 60 MHz e confira a frequência no pino PQ4 com um osciloscópio.
3. Por que no 123 basta mudar o divisor, enquanto no 129 é obrigatório mexer também no `MEMTIM0`?
4. Configure o 123 para rodar direto do cristal (16 MHz, PLL desligado) e compare o consumo no jumper JP4 da placa.
