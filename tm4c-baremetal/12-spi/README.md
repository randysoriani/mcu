# Módulo 12 — SPI (SSI)

A TI chama o SPI de **SSI** (Synchronous Serial Interface). Ele suporta três formatos de quadro (Freescale SPI, TI synchronous serial e Microwire), dados de 4 a 16 bits, FIFOs de 8 entradas e um **loopback interno**.

| | SSI | CLK | TX (MOSI) | RX (MISO) | CS (GPIO) | PCTL |
|---|---|---|---|---|---|---|
| 123 | SSI2 | PB4 | PB7 | PB6 | PB5 | 2 |
| 129 | SSI2 | PD3 | PD1 (XDAT0) | PD0 (XDAT1) | PD2 | 15 |

Código: [`src/spi.c`](src/spi.c), [`src/main.c`](src/main.c)
- **DEMO 1**: loopback interno (`CR1.LBM`) — testa o driver **sem nenhum fio**
- **DEMO 2**: memória W25Qxx (JEDEC ID, status, leitura)

```sh
make MCU=123 flash
make clean && make MCU=129 EXTRA=-DDEMO=2 flash
```

> 123: na LaunchPad, PB6/PB7 estão ligados a PD0/PD1 por resistores de 0 Ω (R9/R10). Não use PD0/PD1 como saída ao mesmo tempo (ou remova os resistores).

---

## 1. Registradores (base SSIn)

| Offset | Reg. | Bits |
|---|---|---|
| 0x000 | `CR0` | 3:0 **DSS** (bits − 1: 7 = 8 bits, 15 = 16), 5:4 **FRF** (0 Freescale, 1 TI, 2 Microwire), 6 **SPO** (CPOL), 7 **SPH** (CPHA), 15:8 **SCR** |
| 0x004 | `CR1` | 0 **LBM** (loopback), 1 **SSE** (habilita), 2 MS (1 = escravo), 4 EOT |
| 0x008 | `DR` | dado (escrita → FIFO TX, leitura ← FIFO RX) |
| 0x00C | `SR` | 0 TFE (TX vazia), 1 **TNF** (TX não cheia), 2 **RNE** (RX não vazia), 3 RFF, 4 **BSY** |
| 0x010 | `CPSR` | prescaler: **par**, 2..254 |
| 0x014–0x020 | `IM`, `RIS`, `MIS`, `ICR` | interrupções (FIFO, timeout, overrun) |
| 0x024 | `DMACTL` | 0 RXDMAE, 1 TXDMAE |
| 0xFC8 | `CC` | clock: 0 SYSCLK, 5 PIOSC |

## 2. Bit rate
```
SSInClk = SYSCLK / (CPSDVSR × (1 + SCR))
```
| | CPSDVSR | SCR | Resultado |
|---|---|---|---|
| 123 (80 MHz) | 8 | 1 | 5 MHz |
| 129 (120 MHz) | 12 | 1 | 5 MHz |

Máximo como mestre: SYSCLK/2 (40 MHz no 123, 60 MHz no 129 — limitado também pelos pinos).

## 3. Modos SPI (Freescale)
| Modo | SPO | SPH | Clock em repouso | Amostra na |
|---|---|---|---|---|
| 0 | 0 | 0 | baixo | subida |
| 1 | 0 | 1 | baixo | descida |
| 2 | 1 | 0 | alto | descida |
| 3 | 1 | 1 | alto | subida |

## 4. Transferência
```c
uint8_t spi_xfer(uint8_t out)
{
    while (!(SSI2->SR & SSI_SR_TNF)) { }   /* há espaço na FIFO de TX */
    SSI2->DR = out;
    while (!(SSI2->SR & SSI_SR_RNE)) { }   /* chegou o byte recebido  */
    return SSI2->DR;
}
```
SPI é full-duplex: cada byte enviado traz um de volta, que precisa ser lido (senão a FIFO de RX enche e dá overrun).

Antes de soltar o CS, espere **`BSY = 0`** — `TNF`/`TFE` só dizem que a FIFO aceitou o dado, não que o último bit saiu.

## 5. Sobre o pino FSS
O SSI tem um pino de *frame select* (SSInFss) controlado pelo hardware. No formato Freescale com SPH = 0 ele **pulsa entre cada byte**, o que quebra comandos de vários bytes da maioria dos chips (Flash, displays). Por isso o curso usa um **GPIO comum como CS**.

## 6. W25Qxx (DEMO 2)
| Comando | Código | Resposta |
|---|---|---|
| JEDEC ID | `0x9F` | fabricante (0xEF Winbond), tipo, capacidade (2^n bytes) |
| Read Status 1 | `0x05` | BUSY, WEL |
| Read Data | `0x03` + endereço 24 bits | dados |
| Write Enable | `0x06` | |
| Page Program | `0x02` + endereço + até 256 bytes | |
| Sector Erase 4 KB | `0x20` + endereço | |

---

## Exercícios
1. Grave uma página no W25Q (WREN, 0x02, espera BUSY) e leia de volta.
2. Use o SSI em 16 bits (DSS = 15) para um DAC SPI (MCP4921).
3. Envie um buffer de 1 KB por uDMA (`DMACTL.TXDMAE`, canal do SSI2 TX).
