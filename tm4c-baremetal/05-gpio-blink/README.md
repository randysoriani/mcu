# Módulo 05 — GPIO como saída: o "Hello World" (blink)

- [`123/main.c`](123/main.c) — pisca o LED **verde PF3** da EK-TM4C123GXL
- [`129/main.c`](129/main.c) — pisca **D1 (PN1)** e **D2 (PN0)** alternados na EK-TM4C1294XL

Programas autocontidos, rodando no PIOSC de 16 MHz.

```sh
make MCU=123 flash
make MCU=129 flash
```

---

## 1. Os passos

```c
SYSCTL_RCGCGPIO |= (1U << 5);                  /* 1. clock da porta F (A=0, B=1, ... F=5) */
while (!(SYSCTL_PRGPIO & (1U << 5))) { }       /* 2. espera ficar pronta                  */
GPIOF->DIR |= (1U << 3);                       /* 3. PF3 saída                            */
GPIOF->DEN |= (1U << 3);                       /* 4. HABILITA A FUNÇÃO DIGITAL            */
GPIOF->DATA_BITS[1U << 3] = 0xFF;              /* 5. acende (escrita mascarada)           */
```

O passo 4 é o erro número 1 de quem vem do STM32: **todo pino do TM4C começa com a parte digital desligada** (`DEN = 0`). Sem `DEN`, o pino não lê nem escreve, e nenhum erro aparece.

Bits do `RCGCGPIO`: A0 B1 C2 D3 E4 F5 G6 H7 J8 K9 L10 M11 N12 P13 Q14 (129). LED do 129 = porta N = bit 12.

---

## 2. Registradores do GPIO (iguais nas duas famílias)

| Offset | Registrador | Descrição |
|---|---|---|
| 0x000–0x3FC | `DATA` | dado, **endereçado por máscara** (bits [9:2] do endereço) |
| 0x400 | `DIR` | 1 = saída |
| 0x404–0x41C | `IS`, `IBE`, `IEV`, `IM`, `RIS`, `MIS`, `ICR` | interrupções (módulo 08) |
| 0x420 | `AFSEL` | 1 = pino controlado por periférico |
| 0x500 / 0x504 / 0x508 | `DR2R` / `DR4R` / `DR8R` | corrente de saída 2/4/8 mA (padrão 2 mA) |
| 0x50C | `ODR` | 1 = open-drain |
| 0x510 / 0x514 | `PUR` / `PDR` | pull-up / pull-down |
| 0x518 | `SLR` | controle de slew rate (só com 8 mA) |
| 0x51C | **`DEN`** | 1 = função digital habilitada |
| 0x520 / 0x524 | `LOCK` / `CR` | proteção de pinos especiais (módulo 08) |
| 0x528 | `AMSEL` | 1 = modo analógico (ADC) |
| 0x52C | `PCTL` | 4 bits por pino: qual função alternativa |
| 0x530 / 0x534 | `ADCCTL` / `DMACTL` | pino dispara ADC / uDMA |

Cada porta tem **8 pinos** (0–7), e 1 bit por pino em cada registrador — mais simples que os 2 bits/pino do STM32.

### Força de saída
Em vez de "velocidade" (STM32 `OSPEEDR`), o TM4C escolhe a **corrente**: `DR2R`, `DR4R` ou `DR8R` (escrever 1 num deles zera o bit nos outros dois). LEDs da LaunchPad: 2 mA basta (há transistor/resistor).

---

## 3. DATA mascarado

```
endereço = base + (máscara << 2)
```
| Escrita | Efeito |
|---|---|
| `DATA_BITS[0x08] = 0xFF` (base+0x020) | PF3 = 1, demais intocados |
| `DATA_BITS[0x08] = 0x00` | PF3 = 0 |
| `DATA_BITS[0x0E] = 0x04` (base+0x038) | PF1 = 0, **PF2 = 1**, PF3 = 0 (LED azul) |
| `DATA = 0xFF` (base+0x3FC) | todos |

Na leitura, bits fora da máscara voltam 0. É atômico e não precisa de read-modify-write — interrupções podem mexer em outros pinos da mesma porta sem conflito. O `^=` em `DATA_BITS[m]` lê só os bits da máscara, inverte e escreve só eles.

---

## 4. Pinos protegidos e de cuidado

| Chip | Pinos | Motivo |
|---|---|---|
| 123 | **PC0–PC3** | JTAG — não mexa ou perde o debug |
| 123 | PF0, PD7 | NMI — exigem LOCK/CR para virar GPIO |
| 129 | PD7, PE7 | NMI |
| 123 | PB6/PB7 ↔ PD0/PD1 | ligados entre si por R9/R10 (0 Ω) na LaunchPad |

---

## 5. Comparando com o STM32

| STM32 | TM4C |
|---|---|
| `RCC->AHB1ENR \|= GPIOAEN` | `SYSCTL_RCGCGPIO \|= 1` + esperar `PRGPIO` |
| `MODER = 01` | `DIR = 1` **e** `DEN = 1` |
| `BSRR = 1<<n` / `1<<(n+16)` | `DATA_BITS[1<<n] = 0xFF / 0x00` |
| `OSPEEDR` | `DR2R/DR4R/DR8R` |
| `AFR` (4 bits) | `AFSEL` + `PCTL` (4 bits) |

---

## Exercícios
1. No 123, faça um ciclo de cores no LED RGB (vermelho, verde, azul, amarelo, ciano, magenta, branco) usando `DATA_BITS[0x0E]`.
2. Tire a linha do `DEN` e verifique que nada acontece.
3. No 129, pisque os 4 LEDs (PN1, PN0, PF4, PF0) como um contador binário.
4. Meça a frequência máxima de toggle com `DR2R` e `DR8R`.
