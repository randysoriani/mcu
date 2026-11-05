# Módulo 07 — USART / UART

A serial assíncrona é a ferramenta de depuração mais útil em embarcados: com ela o programa "fala". Nas Nucleo, a **USART2** (PA2 = TX, PA3 = RX) está ligada ao ST-LINK, que a expõe pelo USB como `/dev/ttyACM0` — não é preciso nenhum adaptador.

Código:
- [`f0/uart_irq.c`](f0/uart_irq.c) e [`f4/uart_irq.c`](f4/uart_irq.c) — driver com **interrupção** e **buffers circulares** de RX e TX
- [`src/main.c`](src/main.c) — um *shell* com comandos (`help`, `led on`, `uptime`, `clock`, `echo`, `reset`)
- [`common/f0/uart.c`](../common/f0/uart.c), [`common/f4/uart.c`](../common/f4/uart.c) — versão simples **por polling**, usada pelos outros módulos

```sh
make MCU=f0 flash && picocom -b 115200 /dev/ttyACM0
```
Sessão real (F4 no QEMU):
```
=== Modulo 07: UART (interrupcao + buffer circular) ===
digite 'help'
> help
Comandos:
  led on|off|toggle
  ...
> echo ola mundo
ola mundo
> uptime
2.455 s  (overflows RX: 0)
```

---

## 1. O protocolo

```
 repouso  start  D0  D1  D2  D3  D4  D5  D6  D7  stop  repouso
 ───────┐     ┌───┬───┬───┬───┬───┬───┬───┬───┬──────────────
   1    │  0  │ LSB primeiro                 MSB│  1
        └─────┴───┴───┴───┴───┴───┴───┴───┴───┘
        |<- 1 bit = 1/baud (8,68 µs a 115200) ->|
```

- Linha em repouso = 1. **Start bit** = 0 sincroniza o receptor. Dados com **LSB primeiro**. **Stop bit(s)** = 1.
- Sem clock compartilhado: os dois lados precisam concordar no **baud rate** (erro tolerável total ≈ ±2–3%).
- "8N1" = 8 bits de dados, sem paridade, 1 stop bit → 10 bits por byte → 11 520 bytes/s a 115 200 bps.
- Níveis: TTL 3,3 V no MCU. RS-232 usa ±3..15 V invertido — precisa de MAX3232. RS-485 é diferencial — precisa de transceiver.

O receptor do STM32 amostra cada bit 16 vezes (ou 8, com `OVER8`) e decide pelos 3 do meio, detectando ruído (NE) e erro de enquadramento (FE, stop bit = 0).

---

## 2. Baud rate

```
            fCK                                fCK
 baud = ───────────── (OVER8 = 0)   →   USARTDIV = ─────
         16 × USARTDIV                             baud
```

Nos **dois** chips, com oversampling 16, o valor gravado em `BRR` é simplesmente `fCK / baud` (o F4 descreve como "mantissa 12 bits + fração 4 bits", que é o mesmo número em ponto fixo 12.4):

| | fCK (USART2) | 115200 | Erro |
|---|---|---|---|
| F072 | PCLK = 48 MHz | 48e6/115200 = 416,67 → **417** (0x1A1) | 0,08% |
| F446 | PCLK1 = 45 MHz | 45e6/115200 = 390,63 → **391** (0x187: mantissa 24, fração 7/16) | 0,1% |

```c
USART2->BRR = (fck + baud/2) / baud;     /* arredonda */
```

> No F072 a USART1 pode usar SYSCLK, HSI ou LSE (`RCC_CFGR3.USART1SW`); a USART2 usa PCLK. No F4 o clock é sempre o PCLK do barramento (APB1 para USART2/3, UART4/5; APB2 para USART1/6).

---

## 3. Duas IPs diferentes

O F0 (como F3, F7, L4, G0, H7...) tem a USART de **nova geração**; o F4 (como F1, F2) tem a **clássica**.

### STM32F072 — USART v2 (RM0091, cap. *USART*)

| Offset | Registrador | Principais bits |
|---|---|---|
| 0x00 | `CR1` | 0 **UE**, 2 **RE**, 3 **TE**, 4 IDLEIE, 5 **RXNEIE**, 6 TCIE, 7 **TXEIE**, 8 PEIE, 9 PS, 10 PCE, 12 M0, 15 OVER8, 28 M1 |
| 0x04 | `CR2` | 13:12 STOP (00 = 1 bit), 15 SWAP (troca TX/RX!), 16 RXINV, 17 TXINV, 19 MSBFIRST, 23 RTOEN |
| 0x08 | `CR3` | 0 EIE, 3 HDSEL (half-duplex), 6 **DMAR**, 7 **DMAT**, 8 RTSE, 9 CTSE, 12 OVRDIS |
| 0x0C | `BRR` | divisor |
| 0x14 | `RTOR` | receiver timeout |
| 0x18 | `RQR` | requisições (flush RX etc.) |
| 0x1C | `ISR` | 0 PE, 1 FE, 2 NF, 3 **ORE**, 4 IDLE, 5 **RXNE**, 6 **TC**, 7 **TXE**, 16 BUSY |
| 0x20 | `ICR` | escreve 1 limpa: 0 PECF, 1 FECF, 2 NCF, 3 **ORECF**, 4 IDLECF, 6 TCCF |
| 0x24 | `RDR` | dado recebido (ler limpa RXNE) |
| 0x28 | `TDR` | dado a transmitir (escrever limpa TXE) |

Tamanho de palavra: `M1:M0` = 00 → 8 bits, 01 → 9 bits, 10 → 7 bits. Com paridade (PCE), o bit de paridade ocupa o MSB da palavra (8 bits + paridade = M0=1).

### STM32F446 — USART v1 (RM0390, cap. *USART*)

| Offset | Registrador | Principais bits |
|---|---|---|
| 0x00 | `SR` | 0 PE, 1 FE, 2 NF, 3 **ORE**, 4 IDLE, 5 **RXNE**, 6 **TC**, 7 **TXE**, 8 LBD, 9 CTS |
| 0x04 | `DR` | dado (escrita → TX, leitura ← RX) |
| 0x08 | `BRR` | [15:4] mantissa, [3:0] fração |
| 0x0C | `CR1` | 2 **RE**, 3 **TE**, 4 IDLEIE, 5 **RXNEIE**, 6 TCIE, 7 **TXEIE**, 9 PS, 10 PCE, 12 M, **13 UE**, 15 OVER8 |
| 0x10 | `CR2` | 13:12 STOP, 14 LINEN, 11 CLKEN (modo síncrono) |
| 0x14 | `CR3` | 0 EIE, 3 HDSEL, 6 **DMAR**, 7 **DMAT**, 8 RTSE, 9 CTSE |
| 0x18 | `GTPR` | smartcard/IrDA |

Note a diferença de posição: **UE é o bit 0 do CR1 no F0 e o bit 13 no F4**; e o F4 começa pelo `SR` no offset 0.

Limpeza de flags de erro no F4: **ler SR e depois ler DR** (a sequência limpa ORE, NF, FE, PE). No F0: escrever 1 no bit correspondente do `ICR`.

---

## 4. Pinos

| | TX | RX | AF |
|---|---|---|---|
| F072 USART2 | PA2 | PA3 | **AF1** |
| F446 USART2 | PA2 | PA3 | **AF7** |

A função alternativa vem do datasheet (tabela *Alternate functions*). Configure o pino em modo AF (`MODER = 10`) e o número em `AFR`. Coloque pull-up no RX para a linha não flutuar se o fio soltar.

---

## 5. Polling

```c
void uart_putc(char c) {
    while (!(USART2->ISR & USART_ISR_TXE)) { }   /* F4: USART2->SR & USART_SR_TXE */
    USART2->TDR = c;                             /* F4: USART2->DR = c            */
}
```

- **TXE** (transmit data register empty): o registrador de dados está livre — pode escrever o próximo byte, mesmo que o anterior ainda esteja saindo pelo shift register.
- **TC** (transmission complete): o shift register também terminou. Use antes de desligar a USART, entrar em low-power ou trocar a direção num RS-485.
- **RXNE**: há um byte em RDR/DR.
- **ORE** (overrun): chegou um byte novo com o anterior ainda não lido; o novo se perde. A interrupção de ORE está ligada ao `RXNEIE`: se ORE não for limpo, a IRQ fica disparando sem parar. No F0 limpa-se com `ICR = ORECF` (veja `common/f0/uart.c`); no F4, lendo SR e depois DR.

Polling é simples, mas `kprintf` de 80 caracteres bloqueia a CPU por ~7 ms a 115200.

---

## 6. Interrupção + buffer circular

```
           main                          ISR (USART2_IRQHandler)
 uart_putc ──► tx_buf[head++] ──► TXEIE=1 ──► TXE ─► TDR = tx_buf[tail++]
                                               buffer vazio ─► TXEIE=0
 uart_getc ◄── rx_buf[tail++] ◄─────────────── RXNE ─► rx_buf[head++] = RDR
```

Buffer circular (ring buffer) com tamanho potência de 2:
```c
#define N 128
volatile uint8_t  buf[N];
volatile uint32_t head, tail;           /* head: produtor escreve; tail: consumidor lê */
vazio:  head == tail
cheio:  ((head + 1) & (N-1)) == tail    /* sacrifica 1 posição para distinguir */
```
Com **um produtor e um consumidor** (ISR × main), e índices de 32 bits (escrita atômica), não é preciso desabilitar interrupções: cada lado só escreve no próprio índice.

Pontos de atenção no driver:
1. **TXEIE só fica ligado enquanto há dados**. TXE fica em 1 sempre que o registrador está vazio; com TXEIE ligado e nada para mandar, a IRQ dispararia sem parar.
2. O handler verifica `(CR1 & TXEIE) && (ISR & TXE)`: a IRQ é compartilhada por RX, TX e erros.
3. Erros (ORE/FE/NE/PE) precisam ser limpos; senão a IRQ (com EIE/RXNEIE) fica disparando.
4. `uart_putc` com buffer cheio espera a ISR esvaziar — **não chame `kprintf` dentro de uma ISR de prioridade igual ou maior que a da USART**, ou o sistema trava.

---

## 7. Detalhes úteis

- **IDLE line**: flag IDLE indica que a linha ficou em repouso por 1 quadro após receber dados — ótimo para detectar o fim de uma mensagem de tamanho variável (com DMA, módulo 15).
- **Half-duplex** (`HDSEL`): TX e RX no mesmo fio (servos Dynamixel, 1-Wire por UART).
- **Hardware flow control** (RTS/CTS): `RTSE`/`CTSE` no CR3.
- **Auto baud rate** (só F0, `CR2.ABREN`): mede o primeiro caractere e ajusta o BRR.
- **Inversão e troca de pinos** (só F0, `CR2.TXINV/RXINV/SWAP`): conserta placas com TX/RX trocados sem refazer a PCB.
- **printf da newlib**: implementando `int _write(int fd, char *p, int n)` que chama `uart_putc`, o `printf()` padrão funciona (custa ~20 KB com float). O curso usa `kprintf` (common/lib/printf.c, ~1 KB).

---

## Exercícios

1. Configure a USART2 para 9600 8E1 (paridade par). Teste com `picocom -b 9600 -y e`.
2. Adicione ao shell um comando `peek <endereço>` que imprime uma word da memória em hexadecimal (cuidado com endereços inválidos!) e `poke <endereço> <valor>`.
3. Use a flag IDLE para processar comandos sem precisar de ENTER (fim de mensagem = pausa na linha).
4. Implemente `_write()` e use o `printf()` da newlib-nano. Compare o tamanho do binário.
5. Ligue a USART1 (F072: PA9/PA10 AF1; F446: PA9/PA10 AF7) a um adaptador USB-serial e faça uma "ponte" entre USART1 e USART2.
