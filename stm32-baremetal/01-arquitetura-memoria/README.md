# Módulo 01 — Arquitetura Cortex-M e organização da memória

Antes de escrever o startup, é preciso saber **o que o processador faz sozinho** ao ligar e **onde está cada coisa** no espaço de endereçamento.

Código: [`src/main.c`](src/main.c) (o mesmo para F0 e F4) — imprime CPUID, IDCODE, tamanho da Flash, Unique ID e o endereço de cada tipo de objeto C; no F4 pisca o LED usando **bit-banding**.

```sh
make MCU=f0 flash   # ou MCU=f4
picocom -b 115200 /dev/ttyACM0
```

---

## 1. Cortex-M0 × Cortex-M4

| | Cortex-M0 (STM32F0) | Cortex-M4F (STM32F4) |
|---|---|---|
| Arquitetura | ARMv6-M | ARMv7E-M |
| Instruções | Thumb (16 bits) + poucas Thumb-2 (BL, MRS, MSR, DMB/DSB/ISB) | Thumb-2 completo, DSP (SIMD, saturação), MAC de 1 ciclo |
| Divisão por hardware | **não** (libgcc `__aeabi_uidiv`) | sim (`UDIV`/`SDIV`, 2–12 ciclos) |
| Multiplicação | 1 ciclo (opção do fabricante; F0 = 1 ciclo) | 1 ciclo, 32×32→64 em 1 ciclo |
| FPU | não | FPv4-SP (float de 32 bits) |
| Pipeline | 3 estágios, von Neumann | 3 estágios + branch speculation, Harvard (I-Code/D-Code/System) |
| Acesso desalinhado | **HardFault** | permitido para LDR/STR (exceto LDM/STM/LDRD) |
| Interrupções externas | até 32 | até 240 (F446 usa 97) |
| Bits de prioridade | 2 (4 níveis) | 4 no STM32 (16 níveis) + preempção/subprioridade |
| VTOR (relocar tabela de vetores) | **não** (no STM32F0 usa-se remap da SRAM em 0x0) | sim |
| Bit-banding | não | sim |
| MPU | não | sim (8 regiões) |
| Exceções de falha | só HardFault | HardFault, MemManage, BusFault, UsageFault |
| DWT cycle counter | não | sim |

### Registradores do núcleo (iguais nos dois)

```
r0-r3    argumentos / retorno / scratch (caller-saved)
r4-r11   variáveis locais (callee-saved)      (M0: r8-r11 só com MOV/ADD/CMP)
r12      IP: scratch
r13 SP   stack pointer -> MSP (main) ou PSP (process), banked
r14 LR   link register: endereço de retorno (ou EXC_RETURN em exceções)
r15 PC   program counter
xPSR     APSR (flags N Z C V Q) | IPSR (número da exceção ativa) | EPSR (bit T)
PRIMASK  1 = mascara todas as interrupções configuráveis (cpsid i / cpsie i)
CONTROL  bit0 nPRIV, bit1 SPSEL (MSP/PSP), bit2 FPCA (M4)
(M4) BASEPRI, FAULTMASK, s0-s31, FPSCR
```

O **bit T** do EPSR precisa ser sempre 1 (Thumb). Por isso todo endereço de função tem o **bit 0 = 1** (`main` em `0x080001d5`, por exemplo): ao saltar com `BX`/`BLX`, o bit 0 vai para o T. Um ponteiro de função com bit 0 = 0 causa HardFault (UsageFault INVSTATE no M4).

### Modos

- **Thread mode**: código normal (`main`).
- **Handler mode**: dentro de uma exceção/interrupção (sempre usa MSP e é privilegiado).

Ao entrar numa exceção o hardware empilha automaticamente **r0-r3, r12, LR, PC, xPSR** (8 words; no M4 com FPU ativa, também s0-s15 e FPSCR, com *lazy stacking*) e coloca em LR um valor mágico `0xFFFFFFFx` (EXC_RETURN). Por isso um handler de interrupção pode ser uma **função C comum** — não precisa de `__attribute__((interrupt))`.

---

## 2. O mapa de memória de 4 GB

O Cortex-M define um mapa fixo, dividido em regiões de 512 MB. Tudo — Flash, RAM, periféricos, registradores do núcleo — é acessado como **memória** (*memory-mapped I/O*):

```
0xFFFF_FFFF ┌───────────────────────────┐
            │ Vendor specific           │
0xE010_0000 ├───────────────────────────┤
            │ Private Peripheral Bus    │  SysTick 0xE000E010, NVIC 0xE000E100,
0xE000_0000 │ (núcleo)                  │  SCB 0xE000ED00, DWT, ITM, FPB, MPU
            ├───────────────────────────┤
            │ External device / RAM     │  (FMC/FSMC em chips maiores)
0xA000_0000 ├───────────────────────────┤
0x6000_0000 │                           │
            ├───────────────────────────┤
            │ Peripheral                │  APB, AHB (RCC, GPIO, USART...)
0x4000_0000 │  + bit-band alias 0x42..  │
            ├───────────────────────────┤
            │ SRAM                      │
0x2000_0000 │  + bit-band alias 0x22..  │
            ├───────────────────────────┤
            │ Code                      │  Flash 0x0800_0000, System memory,
0x0000_0000 └───────────────────────────┘  alias de boot em 0x0000_0000
```

### STM32F072RB

| Endereço | Tamanho | Conteúdo |
|---|---|---|
| `0x0000_0000` | — | **alias** de Flash, System memory ou SRAM conforme BOOT0/option bytes |
| `0x0800_0000` | 128 KB | Flash principal (64 páginas de 2 KB) |
| `0x1FFF_C800` | 12 KB | System memory (bootloader USART/USB/I2C da ST) |
| `0x1FFF_F7AC..` | | Unique ID, calibração do ADC/sensor, tamanho da Flash |
| `0x1FFF_F800` | | Option bytes |
| `0x2000_0000` | 16 KB | SRAM |
| `0x4000_0000` | | APB (TIM2..., USART2, I2C, CAN, PWR, DAC... TIM1, SPI1, USART1, ADC) |
| `0x4002_0000` | | AHB1 (DMA, **RCC 0x4002_1000**, FLASH 0x4002_2000, CRC) |
| `0x4800_0000` | | AHB2: **GPIOA..GPIOF** (acesso em 1 ciclo) |
| `0xE000_0000` | | Periféricos do núcleo |

### STM32F446RE

| Endereço | Tamanho | Conteúdo |
|---|---|---|
| `0x0800_0000` | 512 KB | Flash (setores 4×16 KB, 1×64 KB, 3×128 KB) |
| `0x1FFF_0000` | 30 KB | System memory (bootloader) |
| `0x1FFF_7800` | 528 B | OTP |
| `0x1FFF_7A10..` | | Unique ID, tamanho da Flash, calibração |
| `0x1FFF_C000` | | Option bytes |
| `0x2000_0000` | 112 KB | SRAM1 |
| `0x2001_C000` | 16 KB | SRAM2 (contígua à SRAM1) |
| `0x4000_0000` | | APB1 (45 MHz): TIM2-7,12-14, RTC, WWDG, IWDG, SPI2/3, USART2/3, UART4/5, I2C1-3, CAN1/2, PWR, DAC |
| `0x4001_0000` | | APB2 (90 MHz): TIM1/8, USART1/6, ADC1-3, SPI1/4, SYSCFG, EXTI, TIM9-11, SAI |
| `0x4002_0000` | | AHB1 (180 MHz): **GPIOA..H**, CRC, **RCC 0x4002_3800**, FLASH 0x4002_3C00, BKPSRAM, DMA1/2 |
| `0x5000_0000` | | AHB2: USB OTG FS, DCMI |
| `0xA000_0000` | | FMC / QUADSPI (registradores) |

> O endereço base de cada periférico está na tabela "Memory map" / "Register boundary addresses" do Reference Manual (RM0091 §2.2, RM0390 §2.2). Os headers `common/*/stm32f*.h` reproduzem essas tabelas.

---

## 3. Barramentos e por que isso importa

```
             Cortex-M4
        I-Code  D-Code  System
          │       │       │        DMA1    DMA2
          ▼       ▼       ▼         │       │
   ┌───────────── AHB bus matrix ───────────────┐
   │                                            │
 Flash(ART)   SRAM1   SRAM2   AHB1(GPIO,RCC)  AHB2   FMC
                                │
                       ┌────────┴────────┐
                     APB1 (/4)        APB2 (/2)
                     45 MHz           90 MHz
```

Consequências práticas:

1. **Todo periférico começa sem clock.** Antes de tocar num registrador, ligue o bit correspondente em `RCC->AHBxENR`/`APBxENR`. Sem clock, escritas são ignoradas e leituras retornam 0.
2. **Acessos ao APB são mais lentos** que ao AHB (o barramento roda a uma fração do HCLK e há uma ponte AHB→APB). No F0 o GPIO fica direto no AHB2 justamente para ser rápido.
3. **Errata F4**: logo após habilitar o clock de um periférico, é preciso esperar 2 ciclos antes de acessá-lo. Uma leitura de volta do registrador RCC resolve: `(void)RCC->AHB1ENR;`.
4. O **DMA** é outro mestre no barramento: compete com a CPU (módulo 15).
5. A Flash é lenta comparada ao núcleo: acima de 24 MHz (F0) / 30 MHz (F4) a CPU precisa de *wait states* (módulo 04). O ART accelerator do F4 (cache + prefetch) esconde isso na maior parte do tempo.

---

## 4. Boot: o que acontece em `0x0000_0000`

No reset, o núcleo **sempre** lê:

| Endereço | Conteúdo | Vai para |
|---|---|---|
| `0x0000_0000` | valor inicial do stack pointer | **MSP** |
| `0x0000_0004` | endereço do Reset_Handler (bit 0 = 1) | **PC** |

E o STM32 decide o que aparece em `0x0000_0000` pelo pino **BOOT0** (e option bytes):

| BOOT0 | Alias em 0x0 | Uso |
|---|---|---|
| 0 | Flash principal (`0x0800_0000`) | normal — a Nucleo tem BOOT0 em GND |
| 1 | System memory | bootloader da ST (gravar pela UART/USB sem ST-LINK) |
| 1 + BOOT1/nBOOT1=0 | SRAM | executar da RAM |

Por isso o linker script coloca a **tabela de vetores no início da Flash**: com BOOT0=0 ela aparece em 0x0. Detalhes no módulo 02.

---

## 5. Acessando registradores em C

Um registrador é só um endereço. Três formas, da mais crua à usada no curso:

```c
/* 1) Ponteiro "na mão" */
*(volatile uint32_t *)0x40020014 |= (1U << 5);          /* GPIOA->ODR no F4 */

/* 2) Macro por registrador */
#define GPIOA_ODR (*(volatile uint32_t *)0x40020014)
GPIOA_ODR |= (1U << 5);

/* 3) Struct sobreposta ao bloco do periférico (estilo CMSIS, usado no curso) */
typedef struct {
    volatile uint32_t MODER;    /* 0x00 */
    volatile uint32_t OTYPER;   /* 0x04 */
    volatile uint32_t OSPEEDR;  /* 0x08 */
    volatile uint32_t PUPDR;    /* 0x0C */
    volatile uint32_t IDR;      /* 0x10 */
    volatile uint32_t ODR;      /* 0x14 */
    /* ... */
} GPIO_TypeDef;
#define GPIOA ((GPIO_TypeDef *)0x40020000)
GPIOA->ODR |= (1U << 5);
```

A struct funciona porque cada campo `uint32_t` ocupa 4 bytes e o compilador os coloca em sequência — o offset do campo é o offset do registrador. Buracos no mapa viram campos `RESERVED`. A vantagem: um único ponteiro base, o GDB entende (`p/x *GPIOA`), e a mesma struct serve para GPIOA, GPIOB...

### Por que `volatile`?

Sem `volatile`, o compilador assume que só o seu código altera a memória. Ele pode:
- remover uma leitura repetida (`while (!(USART2->SR & TXE));` vira laço infinito ou some),
- juntar duas escritas (`ODR = 1; ODR = 0;` vira só `ODR = 0`),
- reordenar acessos.

`volatile` diz: "cada acesso escrito no código é um acesso real à memória, na ordem escrita". Use em: registradores de hardware e variáveis compartilhadas entre uma ISR e o `main`.

### Operações de bit

```c
reg |=  (1U << n);            /* set   */
reg &= ~(1U << n);            /* clear */
reg ^=  (1U << n);            /* toggle */
if (reg & (1U << n)) { }      /* test  */
reg = (reg & ~(MASK << POS)) | (VAL << POS);   /* escreve um campo de vários bits */
```

Use sempre `1U` (unsigned): `1 << 31` em `int` é comportamento indefinido.

### Read-modify-write e atomicidade

`reg |= x` são **três** operações: LDR, ORR, STR. Se uma interrupção alterar o mesmo registrador entre o LDR e o STR, a alteração da ISR é perdida. Soluções:
- registradores com **set/reset separados** (GPIO `BSRR`: escrever 1 no bit n seta, no bit n+16 reseta — atômico);
- flags **rc_w1** (escreve 1 para limpar: `EXTI->PR`) ou **rc_w0** (escreve 0 para limpar: `TIMx->SR`) — escreve-se só a máscara, sem ler antes;
- **bit-banding** (M3/M4);
- desabilitar interrupções em volta da seção crítica (`__disable_irq()` / `__enable_irq()`).

Notação do RM: `rw` leitura/escrita, `r` só leitura, `w` só escrita, `rc_w1`/`rc_w0` limpa escrevendo 1/0, `rs` software seta, hardware limpa, `t` toggle.

---

## 6. Bit-banding (só F4)

Duas regiões de 1 MB têm um **alias** de 32 MB em que cada *bit* vira uma *word*:

| Região real | Alias |
|---|---|
| SRAM `0x2000_0000`–`0x200F_FFFF` | `0x2200_0000`–`0x23FF_FFFF` |
| Periféricos `0x4000_0000`–`0x400F_FFFF` | `0x4200_0000`–`0x43FF_FFFF` |

```
endereço_alias = base_alias + (endereço_byte - base_região) * 32 + bit * 4
```

Exemplo: `GPIOA->ODR` (F4) = `0x4002_0014`, bit 5:
```
0x4200_0000 + 0x0002_0014 * 32 + 5 * 4 = 0x4240_0294
```
Escrever 1/0 em `0x4240_0294` muda **só** o bit 5, em uma única transação atômica do barramento. Veja `BITBAND_PERIPH()` em `src/main.c`. (O Cortex-M7 e os STM32 mais novos não têm bit-banding; prefira BSRR e flags rc_w1 em código portável.)

---

## 7. Endianness e alinhamento

- STM32 é **little-endian**: `0x12345678` na memória fica `78 56 34 12`. Note isso no `xxd` do `.bin`.
- No **Cortex-M0**, ler um `uint32_t` em endereço não múltiplo de 4 gera **HardFault**. Cuidado ao converter `uint8_t buffer[]` em `uint32_t *`. Use `memcpy` ou monte o valor byte a byte.
- Structs são alinhadas pelo compilador; `__attribute__((packed))` remove o preenchimento, mas gera acessos byte a byte (lentos) no M0.

---

## 8. Identificando o chip

| Registrador | Endereço | Conteúdo |
|---|---|---|
| `SCB->CPUID` | `0xE000_ED00` | Implementer `0x41` (ARM), PartNo `0xC20` (M0), `0xC24` (M4), revisão |
| `DBGMCU->IDCODE` | F0 `0x4001_5800`, F4 `0xE004_2000` | DEV_ID[11:0]: `0x448` = F07x, `0x421` = F446; REV_ID[31:16] |
| Flash size | F0 `0x1FFF_F7CC`, F4 `0x1FFF_7A22` | 16 bits, em KB |
| Unique ID | F0 `0x1FFF_F7AC`, F4 `0x1FFF_7A10` | 96 bits únicos por chip |

Saída típica (F446):
```
CPUID       = 0x410FC241  (implementer 0x41, partno 0xC24 = Cortex-M4, r0p1)
IDCODE      = 0x10006421  (DEV_ID 0x421, REV_ID 0x1000)
Flash       = 512 KB
...
  tabela_const     0x08000c50  (.rodata, FLASH)
  var_inicializada 0x20000000  (.data, RAM) = 0xCAFEBABE
  var_zerada       0x20000004  (.bss,  RAM) = 0
  var_local        0x2001ffe4  (pilha)      = 42
```

---

## Exercícios

1. Calcule o endereço de bit-band do bit 13 de `GPIOC->IDR` no F446 e leia o botão por ele.
2. No F0, crie `uint8_t buf[8]` e leia `*(uint32_t *)&buf[1]`. O que acontece? Use o GDB para ver em que handler o programa parou e o valor de `xPSR`.
3. Leia os 96 bits do Unique ID e use-os para gerar um número de série em hexadecimal.
4. Descubra no RM0390 em qual barramento está o TIM1 e qual a frequência máxima do clock dele.
