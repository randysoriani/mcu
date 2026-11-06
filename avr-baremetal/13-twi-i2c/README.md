# Módulo 13 — TWI (I2C)

> **Objetivo deste módulo:** entender o barramento I2C (que a Atmel chama de **TWI**, *Two-Wire Interface*): dois fios, vários dispositivos, cada um com um endereço. O exemplo varre o barramento e lê o registrador de identificação de sensores comuns (MPU-6050, BMP280/BME280).

Código:
- [`m328p/main.c`](m328p/main.c) — TWI conduzido passo a passo pelos **códigos de status**
- [`uc3a/main.c`](uc3a/main.c) — TWI do UC3, orientado a transação

Montagem: SDA e SCL do sensor nos pinos indicados, GND comum e **resistores de pull-up de 4,7 kΩ** no SDA e no SCL para a tensão do sensor (muitos módulos já os têm).

| | SDA | SCL |
|---|---|---|
| Arduino Uno | A4 (PC4) | A5 (PC5) |
| EVK1100 | PA29 | PA30 |

---

## 1. Como o I2C funciona

### 1.1 Dois fios, muitos dispositivos
```
 VCC ─┬─────────┬──── (pull-ups)
     [R]       [R]
 SDA ─┴──┬──────┼──────┬──────
 SCL ────┼──┬───┴──┬───┼──┬───
       mestre    sensor1  sensor2
       (AVR)     (0x68)   (0x76)
```
- **SCL**: clock, gerado pelo mestre (100 kHz padrão, 400 kHz rápido).
- **SDA**: dados, nos dois sentidos.
- **Dreno aberto**: ninguém força nível 1; os dispositivos só **puxam para 0** ou soltam. Os resistores de pull-up fazem o 1. Assim, dois dispositivos falando ao mesmo tempo não entram em curto. Sem os pull-ups, **nada funciona**.

### 1.2 Uma transação
```
 START | endereço (7 bits) + R/W | ACK | dado | ACK | ... | STOP
```
- **START**: SDA desce com SCL alto. **STOP**: SDA sobe com SCL alto. São as únicas vezes que o SDA muda com SCL alto.
- **Endereço**: 7 bits + 1 bit de direção (0 = escrever, 1 = ler). Ex.: MPU-6050 = 0x68 → byte `0xD0` para escrever, `0xD1` para ler.
- **ACK**: após cada byte, quem recebeu puxa o SDA para 0 por um bit ("recebi"). **NACK** (deixar em 1) = "ninguém aqui" ou "não quero mais".

### 1.3 Ler um registrador de um sensor (o padrão de quase todos)
```
 START  0xD0 (0x68+W)  ACK  0x75 (registrador)  ACK
 START repetido  0xD1 (0x68+R)  ACK  [dado]  NACK  STOP
```
O "START repetido" muda a direção sem soltar o barramento.

Analogia: telefonar para um ramal (endereço), pedir uma pasta (registrador) e pedir que leiam o conteúdo.

---

## 2. ATmega328P: TWI por códigos de status

### 2.1 Registradores
| Registrador | Endereço | Função |
|---|---|---|
| **TWBR** | 0xB8 | divisor da velocidade |
| **TWSR** | 0xB9 | bits 7:3 = **código de status**; bits 1:0 = prescaler |
| **TWAR** | 0xBA | endereço próprio (modo escravo) |
| **TWDR** | 0xBB | dado / endereço a enviar ou byte recebido |
| **TWCR** | 0xBC | **TWINT** (7): passo concluído (limpa escrevendo 1!); **TWEA** (6): responder ACK; **TWSTA** (5): gerar START; **TWSTO** (4): gerar STOP; **TWEN** (2): liga; TWIE (0): interrupção |

### 2.2 Velocidade
```
SCL = F_CPU / (16 + 2 × TWBR × prescaler)
100 kHz: TWBR = (16 MHz / 100 kHz − 16) / 2 = 72
```

### 2.3 O jeito AVR: um passo de cada vez
Cada ação é iniciada escrevendo no `TWCR` **com TWINT = 1** (o que limpa a flag e libera o hardware). Quando a ação termina, o hardware põe TWINT = 1 e deixa em `TWSR` um código que diz **exatamente o que aconteceu**:

| Código | Significado |
|---|---|
| 0x08 | START enviado |
| 0x10 | START repetido enviado |
| 0x18 / 0x20 | endereço + W enviado; recebeu ACK / NACK |
| 0x28 / 0x30 | byte enviado; ACK / NACK |
| 0x38 | perdeu a arbitragem (outro mestre) |
| 0x40 / 0x48 | endereço + R enviado; ACK / NACK |
| 0x50 / 0x58 | byte recebido; respondemos ACK / NACK |

```c
TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);   /* gera START */
while (!(TWCR & (1 << TWINT))) { }                  /* espera o passo */
if ((TWSR & 0xF8) != 0x08) erro();                  /* confere o código */
TWDR = (0x68 << 1) | 0;                             /* endereço + W */
TWCR = (1 << TWINT) | (1 << TWEN);
...
```
Para receber: `TWEA = 1` responde ACK (quero mais), `TWEA = 0` responde NACK (último byte). O STOP é `TWINT | TWSTO | TWEN`; o hardware limpa TWSTO quando termina.

A varredura do exemplo tenta cada endereço de 0x08 a 0x77 e vê se o código é 0x18 (alguém respondeu ACK).

Sempre use **timeout** nos laços de espera: um sensor travado segurando o SDA em 0 trava o programa. (Recuperação: gerar 9 pulsos de SCL manualmente pelo GPIO.)

---

## 3. AT32UC3A: TWI orientado a transação

Base `0xFFFF_2C00`. O hardware gera START, endereço, endereço interno e STOP sozinho.

| Offset | Registrador | Função |
|---|---|---|
| 0x00 | **CR** | START (0), STOP (1), MSEN (2), MSDIS (3), SVDIS (5), SWRST (7) |
| 0x04 | **MMR** | IADRSZ (9:8): bytes de endereço interno (registrador); **MREAD** (12); **DADR** (22:16): endereço do escravo |
| 0x0C | **IADR** | endereço interno (o registrador do sensor) |
| 0x10 | **CWGR** | velocidade: CLDIV, CHDIV, CKDIV |
| 0x20 | **SR** | **TXCOMP** (0), **RXRDY** (1), TXRDY (2), **NACK** (8) |
| 0x30 / 0x34 | RHR / THR | dado recebido / a enviar |

Leitura de um registrador:
```c
TWI->MMR  = (0x68 << 16) | MREAD | (1 << 8);   /* escravo 0x68, leitura, 1 byte de endereço interno */
TWI->IADR = 0x75;                              /* o registrador */
TWI->CR   = START | STOP;                      /* 1 byte: START e STOP juntos */
espera RXRDY; valor = TWI->RHR; espera TXCOMP;
```
O hardware faz sozinho: START, 0xD0, 0x75, START repetido, 0xD1, lê, NACK, STOP. Para vários bytes, dê o STOP **antes de ler o penúltimo**, para o hardware saber qual recebe o NACK.

Velocidade: `T_baixo = T_alto = (CLDIV × 2^CKDIV + 4) / PBA` → CLDIV = 236 para 100 kHz a 48 MHz.

---

## Exercícios
1. Leia a aceleração do MPU-6050 (acorde-o escrevendo 0 no registrador 0x6B; dados em 0x3B a 0x40).
2. Leve o barramento para 400 kHz (TWBR = 12) e meça com um analisador lógico.
3. Reescreva o driver do ATmega usando a interrupção `TWI_vect` e uma máquina de estados.
4. Configure o ATmega como **escravo** (TWAR) e faça dois Arduinos conversarem.
