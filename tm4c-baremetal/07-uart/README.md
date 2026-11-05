# Módulo 07 — UART (ARM PL011)

A UART do TM4C é uma **PL011 da ARM** com FIFOs de 16 bytes. A UART0 (PA0 = RX, PA1 = TX) vai para o ICDI → `/dev/ttyACM0` nas duas LaunchPads.

Código:
- [`src/uart_irq.c`](src/uart_irq.c) — driver com interrupção + buffers circulares (igual nos dois chips)
- [`src/main.c`](src/main.c) — shell (`help`, `led on|off|toggle`, `uptime`, `clock`, `echo`, `reset`)
- [`common/lib/uart.c`](../common/lib/uart.c) — versão por polling, usada nos outros módulos

```sh
make MCU=123 flash && picocom -b 115200 /dev/ttyACM0
```

---

## 1. Pinos e clock

```c
periph_enable(&SYSCTL_RCGCUART, &SYSCTL_PRUART, 0);   /* UART0 */
gpio_port_enable(PORTA);
gpio_af(GPIOA, 0, 1);      /* AFSEL + PCTL = 1 (U0RX) + DEN */
gpio_af(GPIOA, 1, 1);      /* U0TX */
```

## 2. Baud rate

```
BRD = UARTclk / (16 × baud)     (com HSE = 0)
IBRD = parte inteira            FBRD = round(fração × 64)
```

| SYSCLK | 115200 | IBRD | FBRD | Erro |
|---|---|---|---|---|
| 80 MHz | 43,4028 | 43 | 26 | 0,01% |
| 120 MHz | 65,1042 | 65 | 7 | 0,01% |
| 16 MHz (reset) | 8,6806 | 8 | 44 | 0,08% |

Em ponto fixo: `div = 4 × clk / baud` → `IBRD = div >> 6`, `FBRD = div & 63`.

> `IBRD`/`FBRD` só são aplicados quando se escreve **`LCRH`** depois deles. Ordem: `CTL = 0` → `IBRD` → `FBRD` → `LCRH` → `CTL = UARTEN|TXE|RXE`.

## 3. Registradores

| Offset | Reg. | Bits principais |
|---|---|---|
| 0x000 | `DR` | [7:0] dado; na leitura [11:8] = OE, BE, PE, FE **deste byte** |
| 0x004 | `RSR`/`ECR` | erros; escrever qualquer valor limpa |
| 0x018 | `FR` | 3 BUSY, 4 **RXFE**, 5 **TXFF**, 6 RXFF, 7 TXFE |
| 0x024 | `IBRD` | divisor inteiro (16 bits) |
| 0x028 | `FBRD` | divisor fracionário (6 bits) |
| 0x02C | `LCRH` | 0 BRK, 1 PEN, 2 EPS, 3 STP2, **4 FEN** (FIFOs), **6:5 WLEN** (11 = 8 bits), 7 SPS |
| 0x030 | `CTL` | **0 UARTEN**, 4 EOT, 5 HSE, 7 LBE (loopback), **8 TXE**, **9 RXE**, 14 RTSEN, 15 CTSEN |
| 0x034 | `IFLS` | [2:0] nível TX, [5:3] nível RX: 0 = 1/8, 1 = 1/4, **2 = 1/2**, 3 = 3/4, 4 = 7/8 |
| 0x038 | `IM` | 4 RXIM, 5 TXIM, 6 RTIM, 7 FEIM, 8 PEIM, 9 BEIM, 10 OEIM |
| 0x03C/0x040 | `RIS`/`MIS` | status bruto / mascarado |
| 0x044 | `ICR` | escreve 1 para limpar |
| 0x048 | `DMACTL` | 0 RXDMAE, 1 TXDMAE (módulo 15) |
| 0xFC8 | `CC` | fonte de clock: 0 = SYSCLK, 5 = PIOSC |

## 4. Polling
```c
while (UART0->FR & UART_FR_TXFF) { }     /* FIFO de TX cheia? */
UART0->DR = c;

if (UART0->FR & UART_FR_RXFE) return -1; /* FIFO de RX vazia? */
return UART0->DR & 0xFF;
```
Com a FIFO, `kprintf` de até 16 caracteres retorna sem esperar.

## 5. Interrupções do PL011: as pegadinhas

- **TX é por borda**: TXRIS sobe quando a FIFO **passa** para baixo do nível do `IFLS`. Se a FIFO já está vazia quando você habilita TXIM, nada acontece. Por isso `uart_putc`:
  1. se o buffer de software está vazio e a FIFO tem espaço → escreve direto no `DR`;
  2. senão enfileira e liga `TXIM`; a ISR mantém a FIFO cheia, desligando `TXIM` quando o buffer acaba.
- **RX só avisa com a FIFO no nível** (8 bytes com IFLS = 1/2). Para bytes soltos (digitação), habilite também **RTIM** (receive timeout: dispara após 32 tempos de bit sem novos dados).
- A ISR lê **todos** os bytes disponíveis (`while (!(FR & RXFE))`).
- Seção crítica em `uart_putc` (`__disable_irq`) porque main e ISR mexem em `IM`.

## 6. Outros recursos
- **Loopback** interno: `CTL.LBE = 1`.
- **9 bits / endereço** (multidrop RS-485): `9BITADDR` (offset 0x0A4).
- **Flow control** RTS/CTS em UART1 (123: PC4/PC5 — cuidado, não são JTAG — ou PF0/PF1).
- **IrDA SIR** e **ISO7816 smart card**.
- `CC = 5` (PIOSC): baud rate independente do SYSCLK — útil com deep-sleep.

---

## Exercícios
1. 9600 8E1: `LCRH = WLEN_8 | FEN | PEN | EPS`.
2. Ative o loopback interno (`LBE`) e teste o driver sem cabo.
3. Adicione `peek`/`poke` ao shell.
4. UART1 (123: PB0/PB1, PCTL 1) ligada a um adaptador USB-serial; faça uma ponte UART0 ↔ UART1.
