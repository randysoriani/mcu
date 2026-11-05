# Módulo 01 — Arquitetura Cortex-M4F e organização da memória do TM4C

Código: [`src/main.c`](src/main.c) (os dois chips) — imprime CPUID, DID0/DID1, tamanho da Flash/SRAM, os endereços de cada tipo de objeto e pisca o LED usando **bit-banding**.

```sh
make MCU=123 flash && picocom -b 115200 /dev/ttyACM0
```

---

## 1. O núcleo

Os dois chips usam **Cortex-M4F** (ARMv7E-M): Thumb-2 completo, divisão por hardware, instruções DSP, FPU de precisão simples, MPU de 8 regiões, bit-banding, NVIC com 8 níveis de prioridade (3 bits implementados pela TI).

Registradores: r0–r12, SP (MSP/PSP), LR, PC, xPSR, PRIMASK, BASEPRI, FAULTMASK, CONTROL, s0–s31, FPSCR. Todo endereço de função tem bit 0 = 1 (Thumb). Na entrada de uma exceção o hardware empilha r0–r3, r12, LR, PC, xPSR (+ s0–s15/FPSCR se a FPU foi usada) — por isso handlers são funções C comuns. Detalhes no módulo 01 do curso STM32: o núcleo é o mesmo do STM32F4.

---

## 2. Mapa de memória

| Região | TM4C123GH6PM | TM4C1294NCPDT |
|---|---|---|
| Flash | `0x0000_0000`–`0x0003_FFFF` (256 KB) | `0x0000_0000`–`0x000F_FFFF` (1 MB) |
| ROM da TI (bootloader + TivaWare em ROM) | `0x0100_0000` | `0x0100_0000` |
| SRAM | `0x2000_0000`–`0x2000_7FFF` (32 KB) | `0x2000_0000`–`0x2003_FFFF` (256 KB) |
| Alias bit-band da SRAM | `0x2200_0000` | `0x2200_0000` |
| Periféricos | `0x4000_0000`–`0x400F_FFFF` | idem + `0x4400_0000` (CRC/AES/DES/SHA) |
| Alias bit-band dos periféricos | `0x4200_0000` | `0x4200_0000` |
| Núcleo (SysTick, NVIC, SCB) | `0xE000_E000` | `0xE000_E000` |

**Diferença em relação ao STM32**: a Flash começa em `0x0000_0000`. Não há pino BOOT0 nem alias — a tabela de vetores do seu programa **é** o que o núcleo lê no reset. (Se a Flash estiver apagada, `0x0000_0004` = `0xFFFFFFFF` e o chip entra no bootloader da ROM.)

### Periféricos principais (iguais nos dois, salvo nota)

| Endereço | Periférico |
|---|---|
| `0x4000_0000` / `0x4000_1000` | WDT0 / WDT1 |
| `0x4000_4000`–`0x4000_7000`, `0x4002_4000`–`0x4002_5000` | **GPIO A–F pelo APB** (só 123) |
| `0x4000_8000`–`0x4000_B000` | SSI0–3 |
| `0x4000_C000`–`0x4001_3000` | UART0–7 |
| `0x4002_0000`–`0x4002_3000` | I2C0–3 |
| `0x4002_8000` / `0x4002_9000` | PWM0 / PWM1 (PWM1 só 123) |
| `0x4002_C000` / `0x4002_D000` | QEI0 / QEI1 (QEI1 só 123) |
| `0x4003_0000`–`0x4003_5000` | TIMER0–5 |
| `0x4003_6000`... | WTIMER0–5 (32/64 bits, só 123) |
| `0x4003_8000` / `0x4003_9000` | ADC0 / ADC1 |
| `0x4004_0000` / `0x4004_1000` | CAN0 / CAN1 |
| `0x4005_8000`–`0x4006_6000` | **GPIO pelo AHB** (123: A–F opcional; 129: A–Q, única opção) |
| `0x400A_F000` | EEPROM |
| `0x400F_C000` | Hibernação (RTC) |
| `0x400F_D000` | Controle da Flash |
| `0x400F_E000` | **System Control** (clock, reset, power) |
| `0x400F_F000` | uDMA |

### APB × AHB para GPIO

O 123 tem duas "aberturas" para cada porta A–F: a legada pelo APB (padrão após reset) e uma pelo AHB (mais rápida, 1 ciclo; escolhida porta a porta em `SYSCTL_GPIOHBCTL`). O curso usa o APB no 123. O 129 só tem a abertura AHB. Por isso `GPIOF` tem endereços diferentes nos dois headers — o código não muda.

---

## 3. System Control: o "RCC" da TI

Tudo em `0x400F_E000`. Para cada tipo de periférico há um registrador com **1 bit por instância**:

| Offset | Registrador | Função |
|---|---|---|
| `0x5xx` | `SRxxx` | reset do periférico |
| `0x6xx` | **`RCGCxxx`** | clock em Run mode |
| `0x7xx` | `SCGCxxx` | clock em Sleep |
| `0x8xx` | `DCGCxxx` | clock em Deep-Sleep |
| `0xAxx` | **`PRxxx`** | periférico pronto (r) |

`xx`: `00` WD, `04` TIMER, `08` GPIO, `0C` DMA, `14` HIB, `18` UART, `1C` SSI, `20` I2C, `34` CAN, `38` ADC, `40` PWM, `44` QEI, `58` EEPROM, `5C` WTIMER.

Ex.: UART0 → `SYSCTL_RCGCUART` (0x618) bit 0; esperar `SYSCTL_PRUART` (0xA18) bit 0.

```c
static inline void periph_enable(volatile uint32_t *rcgc, volatile uint32_t *pr, uint32_t n)
{
    *rcgc |= (1U << n);
    while (!(*pr & (1U << n))) { }   /* acessar antes = Bus Fault */
}
```

> Acessar um periférico sem clock no TM4C gera **BusFault/HardFault** (no STM32 a escrita é só ignorada). Erro clássico de iniciante.

---

## 4. Identificação do chip

| Registrador | Endereço | Conteúdo |
|---|---|---|
| `SCB->CPUID` | `0xE000_ED00` | `0x410FC241` = Cortex-M4 r0p1 |
| `SYSCTL_DID0` | `0x400F_E000` | CLASS [23:16]: `0x05` = TM4C123, `0x0A` = TM4C129; MAJOR/MINOR = revisão do silício (A0, A1, ...) |
| `SYSCTL_DID1` | `0x400F_E004` | PARTNO [23:16]: `0xA1` = TM4C123GH6PM, `0x1F` = TM4C1294NCPDT; PINCOUNT, TEMP, PKG |
| `FLASHPP` | `0x400F_DFC0` | SIZE [15:0]: Flash = (SIZE+1) × 2 KB |
| `SSIZE` | `0x400F_DFC4` | SRAM = (SIZE+1) × 256 B |

A revisão do silício importa: as erratas da TI são por revisão.

---

## 5. Bit-banding

Igual a qualquer Cortex-M3/M4: cada bit de `0x4000_0000`–`0x400F_FFFF` tem uma word em `0x4200_0000 + offset×32 + bit×4`. O exemplo usa o alias do registrador `DATA` (offset `0x3FC`) do LED.

Mas no TM4C o GPIO já tem algo melhor:

## 6. O registrador DATA mascarado

O bloco DATA ocupa 256 words (`0x000`–`0x3FC`). Os **bits [9:2] do endereço** são uma máscara: só os pinos cujo bit de máscara é 1 são lidos/escritos.

```c
GPIOF->DATA_BITS[1 << 3] = 0xFF;    /* endereço base+0x020: escreve SÓ o PF3 */
GPIOF->DATA_BITS[0x0E]   = 0x04;    /* pinos 1,2,3: PF2=1, PF1=PF3=0          */
GPIOF->DATA              = 0x08;    /* base+0x3FC: todos os pinos             */
```

É uma escrita **atômica** em pinos selecionados, sem read-modify-write — o equivalente ao `BSRR` do STM32, só que mais flexível.

---

## Exercícios
1. Leia `SYSCTL_DID0` e descubra a revisão do seu chip; procure-a na errata.
2. Comente o `periph_enable` de `uart_init` e veja no GDB em que exceção o programa para (`p/x $xpsr & 0x1ff`, `p/x *(uint32_t*)0xE000ED28` = CFSR).
3. No 123, habilite a abertura AHB da porta F (`SYSCTL_GPIOHBCTL |= 1<<5`) e pisque o LED pelo endereço `0x4005_D000`.
4. Calcule o alias bit-band do bit 4 (SW1) do `GPIOF->DATA` no 123 e leia o botão por ele.
