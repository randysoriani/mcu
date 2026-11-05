# Módulo 13 — I2C

O mestre I2C da TI é **orientado a comandos**: você escreve no `MCS` uma combinação de START/RUN/STOP/ACK e o hardware executa a fase inteira (START + endereço + byte + ACK...). É bem mais simples que o I2C v1 do STM32F4.

Pinos (os dois chips): **PB2 = I2C0SCL, PB3 = I2C0SDA**; PCTL **3** (123) / **2** (129). Pull-ups externos de 4k7 a 3V3 são necessários (módulos breakout costumam ter).

Código: [`src/i2c.c`](src/i2c.c), [`src/i2c.h`](src/i2c.h), [`src/main.c`](src/main.c) — scanner (como `i2cdetect`) e identificação de MPU6050, BMP/BME280, HMC5883L e EEPROM 24Cxx.

```sh
make MCU=123 flash && picocom -b 115200 /dev/ttyACM0
```

---

## 1. Pinos
```c
gpio_af(GPIOB, 2, 3);        /* SCL: push-pull (o controlador trata o clock stretching) */
gpio_af(GPIOB, 3, 3);        /* SDA */
GPIOB->ODR |= (1 << 3);      /* SDA open-drain */
```
O datasheet pede **SDA em open-drain e SCL não** — diferente do STM32, onde os dois são open-drain.

## 2. Registradores do mestre (base I2Cn)

| Offset | Reg. | |
|---|---|---|
| 0x000 | `MSA` | [7:1] endereço do escravo, [0] R/S (1 = leitura) |
| 0x004 | `MCS` | **escrita = comando**, **leitura = status** (abaixo) |
| 0x008 | `MDR` | dado |
| 0x00C | `MTPR` | período do SCL |
| 0x010–0x01C | `MIMR`, `MRIS`, `MMIS`, `MICR` | interrupção de fim de transação |
| 0x020 | `MCR` | 0 LPBK, **4 MFE** (habilita mestre), 5 SFE (escravo) |
| 0x02C | `MBMON` | estado real de SCL/SDA |

### MCS na escrita
| Bit | Nome |
|---|---|
| 0 | RUN |
| 1 | START |
| 2 | STOP |
| 3 | ACK (na leitura: 1 = mestre dá ACK, 0 = NACK) |
| 4 | HS (high-speed) |

### MCS na leitura
| Bit | Nome | |
|---|---|---|
| 0 | BUSY | transação em andamento |
| 1 | ERROR | houve erro |
| 2 | ADRACK | 1 = **endereço** sem ACK |
| 3 | DATACK | 1 = **dado** sem ACK |
| 4 | ARBLST | perdeu arbitragem |
| 5 | IDLE | |
| 6 | BUSBSY | barramento ocupado (outro mestre) |
| 7 | CLKTO | timeout de clock |

## 3. Velocidade
```
SCL = SYSCLK / (2 × (SCL_LP + SCL_HP) × (TPR + 1)),   SCL_LP = 6, SCL_HP = 4
TPR = SYSCLK / (20 × SCL) − 1
```
| | 100 kHz | 400 kHz |
|---|---|---|
| 123 (80 MHz) | 39 | 9 |
| 129 (120 MHz) | 59 | 14 |

## 4. Receitas de comando

| Operação | Sequência de `MCS` |
|---|---|
| escrever 1 byte | `START\|RUN\|STOP` (0x07) |
| escrever N | 1º `START\|RUN` (0x03), meio `RUN` (0x01), último `RUN\|STOP` (0x05) |
| ler 1 byte | `MSA` com R=1; `START\|RUN\|STOP` (0x07) |
| ler N | 1º `START\|RUN\|ACK` (0x0B), meio `RUN\|ACK` (0x09), último `RUN\|STOP` (0x05) → NACK + STOP |
| escrever registrador e ler (repeated start) | escrita sem STOP, depois `MSA` com R=1 e `START\|RUN...` |
| erro no meio | `MCS = STOP` (0x04), exceto se ARBLST |

Antes de cada byte de escrita, coloque o dado em `MDR`; depois de cada byte de leitura, leia `MDR`.

## 5. Esperando o fim: BUSY × MRIS
O jeito "óbvio" (`while (MCS & BUSY)`) tem uma armadilha: **o BUSY demora alguns ciclos para subir** depois da escrita no `MCS` (documentado no TM4C129). O laço pode terminar antes de começar. O driver do curso limpa `MICR`, envia o comando e espera **`MRIS` bit 0** (fim de transação, setado mesmo com a interrupção desabilitada), depois lê o status em `MCS`.

## 6. Scanner
`i2c_probe()` faz uma leitura de 1 byte (inofensiva para quase todos os dispositivos) e verifica `ADRACK`. Endereços 0x00–0x07 e 0x78–0x7F são reservados.

## 7. Recursos extras
- **Glitch filter** e **FIFO de 8 bytes + modo burst** no TM4C129 (`MBLEN`, `MBCNT`) — enviam vários bytes por comando, com uDMA.
- **Modo escravo** (offset 0x800) e loopback interno (`MCR.LPBK`).
- **Multi-mestre**: `ARBLST`, `BUSBSY`.

---

## Exercícios
1. Leia o acelerômetro do MPU6050 (acorde-o: registrador 0x6B = 0; leia 6 bytes a partir de 0x3B).
2. Implemente 400 kHz (`MTPR`) e meça com o analisador lógico.
3. Grave e leia uma página da EEPROM 24C32 (endereço de 16 bits, espera de 5 ms após a escrita).
