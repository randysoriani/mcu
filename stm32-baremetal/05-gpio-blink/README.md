# Módulo 05 — GPIO como saída: o "Hello World" (blink)

O primeiro programa que faz algo visível: piscar o LED **LD2** (verde) da Nucleo, ligado em **PA5** (ativo em nível alto).

- [`f0/main.c`](f0/main.c) — Nucleo-F072RB, usa `BSRR`
- [`f4/main.c`](f4/main.c) — Nucleo-F446RE, usa `ODR ^=`

Os dois programas são **autocontidos**: só incluem o header de registradores. Nenhum clock é configurado (roda com o HSI do reset).

```sh
make MCU=f0 flash
make MCU=f4 flash
```

---

## 1. Os três passos de qualquer periférico

1. **Ligar o clock** do periférico no RCC.
2. **Configurar** (modo, velocidade, etc.).
3. **Usar**.

```c
RCC->AHBENR |= RCC_AHBENR_GPIOAEN;           /* F0: bit 17 do AHBENR  */
RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;         /* F4: bit 0 do AHB1ENR  */

GPIOA->MODER &= ~(3U << (5 * 2));            /* limpa MODER5          */
GPIOA->MODER |=  (1U << (5 * 2));            /* 01 = saída            */

GPIOA->BSRR = (1U << 5);                     /* PA5 = 1               */
GPIOA->BSRR = (1U << (5 + 16));              /* PA5 = 0               */
```

---

## 2. Estrutura de um pino

```
                    ┌────────────── VDD
                    │      ┌─ pull-up (PUPDR)
                 [P-MOS]   │
   ODR/BSRR ──► driver ────┼────────────●── pino
   AF (periférico) ─►      │            │
                 [N-MOS]   └─ pull-down │
                    │                   │
                   VSS     IDR ◄─ Schmitt trigger ◄──┘
                                       ADC/DAC ◄── (modo analógico)
```

- **Push-pull** (OTYPER = 0): P-MOS liga para 1, N-MOS liga para 0. Força os dois níveis.
- **Open-drain** (OTYPER = 1): só o N-MOS atua; o nível 1 vem de um resistor de pull-up (I2C, barramentos compartilhados, nível 5 V em pinos *FT*).
- **IDR** lê o pino em qualquer modo (exceto analógico).

---

## 3. Registradores do GPIO (iguais no F0 e no F4)

Bases: F0 `GPIOA = 0x4800_0000` (AHB2), F4 `GPIOA = 0x4002_0000` (AHB1). Cada porta ocupa 0x400 bytes (GPIOB = base + 0x400, ...). Cada porta tem 16 pinos (0–15).

| Offset | Registrador | Bits por pino | Descrição |
|---|---|---|---|
| 0x00 | `MODER` | 2 | 00 entrada, **01 saída**, 10 função alternativa, 11 analógico |
| 0x04 | `OTYPER` | 1 | 0 push-pull, 1 open-drain |
| 0x08 | `OSPEEDR` | 2 | velocidade/slew rate: 00 baixa, 01 média, 10 alta, 11 muito alta |
| 0x0C | `PUPDR` | 2 | 00 nenhum, 01 pull-up, 10 pull-down |
| 0x10 | `IDR` | 1 | valor lido do pino (r) |
| 0x14 | `ODR` | 1 | valor de saída (rw) |
| 0x18 | `BSRR` | — | [15:0] BSy: 1 seta o pino y; [31:16] BRy: 1 reseta o pino y (w) |
| 0x1C | `LCKR` | 1 | trava a configuração até o próximo reset |
| 0x20 | `AFRL` (`AFR[0]`) | 4 | função alternativa dos pinos 0–7 |
| 0x24 | `AFRH` (`AFR[1]`) | 4 | função alternativa dos pinos 8–15 |
| 0x28 | `BRR` | 1 | só F0: 1 reseta o pino (atalho do BSRR[31:16]) |

### Cálculo do campo de um pino

Para o pino `n`:
- `MODER`, `OSPEEDR`, `PUPDR`: bits `[2n+1 : 2n]` → máscara `3U << (2*n)`
- `OTYPER`, `IDR`, `ODR`: bit `n`
- `AFR[n/8]`: bits `[4*(n%8)+3 : 4*(n%8)]`

PA5: MODER bits 11:10, ODR bit 5, AFRL bits 23:20.

### Valores de reset importantes

| Porta | MODER no reset | Por quê |
|---|---|---|
| GPIOA (F0) | `0x2800_0000` | PA13/PA14 em AF (SWD) |
| GPIOA (F4) | `0xA800_0000` | PA13/PA14/PA15 em AF (SWD/JTAG) |
| GPIOB (F4) | `0x0000_0280` | PB3/PB4 em AF (JTAG) |
| demais | `0x0000_0000` | entrada |

> **Nunca escreva `GPIOA->MODER = 0x400;`** (atribuição direta): isso transforma PA13/PA14 em entrada e mata a conexão SWD. Sempre use read-modify-write apenas nos bits do seu pino. Se acontecer, use "connect under reset" (módulo 00).

---

## 4. `ODR` × `BSRR`

```c
GPIOA->ODR ^= (1U << 5);        /* LDR + EOR + STR: 3 instruções, não atômico */
GPIOA->BSRR = (1U << 5);        /* 1 STR: atômico                              */
```

Se uma interrupção mexer em **outro** pino da mesma porta entre o LDR e o STR do `ODR ^=`, a alteração dela é desfeita. O `BSRR` é só-escrita e bits em 0 não têm efeito: cada escrita mexe apenas nos pinos desejados. **Use BSRR** sempre que houver interrupções mexendo na mesma porta.

---

## 5. Velocidade (`OSPEEDR`)

Não é a frequência máxima de toggle por software; é a **taxa de subida/descida** do driver. Mais rápido = bordas mais íngremes = mais EMI, *ringing* e consumo. Use:
- baixa para LEDs, relés, sinais lentos;
- alta/muito alta para SPI rápido, MCO, SDIO, FMC.

Frequências máximas aproximadas (F4, 3,3 V, 50 pF): baixa ~2–4 MHz, média ~25 MHz, alta ~50 MHz, muito alta ~100 MHz (ver datasheet, *I/O AC characteristics*).

---

## 6. O atraso por laço

```c
static void delay(volatile uint32_t n)
{
    while (n--) { __asm volatile ("nop"); }
}
```

- `volatile` no parâmetro e o `nop` impedem que o otimizador elimine o laço (sem efeito observável, `-O2` o removeria).
- O tempo depende do clock, da otimização e dos wait states da Flash — **não** use para temporizações reais. O módulo 06 substitui isso pelo SysTick.

---

## 7. Desmontando o resultado

```sh
make MCU=f0 disasm && less build/f0/05-gpio-blink.lst
```
```
080000c0 <main>:
 80000c0:  2380   movs  r3, #128        @ 0x80
 80000c2:  4a16   ldr   r2, [pc, #88]   @ r2 = 0x40021000 (RCC), do literal pool
 80000c4:  029b   lsls  r3, r3, #10     @ r3 = 0x80 << 10 = 1 << 17 (GPIOAEN)
 80000c6:  6951   ldr   r1, [r2, #20]   @ r1 = RCC->AHBENR (offset 0x14 = 20)
 80000ca:  430b   orrs  r3, r1
 80000ce:  6153   str   r3, [r2, #20]   @ RCC->AHBENR |= GPIOAEN
 80000cc:  2190   movs  r1, #144        @ 0x90
 80000d0:  05c9   lsls  r1, r1, #23     @ r1 = 0x90 << 23 = 0x48000000 (GPIOA)
 80000d2:  680b   ldr   r3, [r1, #0]    @ GPIOA->MODER
```
(saída real do `objdump`, F0, `-O2`; o compilador intercala instruções para aproveitar o pipeline.) O M0 não tem imediatos de 32 bits: constantes viram `movs` + `lsls` ou uma leitura do *literal pool*. Vale a pena olhar: cada linha C vira poucas instruções e os offsets da struct aparecem como `#20` (0x14).

---

## 8. LEDs em outras placas

| Placa | LED | Nível |
|---|---|---|
| Nucleo-64 (F030R8, F072RB, F401RE, F411RE, F446RE) | PA5 | alto |
| STM32F4-Discovery (F407) | PD12 verde, PD13 laranja, PD14 vermelho, PD15 azul | alto |
| STM32F0-Discovery (F051) | PC8 azul, PC9 verde | alto |
| Black Pill (F401/F411CE) | PC13 | **baixo** |
| Blue Pill (F103, fora do escopo) | PC13 | baixo |

---

## Exercícios

1. Pisque o LED com o padrão de SOS em Morse (… --- …).
2. Troque `BSRR` por `BRR` no F0 para apagar. Por que o F4 não tem `BRR`?
3. Configure PA5 como open-drain. O LED ainda acende? Por quê? (Veja o esquemático no UM1724: o LED vai do pino ao GND por um resistor.)
4. Meça com osciloscópio a frequência máxima de toggle de `while(1) GPIOA->ODR ^= 1<<5;` com OSPEEDR baixa e muito alta, a 8 MHz e depois com o clock do módulo 04.
5. Use `LCKR` para travar a configuração do PA5 e tente alterá-la depois.
