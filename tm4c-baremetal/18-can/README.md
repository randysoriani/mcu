# Módulo 18 — CAN (Bosch C_CAN)

Os TM4C têm dois controladores CAN 2.0A/B baseados no **Bosch C_CAN**. Diferente do bxCAN do STM32 (3 mailboxes de TX, 2 FIFOs de RX, bancos de filtros), o C_CAN tem **32 message objects** numa RAM interna; cada objeto é configurado como TX ou RX, com seu próprio ID e máscara (o filtro é o próprio objeto).

| | Controlador | RX | TX | PCTL |
|---|---|---|---|---|
| 123 | CAN0 | PE4 | PE5 | 8 |
| 129 | CAN1 | PB0 | PB1 | 7 |

(No 129 o CAN0 fica em PA0/PA1, que são a UART0 do console.)

Código: [`src/main.c`](src/main.c)
- **DEMO 1** (padrão): *loopback + silent* — o controlador recebe as próprias mensagens, sem tocar no barramento e sem transceiver
- **DEMO 2**: modo normal a 500 kbit/s (transceiver 3,3 V como SN65HVD230, outro nó, terminação de 120 Ω nas duas pontas)

```sh
make MCU=123 flash && picocom -b 115200 /dev/ttyACM0
make clean && make MCU=123 EXTRA=-DDEMO=2 flash
```

---

## 1. Registradores principais (base CANn)

| Offset | Reg. | |
|---|---|---|
| 0x000 | `CTL` | 0 **INIT**, 1 IE, 2 SIE, 3 EIE, 5 DAR (sem retransmissão), 6 **CCE** (libera BIT), 7 **TEST** |
| 0x004 | `STS` | 2:0 LEC (último erro), 3 TXOK, 4 RXOK, 5 EPASS, 6 EWARN, 7 BOFF |
| 0x008 | `ERR` | 7:0 TEC, 14:8 REC |
| 0x00C | **`BIT`** | 5:0 BRP, 7:6 SJW, 11:8 TSEG1, 14:12 TSEG2 (todos "valor − 1") |
| 0x014 | `TST` | 3 SILENT, 4 LBACK (só com CTL.TEST) |
| 0x018 | `BRPE` | extensão do prescaler |
| 0x020–0x048 | **IF1** | registradores de interface 1 |
| 0x080–0x0A8 | **IF2** | registradores de interface 2 |
| 0x100/0x104 | `TXRQ1/2` | objetos com transmissão pendente |
| 0x120/0x124 | `NWDA1/2` | objetos com dado novo |
| 0x140/0x144 | `MSGnINT` | interrupção pendente por objeto |
| 0x160/0x164 | `MSGnVAL` | objetos válidos |

### Interface IFn
| Reg. | Uso |
|---|---|
| `CRQ` | [5:0] número do objeto (1–32): escrever **dispara a cópia**; bit 15 BUSY |
| `CMSK` | o que copiar: 0 DATAB, 1 DATAA, 2 TXRQST/NEWDAT, 3 CLRINTPND, 4 CONTROL, 5 ARB, 6 MASK, **7 WRNRD** (1 = IF → objeto) |
| `MSK1/MSK2` | máscara de aceitação: bit 0 = "não importa"; MSK2 15 MXTD, 14 MDIR |
| `ARB1/ARB2` | ID: ARB2 15 **MSGVAL**, 14 **XTD**, 13 **DIR** (1 = TX), [12:0] ID alto |
| `MCTL` | 15 NEWDAT, 14 MSGLST, 13 INTPND, **12 UMASK**, 11 TXIE, 10 RXIE, 8 **TXRQST**, **7 EOB**, [3:0] DLC |
| `DA1/DA2/DB1/DB2` | dados (16 bits cada: bytes 0-1, 2-3, 4-5, 6-7) |

Codificação do ID:
- 11 bits: `ARB2[12:2] = ID`
- 29 bits: `ARB1 = ID[15:0]`, `ARB2[12:0] = ID[28:16]`, `XTD = 1`

## 2. Bit timing (500 kbit/s)
```
bit = 1 (sync) + TSEG1 + TSEG2  [tq]        tq = BRP / SYSCLK
```
| | SYSCLK | BRP | tq | TSEG1 | TSEG2 | Amostragem |
|---|---|---|---|---|---|---|
| 123 | 80 MHz | 10 | 125 ns | 13 | 2 | 87,5% |
| 129 | 120 MHz | 15 | 125 ns | 13 | 2 | 87,5% |

`BIT = (TSEG2-1)<<12 | (TSEG1-1)<<8 | (SJW-1)<<6 | (BRP-1)`, escrito com `CTL = INIT | CCE`.

## 3. Sequência
1. Clock, pinos (AFSEL+PCTL)
2. `CTL = INIT | CCE`; `BIT`, `BRPE`
3. Configura os objetos via IF (abaixo)
4. Teste: `CTL |= TEST`, `TST = LBACK | SILENT`
5. `CTL &= ~(INIT | CCE)`

**Objeto de RX que aceita tudo** (objeto 2):
```c
IF2.CMSK = WRNRD | MASK | ARB | CONTROL;
IF2.MSK1 = 0; IF2.MSK2 = 0;          /* máscara 0, MXTD = 0: aceita std e ext */
IF2.ARB1 = 0; IF2.ARB2 = MSGVAL;     /* DIR = 0 */
IF2.MCTL = UMASK | EOB | 8;
IF2.CRQ  = 2;  while (IF2.CRQ & BUSY);
```
**Transmitir** (objeto 1): `CMSK = WRNRD|ARB|CONTROL|DATAA|DATAB`, ID com `MSGVAL|DIR`, `MCTL = TXRQST|EOB|DLC`, dados, `CRQ = 1`.

**Receber**: se `NWDA1` bit 1 → `CMSK = ARB|CONTROL|DATAA|DATAB|CLRINTPND|TXRQST` (na leitura, TXRQST limpa o NEWDAT), `CRQ = 2`, lê ARB/MCTL/dados.

## 4. Filtros e FIFO
- Filtro = objeto RX com ID + máscara. Ex.: aceitar 0x100–0x10F → ID 0x100, máscara 0x7F0.
- **FIFO**: vários objetos RX com a mesma máscara, `EOB = 0` em todos menos o último — o controlador preenche em ordem.
- Prioridade de TX: o objeto de **menor número** sai primeiro.

## 5. Modos de teste
| `TST` | Efeito |
|---|---|
| LBACK | recebe o que transmite; o pino TX ainda transmite |
| SILENT | só escuta (não envia ACK nem erros) — análise de barramento |
| LBACK + SILENT | "hot selftest": loopback interno sem afetar o barramento |

---

## Exercícios
1. Filtre só o ID 0x123 (máscara 0x7FF) e mostre que o 0x18DAF110 é descartado.
2. FIFO de 4 objetos para RX em rajada.
3. Interrupções: `CTL.IE`, `RXIE` no objeto, handler `CAN0_Handler` (123) / `CAN1_Handler` (129) lendo `INT`.
