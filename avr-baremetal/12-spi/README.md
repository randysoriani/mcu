# Módulo 12 — SPI

> **Objetivo deste módulo:** entender o barramento SPI — rápido, síncrono, com um fio de clock — e fazer transferências como mestre. É o barramento de cartões SD, displays, memórias Flash e do próprio **ISP** que grava o ATmega.

Código:
- [`m328p/main.c`](m328p/main.c) — mestre a 1 MHz, teste em loopback com um **jumper do pino 11 (MOSI) ao pino 12 (MISO)**
- [`uc3a/main.c`](uc3a/main.c) — SPI0 com **loopback interno** (sem fios)

---

## 1. Como o SPI funciona

Quatro fios:

| Fio | Nome | Sentido |
|---|---|---|
| **SCK** | clock | mestre → escravo |
| **MOSI** | *Master Out, Slave In* | mestre → escravo |
| **MISO** | *Master In, Slave Out* | escravo → mestre |
| **SS / CS** | seleção do escravo (ativo em 0) | mestre → escravo (um fio por escravo) |

A cada pulso de clock, **um bit sai e um bit entra ao mesmo tempo**. Por dentro são dois registradores de deslocamento ligados em anel:
```
   mestre                       escravo
 [b7 b6 ... b0] ──MOSI──► [b7 b6 ... b0]
       ▲                         │
       └─────────MISO────────────┘
 8 pulsos de SCK depois, os dois trocaram os bytes
```
Consequência: **não existe "só ler"**. Para receber um byte, o mestre envia um qualquer (normalmente 0xFF). Para só escrever, descarta o recebido.

Analogia: duas pessoas trocando cartas ao mesmo tempo por uma janela — toda entrega é também um recebimento.

### Modos (CPOL e CPHA)
| Modo | CPOL (clock parado em) | CPHA (amostra na) |
|---|---|---|
| 0 | 0 | 1ª borda (subida) |
| 1 | 0 | 2ª borda (descida) |
| 2 | 1 | 1ª borda (descida) |
| 3 | 1 | 2ª borda (subida) |

O datasheet do dispositivo diz o modo; o modo 0 é o mais comum.

---

## 2. ATmega328P

Pinos (fixos): **SCK = PB5 (pino 13)**, **MISO = PB4 (12)**, **MOSI = PB3 (11)**, **SS = PB2 (10)**. São os mesmos do conector ICSP. O LED "L" está no SCK: ele pisca fraco durante as transferências.

### 2.1 Registradores
| Registrador | Endereço | Bits |
|---|---|---|
| **SPCR** | 0x4C | SPIE (7): interrupção; **SPE** (6): liga; DORD (5): 1 = LSB primeiro; **MSTR** (4): mestre; **CPOL** (3); **CPHA** (2); **SPR1:0** (1:0): clock |
| **SPSR** | 0x4D | **SPIF** (7): transferência concluída; WCOL (6): escrita durante transferência; **SPI2X** (0): dobra o clock |
| **SPDR** | 0x4E | dado: escrever inicia a transferência; ler dá o byte recebido |

Clock (SPI2X : SPR1 : SPR0):

| 000 | 001 | 010 | 011 | 100 | 101 | 110 | 111 |
|---|---|---|---|---|---|---|---|
| ÷4 | ÷16 | ÷64 | ÷128 | ÷2 | ÷8 | ÷32 | ÷64 |

### 2.2 Inicialização e transferência
```c
DDRB |= (1 << 5) | (1 << 3) | (1 << 2);   /* SCK, MOSI, SS saídas; MISO entrada */
SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);   /* modo 0, ÷16 = 1 MHz */

uint8_t spi_xfer(uint8_t out)
{
    SPDR = out;                           /* começa */
    while (!(SPSR & (1 << SPIF))) { }     /* 8 bits depois... */
    return SPDR;                          /* ler SPSR e depois SPDR limpa o SPIF */
}
```
A 1 MHz um byte leva 8 µs; a 8 MHz (÷2), 1 µs — tão rápido que esperar em laço é melhor que interrupção.

### 2.3 A armadilha do SS
Em modo mestre, se o pino **SS (PB2)** estiver como **entrada** e receber nível baixo, o hardware entende que outro mestre assumiu o barramento: **zera o MSTR** e vira escravo, sem avisar (só marca SPIF). Solução: deixe o PB2 **sempre como saída** (e use-o como CS do seu dispositivo).

### 2.4 Chip select
O CS é um GPIO comum controlado pelo programa:
```c
PORTB &= ~(1 << 2);   /* seleciona */
spi_xfer(cmd); valor = spi_xfer(0xFF);
PORTB |= (1 << 2);    /* libera: muitos dispositivos só executam o comando aqui */
```

---

## 3. AT32UC3A: SPI0

Pinos (função A, confira na tabela de multiplexação): SCK = PA13, MISO = PA11, MOSI = PA12, NPCS0 = PA10. Base `0xFFFF_2400`.

| Offset | Registrador | Função |
|---|---|---|
| 0x00 | **CR** | SPIEN (0), SPIDIS (1), SWRST (7), LASTXFER (24) |
| 0x04 | **MR** | **MSTR** (0); PS (1): CS fixo/variável; MODFDIS (4); **LLB** (7): loopback interno; **PCS** (19:16): qual CS (0 = ativo; 1110 = NPCS0) |
| 0x08 | **RDR** | dado recebido |
| 0x0C | **TDR** | dado a enviar |
| 0x10 | **SR** | **RDRF** (0): chegou; **TDRE** (1): pode escrever; TXEMPTY (9) |
| 0x30–0x3C | **CSR0..3** | por chip select: CPOL (0), **NCPHA** (1), CSAAT (3), BITS (7:4): 8 a 16 bits, **SCBR** (15:8): clock = PBA/SCBR, DLYBS, DLYBCT |

Diferenças em relação ao ATmega:
- **4 chip selects em hardware** (NPCS0–3), cada um com seu modo e velocidade no `CSRn`: o SPI ativa o CS sozinho.
- **NCPHA** é o inverso do CPHA: modo 0 = `CPOL = 0, NCPHA = 1`.
- 8 a 16 bits por transferência.
- O **PDCA** (DMA, módulo 19) pode alimentar o SPI.

O exemplo usa `LLB = 1`: o MOSI é ligado internamente ao MISO, então o byte recebido é igual ao enviado.

---

## Exercícios
1. Leia o JEDEC ID de uma memória Flash SPI (comando 0x9F; o W25Q32 responde EF 40 16). Atenção: a Flash é 3,3 V e o Uno é 5 V — use um conversor de nível.
2. Meça com um analisador lógico os quatro modos e confira a tabela.
3. Escreva um driver para um display MAX7219 (matriz de LEDs).
4. Compare o tempo de 256 transferências a ÷2 e a ÷128.
