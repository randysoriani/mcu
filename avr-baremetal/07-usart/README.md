# Módulo 07 — USART: comunicação serial com interrupções

> **Objetivo deste módulo:** entender como a serial transmite bytes por um único fio, calcular o baud rate e construir uma serial **com interrupções e buffers circulares**, para que o programa não fique parado esperando cada byte. O resultado é um pequeno *shell* de comandos.

Código:
- [`src/main.c`](src/main.c) — o shell (igual nos dois chips)
- [`m328p/uart_irq.c`](m328p/uart_irq.c) — USART0 do ATmega com as interrupções RX e UDRE
- [`uc3a/uart_irq.c`](uc3a/uart_irq.c) — USART0 do UC3 com RXRDY e TXRDY

O `Makefile` usa `LIB := clock tick printf` (sem o `uart` da biblioteca, que é por *polling*): o `uart_irq.c` da pasta do chip ocupa o lugar dele.

```sh
make MCU=m328p flash && picocom -b 115200 /dev/ttyACM0
```

Saída real (ATmega328P no QEMU, `make clean && make MCU=m328p EXTRA=-DQEMU && make MCU=m328p qemu`):
```
=== Modulo 07: USART com interrupcoes ===
digite 'help'
> help
comandos: led on|off|toggle, uptime, echo <texto>
> uptime
1.316 s (erros/perdas de RX: 0)
> echo ola avr
ola avr
> xyz
desconhecido: 'xyz' (digite help)
```

---

## 1. Como a serial funciona

### 1.1 Assíncrona: sem fio de clock
Na serial "UART" não há um fio de clock: transmissor e receptor combinam antes a **velocidade** (baud rate: bits por segundo) e cada um mede o tempo com o próprio relógio. Dois fios de dados, um em cada sentido, cruzados:
```
 placa TX ─────────► RX  PC
 placa RX ◄───────── TX  PC
        GND ─────── GND
```

### 1.2 O quadro (8N1)
Parada, a linha fica em **1**. Cada byte é enviado assim:
```
 ────┐   ┌───┬───┬───┬───┬───┬───┬───┬───┬────────
     │ S │ b0│ b1│ b2│ b3│ b4│ b5│ b6│ b7│ P (parada)
     └───┴───┴───┴───┴───┴───┴───┴───┴───┘
```
1 bit de **start** (0) avisa "vem um byte"; 8 bits de dado, **o menos significativo primeiro**; 1 bit de **stop** (1). "8N1" = 8 bits, sem paridade (*None*), 1 stop. São 10 bits por byte: a 115 200 bps, 11 520 bytes/s, **87 µs por byte**.

### 1.3 Níveis elétricos
- No Arduino, a serial do ATmega (TTL, 0/5 V) vai ao conversor USB da placa.
- Na EVK1100, a USART0 passa por um conversor **RS-232** (±12 V, lógica invertida) até o conector DB9. **Nunca ligue RS-232 direto num pino do chip.** Use um cabo USB-serial com DB9.

### 1.4 Baud rate e erro
O receptor amostra cada bit no meio. Se os relógios diferirem mais de ~2%, os últimos bits do quadro são lidos errados. Como o baud rate vem de uma divisão inteira do clock, sempre há algum erro: é preciso calculá-lo.

---

## 2. ATmega328P — USART0

Pinos: **RXD = PD0** (Arduino D0), **TXD = PD1** (D1). Também ligados ao conversor USB da placa: não use D0/D1 para outra coisa.

### 2.1 Registradores
| Registrador | Endereço | Bits importantes |
|---|---|---|
| **UDR0** | 0xC6 | escrever = transmitir; ler = byte recebido (são dois registradores no mesmo endereço) |
| **UCSR0A** | 0xC0 | RXC0 (7): chegou byte; TXC0 (6): transmissão completa; **UDRE0** (5): UDR0 vazio, pode escrever; FE0 (4): erro de quadro; DOR0 (3): byte perdido; UPE0 (2): erro de paridade; **U2X0** (1): velocidade dupla |
| **UCSR0B** | 0xC1 | RXCIE0 (7), TXCIE0 (6), **UDRIE0** (5): interrupções; RXEN0 (4), TXEN0 (3): liga RX/TX |
| **UCSR0C** | 0xC2 | UMSEL0 (7:6): 00 assíncrona; UPM0 (5:4): paridade; USBS0 (3): 2 stops; **UCSZ01:00** (2:1): 11 = 8 bits |
| **UBRR0** | 0xC4/0xC5 | divisor de baud rate (12 bits) |

### 2.2 Cálculo do baud rate
```
normal (U2X0 = 0):  UBRR = F_CPU / (16 × baud) − 1
dupla  (U2X0 = 1):  UBRR = F_CPU / (8 × baud) − 1
```
A 16 MHz e 115 200 bps:

| Modo | UBRR exato | UBRR usado | baud real | erro |
|---|---|---|---|---|
| normal | 7,68 | 8 | 111 111 | **−3,5%** (falha!) |
| dupla | 16,36 | 16 | 117 647 | **+2,1%** (funciona) |

Por isso o curso liga o **U2X0**. O código arredonda: `(F_CPU / 8 + baud / 2) / baud − 1`. (Os cristais de "frequência mágica", como 14,7456 MHz, dão erro zero.)

### 2.3 As três interrupções
| Vetor | Quando | Uso |
|---|---|---|
| `USART_RX_vect` (18) | RXC0 = 1: chegou um byte | guardar o byte |
| `USART_UDRE_vect` (19) | UDRE0 = 1: pode escrever o próximo | enviar o próximo byte do buffer |
| `USART_TX_vect` (20) | TXC0 = 1: o último bit saiu do pino | desligar um transceptor RS-485 |

**Cuidado com a UDRE:** ela fica ativa **enquanto** o UDR0 estiver vazio. Se ninguém escrever um byte, a ISR roda de novo sem parar. Por isso, quando o buffer de transmissão esvazia, a ISR desliga a própria interrupção (`UDRIE0 = 0`), e `uart_putc` a religa quando há algo novo.

---

## 3. O buffer circular

É uma fila de tamanho fixo, onde quem escreve e quem lê andam em círculo:
```
 índice:  0   1   2   3   4   5   6   7
        [   | H | e | l | l | o |   |   ]
              ▲tail             ▲head
 a ISR escreve em head e avança; o main lê em tail e avança.
 vazio: head == tail       cheio: head + 1 == tail
```
Analogia: uma esteira de restaurante japonês — a cozinha (ISR) coloca pratos, o cliente (main) retira, e os dois nunca mexem no mesmo prato.

```c
ISR(USART_RX_vect)
{
    uint8_t status = UCSR0A;              /* erros ANTES de ler o dado */
    uint8_t c = UDR0;                     /* ler UDR0 limpa o RXC0 */
    uint8_t next = (rx_head + 1) & (RX_SIZE - 1);   /* tamanho potência de 2: o & dá a volta */
    if (next != rx_tail) { rx_buf[rx_head] = c; rx_head = next; }
    else uart_rx_overflow++;              /* cheio: perde o byte e conta */
}
```
Por que funciona sem desligar interrupções: cada índice é escrito por **um só lado** (head pela ISR, tail pelo main), e no AVR8 os índices são `uint8_t`, lidos em uma instrução (módulo 06).

---

## 4. AT32UC3A — USART0

Pinos: **RXD0 = PA00**, **TXD0 = PA01**, função A do GPIO (`gpio_periph(PA(0), 0)` passa o pino ao periférico).

### 4.1 Registradores (base `0xFFFF_1400`)
| Offset | Registrador | Função |
|---|---|---|
| 0x00 | **CR** | comandos: RSTRX/RSTTX (2/3), RXEN/RXDIS (4/5), TXEN/TXDIS (6/7), RSTSTA (8: limpa erros) |
| 0x04 | **MR** | modo: CHRL (7:6) = 3 para 8 bits; PAR (11:9) = 4 sem paridade; NBSTOP; OVER |
| 0x08 / 0x0C / 0x10 | **IER / IDR / IMR** | ligar / desligar / ver interrupções (escrever 1 no bit) |
| 0x14 | **CSR** | RXRDY (0), TXRDY (1), OVRE (5), FRAME (6), PARE (7), TXEMPTY (9) |
| 0x18 | **RHR** | byte recebido |
| 0x1C | **THR** | byte a transmitir |
| 0x20 | **BRGR** | divisor: CD (15:0) e FP (18:16, parte fracionária em oitavos) |

### 4.2 Baud rate
```
baud = PBA / (16 × (CD + FP/8))
48 MHz / (16 × 115 200) = 26,04  → CD = 26, FP = 0   (erro +0,16%)
```
O divisor fracionário dá precisão muito melhor que o ATmega.

### 4.3 Interrupções
A USART0 é o **grupo 6** do INTC. Uma única função trata RX e TX olhando o `CSR`. Ligar e desligar a interrupção de TX é uma escrita no `IER` ou `IDR` (sem ler-modificar-escrever).

---

## 5. Exercícios
1. Desligue o U2X0 e veja os caracteres errados a 115 200 bps; troque para 57 600 e calcule o erro.
2. Reduza `RX_SIZE` para 8 e cole um texto longo no terminal: observe o contador de perdas.
3. Adicione o comando `pwm <0-100>` usando o módulo 10.
4. No UC3, ative a paridade par (`PAR = 0`) e configure o picocom com `--parity e`.
