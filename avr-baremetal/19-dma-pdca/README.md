# Módulo 19 — DMA com o PDCA (só AT32UC3A)

> **Objetivo deste módulo:** entender o DMA — um ajudante de hardware que copia dados entre a memória e os periféricos **sem a CPU** — usando o PDCA do UC3A para enviar um texto pela serial enquanto a CPU faz outra coisa.

Código: [`uc3a/main.c`](uc3a/main.c) — envia um texto de ~200 bytes pela USART0 via PDCA; enquanto isso a CPU conta voltas de um laço e mostra quantas conseguiu dar.

> O **ATmega328P não tem DMA**: `make MCU=m328p` neste módulo só mostra uma mensagem. (A família AVR XMEGA tem DMA; os AVR-Dx/ATmega4809, não.) No ATmega, a alternativa é a transmissão por interrupção com buffer circular do módulo 07.

---

## 1. O que é DMA

Sem DMA, para enviar 200 bytes a CPU escreve um byte, espera ~87 µs (115 200 bps), escreve o próximo... São 17 ms de CPU presa — ou 200 interrupções. Com **DMA** (*Direct Memory Access*), a CPU diz uma vez "copie estes 200 bytes da memória para a serial" e fica livre; o controlador de DMA entrega cada byte quando a serial pede.

Analogia: em vez de levar cada carta pessoalmente ao correio, você entrega o maço a um mensageiro e volta ao trabalho.

```
            ┌─────────┐   pedido ("TXRDY")   ┌─────────┐
 RAM ──────►│  PDCA   │◄─────────────────────│ USART0  │
 (texto)    │ canal 0 │──── 1 byte ─────────►│  THR    │──► pino TX
            └─────────┘                      └─────────┘
 CPU: livre
```

---

## 2. O PDCA do UC3A

O **PDCA** (*Peripheral DMA Controller*) tem **15 canais**. Cada canal liga **um periférico** (escolhido pelo **PID**) a uma área da memória, num sentido só (o sentido vem do PID: RX = periférico → memória; TX = memória → periférico).

Base `0xFFFF_0000`; cada canal ocupa 0x40 bytes:

| Offset | Registrador | Função |
|---|---|---|
| 0x00 | **MAR** | endereço de memória atual (avança sozinho) |
| 0x04 | **PSR** | **PID**: qual periférico |
| 0x08 | **TCR** | quantos itens faltam |
| 0x0C / 0x10 | MARR / TCRR | **recarga**: próximo buffer, assumido quando o TCR chega a 0 |
| 0x14 | **CR** | **TEN** (0): liga; TDIS (1): desliga; ECLR (8): limpa erro |
| 0x18 | MR | SIZE (1:0): 0 = byte, 1 = meia-word, 2 = word |
| 0x1C | SR | TEN: canal ligado |
| 0x20 / 0x24 / 0x28 | IER / IDR / IMR | interrupções |
| 0x2C | **ISR** | RCZ (0): recarga zerou; **TRC** (1): transferência completa; TERR (2): erro |

Alguns PIDs (tabela "PDCA Handshake Signals" do datasheet):

| PID | Periférico |
|---|---|
| 0 | ADC |
| 2 | USART0 RX |
| 10 | USART0 TX |

(SPI, TWI e SSC também têm PIDs; confira a tabela.)

### 2.1 O código
```c
PDCA_CH_TypeDef *ch = PDCA_CH(0);
ch->CR  = TDIS;                 /* desliga para configurar */
ch->PSR = PDCA_PID_USART0_TX;   /* 10 */
ch->MR  = 0;                    /* bytes */
ch->MAR = (uint32_t)texto;      /* origem */
ch->TCR = tamanho;              /* quantos */
ch->CR  = TEN;                  /* vai! */

while (!(ch->ISR & TRC)) voltas++;    /* a CPU está livre: aqui só conta */
```
O PDCA entrega o **último byte ao THR** quando TRC = 1; a USART ainda precisa transmiti-lo. Antes de reaproveitar a serial, espere o `TXEMPTY`.

### 2.2 Buffer duplo (recarga)
Com `MARR`/`TCRR`, o canal emenda um segundo buffer sem pausa: enquanto o DMA envia o buffer A, o programa enche o B; quando A termina, o hardware passa para B sozinho (flag RCZ) e o programa recarrega A. É assim que se faz áudio, aquisição contínua do ADC ou recepção serial sem perder bytes.

### 2.3 Cuidados
- O buffer precisa **continuar existindo** até o fim da transferência: nunca uma variável local de uma função que já retornou.
- A CPU e o DMA disputam o barramento; num laço muito apertado de acesso à RAM, a CPU pode ficar um pouco mais lenta.
- Não mexa no buffer enquanto o DMA o lê.

---

## Exercícios
1. Envie o texto com polling e compare quantas voltas a CPU dá com e sem DMA.
2. Use a interrupção TRC (grupo 3 do INTC) em vez do laço.
3. Receba pela serial com o PID 2 e buffer duplo; imprima cada bloco de 16 bytes recebido.
4. Faça o ADC converter 256 amostras direto para a memória (PID 0).
