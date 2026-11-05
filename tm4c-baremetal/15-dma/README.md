# Módulo 15 — uDMA (micro DMA)

O uDMA dos TM4C é um **ARM PL230** com 32 canais. A grande diferença em relação ao DMA do STM32: a configuração de cada canal **não fica em registradores**, e sim numa **tabela de controle na SRAM**, que o controlador lê e atualiza sozinho.

Código: [`src/main.c`](src/main.c)
1. memória → memória pelo **canal de software 30**, modo AUTO
2. **UART0 TX** por uDMA (canal 9) enquanto a CPU conta iterações
3. **ADC0 SS3 contínuo** → uDMA (canal 17) → buffer de 32 amostras

```sh
make MCU=129 flash && picocom -b 115200 /dev/ttyACM0
```

---

## 1. Tabela de controle

```c
static UDMA_Entry ctl_table[64] __attribute__((aligned(1024)));
UDMA->CFG     = 1;                       /* MASTEN */
UDMA->CTLBASE = (uint32_t)ctl_table;
```
- 32 estruturas **primárias** (canais 0–31) + 32 **alternativas** (para ping-pong/scatter-gather).
- Cada estrutura = 16 bytes:

| Word | Campo | |
|---|---|---|
| 0 | `SRCENDP` | endereço do **último** item da origem |
| 1 | `DSTENDP` | endereço do **último** item do destino |
| 2 | `CHCTL` | controle (abaixo) |
| 3 | — | livre |

Sim: o PL230 usa ponteiros de **fim**, não de início. Para endereço fixo (registrador de periférico), fim = o próprio registrador e incremento = "nenhum".

### `CHCTL`
| Bits | Campo | |
|---|---|---|
| 31:30 | DSTINC | 0 byte, 1 half, 2 word, **3 nenhum** |
| 29:28 | DSTSIZE | 0 8 bits, 1 16, 2 32 |
| 27:26 | SRCINC | idem |
| 25:24 | SRCSIZE | (deve ser igual a DSTSIZE) |
| 17:14 | ARBSIZE | transferências por "rajada" antes de rearbitrar: 2^n |
| 13:4 | XFERSIZE | número de itens − 1 (**máx. 1024**) |
| 3 | NXTUSEBURST | |
| 2:0 | XFERMODE | 0 stop, **1 basic**, **2 auto**, **3 ping-pong**, 4–7 scatter-gather |

O controlador **decrementa XFERSIZE e zera XFERMODE** na própria tabela ao terminar. O curso usa a macro `UDMA_CHCTL(...)` de `tm4c_common.h`.

## 2. Registradores de controle (`0x400F_F000`)
| Reg. | Uso |
|---|---|
| `CFG` | MASTEN |
| `CTLBASE` | endereço da tabela |
| `SWREQ` | pedido por software (canal) |
| `ENASET` / `ENACLR` | habilita canal (**o bit volta a 0 quando a transferência termina**) |
| `ALTSET` / `ALTCLR` | estrutura primária/alternativa em uso |
| `REQMASKSET` | ignora pedidos do periférico |
| `PRIOSET` | prioridade alta |
| `USEBURSTSET` | só responde a pedidos de rajada |
| `CHMAP0..3` | 4 bits por canal: qual periférico ("encoding") usa o canal |

### Canais usados
| Canal | Encoding 0 |
|---|---|
| 8 / 9 | UART0 RX / TX |
| 14–17 | ADC0 SS0–SS3 |
| 18 / 19 | TIMER0A / TIMER0B |
| 30 | software (sem periférico) |

Tabela completa: datasheet, *μDMA Channel Assignments*. Cada canal tem até 5 (123) ou 9 (129) periféricos possíveis, escolhidos em `CHMAPn`.

## 3. Modos
- **BASIC**: um pedido do periférico → `2^ARBSIZE` itens; para quando o periférico deixa de pedir ou acaba.
- **AUTO**: um único pedido transfere tudo (memória → memória).
- **PING-PONG**: ao terminar a primária, passa sozinho para a alternativa (e vice-versa), enquanto a CPU recarrega a que terminou — fluxo contínuo (usado no módulo 17).
- **SCATTER-GATHER**: a tabela alternativa vira uma lista de tarefas.

## 4. Periférico + uDMA
1. Periférico: habilitar o pedido de DMA (`UART0->DMACTL.TXDMAE`; no 129 o ADC precisa de `ACTSS.ADENn`; timers do 129: `DMAEV`).
2. uDMA: `CHMAP`, estrutura na tabela, `ENASET`.
3. Fim: o bit do canal em `ENASET` vai a 0 e o periférico recebe uma interrupção de "DMA done" no próprio vetor (UART0, ADC0SS3...).

Ponto de atenção na UART: depois do uDMA, desligue `TXDMAE` antes de voltar a escrever no `DR` pela CPU.

## 5. ADC contínuo
`EMUX = 0xF` (sempre convertendo) + `SAC = 6` (média de 64, para desacelerar) + uDMA em BASIC de 32 amostras. O programa principal re-arma quando o canal termina; entre um ciclo e outro a FIFO do SS3 transborda (`OSTAT`) — inofensivo aqui. Para fluxo sem perda, use ping-pong.

---

## Exercícios
1. Recepção da UART0 por uDMA em ping-pong com dois buffers de 64 bytes.
2. ADC disparado pelo TIMER0 a 10 kHz + uDMA ping-pong + média de cada bloco.
3. `dma_memcpy` de 4 KB: XFERSIZE máximo é 1024 — use scatter-gather ou várias transferências.
