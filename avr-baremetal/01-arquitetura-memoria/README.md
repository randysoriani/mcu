# Módulo 01 — Arquitetura e organização da memória

> **Objetivo deste módulo:** entender como os dois processadores do curso enxergam a memória e os periféricos. O AVR de 8 bits e o AVR32 levam o mesmo nome, mas são arquiteturas **muito diferentes** — e entender por que é a melhor forma de aprender o que é uma "arquitetura".

Código: [`src/main.c`](src/main.c) (o mesmo para os dois chips) — imprime o tamanho dos tipos de C, alguns registradores e onde cada tipo de variável foi parar.

```sh
make MCU=m328p flash && picocom -b 115200 /dev/ttyACM0
```

Saída real (ATmega328P no QEMU):
```
=== Modulo 01: arquitetura e memoria ===
ATmega328P: AVR de 8 bits, arquitetura Harvard
  sizeof(int) = 2 bytes, sizeof(long) = 4, sizeof(void*) = 2
  registrador r24 visto pelo endereco 0x18: 0x3e
  SREG = 0xa1 (bit 7 = interrupcoes ligadas)
  SP   = 0x08e5 (pilha no fim da RAM, RAMEND = 0x08ff)

Onde cada coisa mora:
  main()           0x51 (Flash)
  tabela_const     0x35c
  var_inicializada 0x100 = 0xcafe
  var_zerada       0x38a = 0
  var_local        0x8fa = 42 (pilha)

No AVR8 o endereco de main() e contado em PALAVRAS de 16 bits:
  em bytes, main() esta em 0xa2
```

---

## 0. Revisão: bits, bytes e hexadecimal

| Termo | Tamanho | Tipo em C |
|---|---|---|
| bit | 0 ou 1 | — |
| byte | 8 bits (0–255) | `uint8_t` |
| word (no AVR8) | 16 bits | `uint16_t` |
| word (no AVR32) | 32 bits | `uint32_t` |

Hexadecimal (`0x...`): cada dígito são 4 bits. `0x20` = `0010 0000` = bit 5 ligado. Bits são numerados da direita para a esquerda a partir de 0.

> Note que "word" muda de significado: no AVR de 8 bits as instruções têm 16 bits e o manual chama 16 bits de "word"; no AVR32, word é 32 bits.

---

## 1. Dentro de um microcontrolador

```
┌─────────────────── chip ───────────────────┐
│  Núcleo (CPU) ◄══ barramentos ══► Flash      │  (programa, não apaga)
│                                ══► RAM        │  (variáveis, apaga)
│                                ══► EEPROM     │  (dados, não apaga) — só no 328P
│                                ══► Periféricos│──► pinos
└──────────────────────────────────────────────┘
```

| | ATmega328P | AT32UC3A0512 |
|---|---|---|
| Núcleo | AVR de 8 bits (registradores de 8 bits) | AVR32 UC (registradores de 32 bits) |
| Clock máximo | 20 MHz (16 MHz no Uno) | 66 MHz (48 MHz no curso) |
| Flash | 32 KB | 512 KB |
| RAM | **2 KB** | 64 KB |
| EEPROM | 1 KB | não tem (módulo 16) |
| Desempenho aproximado | 1 instrução por ciclo → ~16 milhões/s, mas cada uma mexe em 8 bits | ~1,2 milhões de instruções por MHz, de 32 bits |

---

## 2. O AVR de 8 bits (ATmega328P)

### 2.1 Arquitetura Harvard: memórias separadas
No ATmega, a memória de **programa** (Flash) e a memória de **dados** (RAM) são **separadas**, com barramentos e **endereços próprios**. O endereço `0x0100` na Flash e o endereço `0x0100` na RAM são lugares diferentes. Isso se chama **arquitetura Harvard**.

```
 Espaço de PROGRAMA (Flash)          Espaço de DADOS                     EEPROM
 endereços de 16 bits (words)        endereços em bytes                  (acessada por registradores)
 0x0000 ┌────────────────┐           0x0000 ┌───────────────────────┐   0x000 ┌──────────┐
        │ tabela de      │                  │ r0..r31 (registradores│         │ 1 KB     │
        │ vetores        │           0x0020 ├───────────────────────┤   0x3FF └──────────┘
        │ programa       │                  │ 64 registradores de E/S│
        │                │           0x0060 ├───────────────────────┤
        │ (bootloader do │                  │ 160 de E/S estendida  │
        │  Arduino no    │           0x0100 ├───────────────────────┤
 0x3FFF │  fim)          │                  │ SRAM (2 KB)           │
        └────────────────┘           0x08FF └───────────────────────┘
   16 K words = 32 KB
```

Consequências práticas:
- **Ponteiros comuns só enxergam o espaço de dados.** Para ler uma tabela gravada na Flash é preciso uma instrução especial (`LPM`). Por isso, no AVR, até as constantes (`const`) são copiadas para a RAM — veja `tabela_const` em `0x35c` na saída acima. O módulo 03 mostra como evitar isso.
- **Endereços de código são contados em words de 16 bits.** `main()` aparece como `0x51`, que é o byte `0xA2`. As ferramentas (avr-objdump) mostram bytes; o PC do processador conta words.
- **2 KB de RAM é pouco.** Strings em `kprintf`, buffers e a pilha disputam esse espaço.

### 2.2 Os 32 registradores e o espaço de dados
O núcleo tem **32 registradores de 8 bits** (`r0`–`r31`). A maioria das instruções opera entre eles em **um ciclo**. Curiosidade importante: eles **também aparecem no espaço de dados**, nos endereços 0x00 a 0x1F (o programa leu `r24` pelo endereço `0x18`).

| Registradores | Uso |
|---|---|
| `r0`, `r1` | o GCC usa `r1` sempre valendo 0 ("registrador zero") e `r0` como rascunho |
| `r2`–`r17` | variáveis preservadas entre chamadas |
| `r18`–`r27` | rascunho e argumentos |
| `r26:r27` = **X**, `r28:r29` = **Y**, `r30:r31` = **Z** | pares usados como **ponteiros** de 16 bits |

Registradores especiais (no espaço de E/S):
- **SREG** (`0x5F`): flags do último resultado — C (vai-um), Z (zero), N (negativo), V (estouro), S, H, T — e o bit **I** (bit 7), que liga/desliga **todas** as interrupções.
- **SP** (`SPH:SPL`, `0x5E:0x5D`): ponteiro da pilha. Começa no fim da RAM (`0x08FF`) e desce.
- **PC**: não é acessível como registrador; avança por words na Flash.

### 2.3 Registradores de periféricos: E/S
Todos os periféricos (portas, timers, serial...) são controlados por registradores de 8 bits mapeados no espaço de dados, de `0x20` a `0xFF`:
- `0x20`–`0x5F`: os 64 "registradores de E/S". Eles também têm um endereço curto (dados − 0x20) usado pelas instruções rápidas `IN`, `OUT`, `SBI` (liga um bit) e `CBI` (desliga um bit). O compilador escolhe essas instruções sozinho.
- `0x60`–`0xFF`: E/S "estendida", acessada com `LDS`/`STS` como memória comum.

Em C, um registrador é um endereço:
```c
#define PORTB (*(volatile uint8_t *)0x25)
PORTB |= (1 << 5);          /* liga o bit 5: o compilador gera "sbi 0x05, 5" (1 instrução) */
```
O `volatile` diz ao compilador que o valor pode mudar "por fora" (pelo hardware) e que cada acesso escrito deve acontecer de verdade. Os headers do curso já declaram todos os registradores assim ([`common/m328p/m328p.h`](../common/m328p/m328p.h)).

### 2.4 Tamanhos dos tipos
No AVR de 8 bits, **`int` tem 16 bits** (faixa −32 768 a 32 767). Contas que passam disso estouram silenciosamente:
```c
int x = 300 * 200;          /* 60 000 não cabe: vira -5536 */
uint32_t y = 300UL * 200;   /* correto */
```
Use os tipos de tamanho explícito (`uint8_t`, `uint16_t`, `uint32_t`) e lembre que **operações de 32 bits custam várias instruções** num processador de 8 bits.

---

## 3. O AVR32 (AT32UC3A0512)

### 3.1 Um mapa de memória único
O AVR32 tem um **único espaço de endereços de 32 bits**, como o ARM Cortex-M: Flash, RAM e periféricos têm endereços diferentes no mesmo espaço, e um ponteiro comum alcança tudo.

```
0xFFFF_FFFF ┌──────────────────────────────┐
            │ periféricos PBA (0xFFFF_0000) │  GPIO, USART, SPI, TWI, TC, PWM, ADC, PM, INTC...
            │ periféricos HSB (0xFFFE_0000) │  USB, Ethernet (MACB), FLASHC, HMATRIX
            ├──────────────────────────────┤
0x8080_0000 │ user page (512 bytes)         │
0x8000_0000 │ Flash (512 KB)                │  o programa
            ├──────────────────────────────┤
0x0000_0000 │ SRAM (64 KB)                  │  variáveis e pilha
            └──────────────────────────────┘
```

### 3.2 Registradores
16 registradores de 32 bits: `r0`–`r12` de uso geral, **`r13` = SP**, **`r14` = LR** (endereço de retorno), **`r15` = PC**. Além deles, há **registradores de sistema** (status `SR`, `EVBA`, `COUNT`, `COMPARE`...), acessados com as instruções `mfsr`/`mtsr` (no curso, as macros `mfsr()`/`mtsr()` de [`uc3a.h`](../common/uc3a/uc3a.h)).

### 3.3 Barramentos
```
  CPU ══► HSB (barramento rápido) ══► Flash, SRAM, USB, Ethernet
                    ║
               ponte HSB-PB
                    ╠══► PBA (periféricos A: GPIO, USART, SPI, TC, PWM, ADC...)
                    ╚══► PBB (periféricos B: USB, Ethernet, FLASHC...)
```
Cada barramento pode ter seu próprio divisor de clock (módulo 04). No reset, todos os clocks dos periféricos já estão **ligados** (diferente do STM32/TM4C).

### 3.4 Periféricos em C
```c
typedef struct { volatile uint32_t VAL, SET, CLR, TGL; } GPIO_REG4;
GPIO_PORT(PB(27))->OVR.TGL = 1UL << 27;   /* inverte o pino PB27 numa única escrita */
```
Muitos registradores do UC3 vêm em quatro versões: o valor, **SET** (escrever 1 liga bits), **CLR** (escrever 1 desliga bits) e **TGL** (escrever 1 inverte). Isso evita o ler-modificar-escrever (módulo 05).

### 3.5 Tipos
No AVR32, `int` e ponteiros têm **32 bits**. O mesmo código C se comporta diferente nos dois chips se depender do tamanho de `int`: por isso o curso usa `uint16_t`/`uint32_t` explícitos.

---

## 4. Comparando

| | AVR 8 bits (ATmega328P) | AVR32 (UC3A) |
|---|---|---|
| Memórias | **separadas** (Harvard): programa, dados, EEPROM | **um mapa** só |
| Ponteiro alcança a Flash? | não (precisa de `LPM`) | sim |
| `const` fica | na RAM (a menos de `PROGMEM`) | na Flash |
| Registradores | 32 × 8 bits | 16 × 32 bits (+ registradores de sistema) |
| `sizeof(int)` | 2 | 4 |
| Periféricos | registradores de 8 bits em `0x20`–`0xFF` | registradores de 32 bits a partir de `0xFFFE_0000` |
| Clock dos periféricos | sempre ligado (o `PRR` pode desligar) | ligado no reset (máscaras no PM podem desligar) |

---

## Exercícios
1. Mude `tabela_const` para 200 elementos e veja o `avr-size`: em qual coluna ela aparece?
2. No ATmega, leia o endereço de dados `0x001E` (registrador `r30`) e explique o valor.
3. Calcule quanto da RAM do 328P sobra para a pilha neste programa, usando o `__heap_start` (módulo 03) e o SP impresso.
4. Escreva `int x = 300 * 200;` e imprima nos dois chips. Explique a diferença.
