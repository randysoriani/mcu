# Módulo 04 — Clock e fuses

> **Objetivo deste módulo:** entender o que é o clock e de onde ele vem nos dois chips. No ATmega328P a fonte de clock é escolhida pelos **fuses**, configurações permanentes gravadas pelo programador — um conceito típico do AVR que não existe nos outros cursos. No AT32UC3A, o próprio programa liga o cristal e o PLL.

Código (autocontido):
- [`m328p/main.c`](m328p/main.c) — lê e imprime os fuses e muda o divisor CLKPR (÷1, ÷2, ÷4, ÷8): o mesmo laço fica cada vez mais lento. Serial a **9600 bps**.
- [`uc3a/main.c`](uc3a/main.c) — RC de 115 kHz → cristal de 12 MHz → PLL a 48 MHz.

```sh
make MCU=m328p flash && picocom -b 9600 /dev/ttyACM0
```

---

## 1. O que é o clock

Os circuitos digitais só avançam a cada pulso de um sinal quadrado, o **clock**:
```
clock  ┌─┐ ┌─┐ ┌─┐ ┌─┐
      ─┘ └─┘ └─┘ └─┘ └─   a cada pulso, a CPU dá um passo
```
Analogia: o metrônomo de uma orquestra. O AVR de 8 bits executa quase toda instrução em **1 pulso**: a 16 MHz, ~16 milhões de instruções por segundo. A serial, os timers e o ADC também contam pulsos de clock, então **tudo que envolve tempo depende da frequência correta**.

Fontes possíveis:

| Tipo | Vantagem | Desvantagem |
|---|---|---|
| oscilador **RC interno** | sem peças externas | impreciso (±1–10%), varia com temperatura |
| **cristal** externo | preciso | precisa de peças na placa |
| sinal de clock externo | vem de outro circuito | idem |

---

## 2. ATmega328P

### 2.1 Fontes de clock
| Fonte | Frequência | Onde |
|---|---|---|
| RC interno calibrado | 8 MHz (±10%, calibrável por `OSCCAL`) | dentro do chip |
| RC de 128 kHz | 128 kHz | dentro do chip; também é o clock do watchdog |
| **Cristal externo** | até 20 MHz | **o Arduino Uno tem um de 16 MHz** (na verdade um ressonador cerâmico) |
| Cristal de baixa frequência | 32,768 kHz | relógios |
| Clock externo | qualquer | pino XTAL1 |

O ATmega328P **não tem PLL**: a frequência é a da fonte, dividida ou não pelo `CLKPR`.

### 2.2 Fuses: configurações gravadas no chip

**O que são:** três bytes de configuração (**lfuse**, **hfuse**, **efuse**) guardados numa área especial do chip, separada da Flash. Eles definem coisas que precisam valer **antes** do programa começar: qual oscilador usar, se há bootloader, quanto tempo esperar a alimentação estabilizar. O programa **não consegue mudá-los**; só um programador (ISP, módulo 00) pode gravá-los.

**Atenção à lógica invertida:** um bit de fuse **programado vale 0**, e **não programado vale 1**.

#### lfuse (fuse baixo)
| Bit | Nome | Significado |
|---|---|---|
| 7 | CKDIV8 | 0 = começa com o clock dividido por 8 |
| 6 | CKOUT | 0 = coloca o clock no pino PB0 (para medir) |
| 5:4 | SUT1:0 | tempo de partida (espera a alimentação/oscilador estabilizar) |
| 3:0 | CKSEL3:0 | **fonte de clock**: `0010` RC 8 MHz, `0011` RC 128 kHz, `0000` clock externo, `1111`–`1000` cristal |

#### hfuse (fuse alto)
| Bit | Nome | Significado |
|---|---|---|
| 7 | RSTDISBL | 0 = o pino RESET vira um pino comum (**depois disso o ISP não funciona mais!**) |
| 6 | DWEN | 0 = liga o debugWIRE (depuração pelo pino RESET, módulo 21) |
| 5 | SPIEN | 0 = gravação por ISP permitida (**nunca desprograme**) |
| 4 | WDTON | 0 = watchdog sempre ligado |
| 3 | EESAVE | 0 = a EEPROM é preservada quando o chip é apagado |
| 2:1 | BOOTSZ1:0 | tamanho da seção de boot (módulo 20) |
| 0 | BOOTRST | 0 = o reset vai para o bootloader em vez do endereço 0 |

#### efuse (fuse estendido)
| Bits | Nome | Significado |
|---|---|---|
| 2:0 | BODLEVEL | detector de queda de tensão: `111` desligado, `110` 1,8 V, `101` 2,7 V, `100` 4,3 V |

#### Valores típicos
| | lfuse | hfuse | efuse |
|---|---|---|---|
| ATmega328P novo de fábrica | `0x62` (RC 8 MHz ÷8 = 1 MHz) | `0xD9` | `0xFF` |
| **Arduino Uno** | **`0xFF`** (cristal, sem ÷8) | **`0xDE`** (bootloader de 512 B, BOOTRST) | **`0xFD`** (BOD 2,7 V; às vezes lido como `0x05`) |

#### Lendo e gravando (precisa de programador ISP)
```sh
avrdude -p m328p -c usbasp -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h
avrdude -p m328p -c usbasp -U lfuse:w:0xFF:m
```
Use uma calculadora de fuses (busque "AVR fuse calculator") e confira três vezes antes de gravar.

> **Como "tijolar" um ATmega:** gravar `RSTDISBL = 0` (perde o ISP), `SPIEN = 1` (idem), ou escolher um cristal que a placa não tem (o chip fica sem clock). Só um programador de **alta tensão** recupera. No Arduino, os fuses errados mais comuns são recuperáveis ligando um clock externo ao XTAL1.

#### Lendo os fuses pelo próprio programa
O programa deste módulo lê os fuses com uma sequência especial (datasheet, *Reading the Fuse and Lock Bits from Software*): escrever `BLBSET | SPMEN` no `SPMCSR` e executar `LPM` em até 3 ciclos, com Z = 0 (lfuse), 1 (lock), 2 (efuse) ou 3 (hfuse):
```c
__asm__ __volatile__ (
    "sts 0x57, %1 \n\t"   /* SPMCSR = BLBSET | SPMEN */
    "lpm %0, Z    \n\t"   /* lê o fuse escolhido por Z */
    : "=r"(r) : "r"(0x09), "z"(z));
```

### 2.3 O divisor CLKPR
O único controle de clock que o programa tem é o **divisor** `CLKPR` (`0x61`): divide a fonte por 1, 2, 4, ..., 256. Serve para economizar energia rodando mais devagar. Ele é protegido por uma **sequência temporizada**, para não ser mudado por acidente:
```c
cli();                    /* nenhuma interrupção pode atrasar os próximos passos */
CLKPR = (1 << CLKPCE);    /* 1) destrava: vale por 4 ciclos */
CLKPR = 0;                /* 2) novo divisor: 0 = ÷1, 1 = ÷2, 2 = ÷4, 3 = ÷8 ... */
```
Se o fuse CKDIV8 estiver programado, o chip começa em ÷8, e o `clock_init()` do curso volta para ÷1.

Depois de mudar o clock, **tudo que depende dele precisa ser recalculado**: o exemplo reconfigura a serial (`UBRR`) a cada mudança.

---

## 3. AT32UC3A0512

### 3.1 Fontes de clock
| Fonte | Frequência |
|---|---|
| RC interno (**clock do reset**) | ~115 kHz |
| OSC0 / OSC1 (cristal) | 0,4 a 20 MHz — **12 MHz na EVK1100** |
| OSC32 | 32,768 kHz (relógio) |
| PLL0 / PLL1 | multiplicam a frequência do OSC0/OSC1 |

### 3.2 O PLL
O **PLL** multiplica a frequência do cristal (analogia: a marcha de uma bicicleta):
```
f_VCO = f_OSC × (PLLMUL + 1) / PLLDIV         (com PLLDIV > 0)
12 MHz × 8 / 1 = 96 MHz   ──► saída ÷2 (PLLOPT bit 1) ──► 48 MHz
```
`PLLOPT` bit 0 escolhe a faixa do VCO (1 = 80–180 MHz). Depois de ligar o PLL, espera-se o bit **LOCK0** ("travado").

### 3.3 Wait states
A Flash do UC3A não acompanha a CPU acima de **33 MHz**: é preciso 1 ciclo de espera (**FWS = 1** em `FLASHC->FCR`) **antes** de acelerar. Sem isso, a CPU lê instruções erradas.

### 3.4 A sequência (`common/uc3a/clock.c`)
```c
PM->OSCCTRL0 = (7 << 0) | (6 << 8);     /* 1) OSC0: cristal > 8 MHz, partida lenta */
PM->MCCTRL  |= PM_MCCTRL_OSC0EN;        /*    liga o OSC0 */
while (!(PM->POSCSR & PM_POSCSR_OSC0RDY)) { }
PM->PLL0 = (16 << 24) | (7 << 16) | (1 << 8) | (3 << 2) | 1;   /* 2) PLL: x8, /1, faixa 80-180, saída /2, liga */
while (!(PM->POSCSR & PM_POSCSR_LOCK0)) { }
FLASHC->FCR |= FLASHC_FCR_FWS;          /* 3) 1 wait state ANTES de passar de 33 MHz */
PM->CKSEL = 0;                          /* 4) CPU, HSB, PBA, PBB sem divisor */
PM->MCCTRL = (PM->MCCTRL & ~3) | 2;     /* 5) clock principal = PLL0 */
```

### 3.5 Registradores do PM usados
| Registrador | Campos |
|---|---|
| `MCCTRL` (0x00) | MCSEL[1:0]: 0 RC, 1 OSC0, 2 PLL0; OSC0EN (bit 2) |
| `CKSEL` (0x04) | divisores da CPU, HSB, PBA e PBB (cada um: bit DIV + 3 bits SEL → ÷2^(SEL+1)) |
| `PLL0` (0x20) | PLLEN 0, PLLOSC 1, PLLOPT 4:2, PLLDIV 11:8, PLLMUL 19:16, PLLCOUNT 29:24 |
| `OSCCTRL0` (0x28) | MODE 2:0 (7 = cristal > 8 MHz), STARTUP 10:8 |
| `POSCSR` (0x54) | LOCK0 (bit 0), OSC0RDY (bit 7) |

Não há fuses de clock no UC3: tudo é configurado pelo programa.

---

## Exercícios
1. No ATmega, mude `CLKPR` para ÷16 e recalcule o `UBRR` para 9600 bps: ainda funciona? Calcule o erro.
2. Leia os fuses do seu Uno pelo programa e interprete cada bit com as tabelas acima.
3. Ligue o fuse CKOUT (com um ISP) e meça 16 MHz no pino 8 com um osciloscópio. Depois desligue.
4. No UC3, configure o PLL para 60 MHz (atenção ao FWS e ao limite de 66 MHz).
