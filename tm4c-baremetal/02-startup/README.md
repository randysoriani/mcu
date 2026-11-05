# Módulo 02 — Startup

Entre o reset e o `main()` alguém precisa definir a pilha, preparar as variáveis globais, habilitar a FPU e montar a tabela de interrupções.

| Arquivo | Conteúdo |
|---|---|
| [`123/startup_123.s`](123/startup_123.s) | startup em **assembly** do TM4C123 (139 IRQs) |
| [`129/startup_129.s`](129/startup_129.s) | startup em **assembly** do TM4C129 (114 IRQs) |
| [`../common/123/startup.c`](../common/123/startup.c), [`../common/129/startup.c`](../common/129/startup.c) | o mesmo em **C**, usado pelos demais módulos |
| [`src/main.c`](src/main.c) | teste: confere `.data`/`.bss` e sobrescreve o `SysTick_Handler` |

```sh
make MCU=123 flash     # LED a 1 Hz = startup OK, 10 Hz = erro
```

---

## 1. Reset

1. Núcleo lê `0x0000_0000` → **MSP**; `0x0000_0004` → **PC** (bit 0 = 1).
2. Executa em Thread mode privilegiado. Clock = **PIOSC 16 MHz** (oscilador interno de precisão), periféricos sem clock, FPU desligada.

No TM4C a Flash começa no endereço 0: a tabela de vetores do linker script está exatamente onde o núcleo a procura.

## 2. O que o startup faz

```
Reset_Handler:
  habilita a FPU (CPACR |= 0xF << 20; DSB; ISB)
  copia .data   FLASH (_sidata) -> RAM (_sdata.._edata)
  zera  .bss    (_sbss.._ebss)
  chama construtores (__init_array)
  main()
```

- **.data**: variáveis inicializadas vivem na RAM, mas o valor inicial é gravado na Flash e copiado aqui.
- **.bss**: o C garante zero para globais sem inicializador; a RAM tem lixo após o power-on.
- **FPU**: compilamos com `-mfloat-abi=hard`; sem habilitar CP10/CP11 a primeira instrução VFP gera UsageFault (NOCP).

## 3. Tabela de vetores

| Offset | Exceção |
|---|---|
| 0x00 | SP inicial |
| 0x04 | Reset |
| 0x08 | NMI |
| 0x0C | HardFault |
| 0x10 / 0x14 / 0x18 | MemManage / BusFault / UsageFault |
| 0x2C | SVCall |
| 0x30 | DebugMonitor |
| 0x38 | PendSV |
| 0x3C | SysTick |
| 0x40 + 4n | IRQ n |

IRQs mais usadas no curso (atenção: **os números diferem entre 123 e 129** a partir da 38):

| Periférico | 123 | 129 |
|---|---|---|
| GPIOA..E | 0..4 | 0..4 |
| UART0 | 5 | 5 |
| ADC0 SS3 | 17 | 17 |
| Watchdog | 18 | 18 |
| TIMER0A / 1A / 2A | 19 / 21 / 23 | 19 / 21 / 23 |
| GPIOF | 30 | 30 |
| CAN0 | **39** | **38** |
| Hibernação | **43** | **41** |
| uDMA software / erro | **46 / 47** | **44 / 45** |
| GPIOJ | — | 51 |
| GPION | — | 73 |
| FPU | 106 | 67 |
| PWM1 gen0..3 | 134..137 | — |

Tabela completa: datasheet, capítulo *Interrupts* ("Interrupts" table). Os headers do curso definem `XXX_IRQn` e o startup define `XXX_Handler`.

## 4. Handlers fracos

```c
void Default_Handler(void) { for (;;) { } }
void TIMER0A_Handler(void) __attribute__((weak, alias("Default_Handler")));
```
```asm
    .weak      TIMER0A_Handler
    .thumb_set TIMER0A_Handler, Default_Handler
```
Defina `void TIMER0A_Handler(void) { ... }` no seu código e o linker usa a sua. Errar o nome compila sem erro e cai no `Default_Handler` — confira com `arm-none-eabi-nm build/123/x.elf | grep Handler` (W = fraco, T = forte).

Convenção do curso: `<PERIFÉRICO>_Handler` (`UART0_Handler`, `GPIOF_Handler`, `TIMER0A_Handler`, `ADC0SS3_Handler`...). O TivaWare usa nomes diferentes (`UARTIntHandler`...) porque lá a tabela é editada à mão em cada projeto.

## 5. Assembly × C

O startup em assembly (Thumb-2) usa pós-incremento e bloco IT:

```asm
1:  cmp   r0, r1
    ittt  lo
    ldrlo r3, [r2], #4      @ r3 = *src++
    strlo r3, [r0], #4      @ *dst++ = r3
    blo   1b
```

O em C faz o mesmo e usa `__attribute__((optimize("no-tree-loop-distribute-patterns")))` para o GCC não trocar os laços por `memcpy`/`memset`. A tabela de vetores do C é um array `const` na seção `.isr_vector`, com `used` para não ser descartado. As duas versões foram geradas a partir da mesma lista de IRQs (sem erros de digitação entre elas).

## 6. Relocando a tabela

`SCB->VTOR` (Cortex-M4) aceita qualquer endereço alinhado a 1 KB (tabela de 155 words no 123 → próxima potência de 2 = 1024 bytes). Bootloaders da TI (o da ROM, ou o "boot_serial" do TivaWare) carregam a aplicação em, p. ex., `0x0000_2800` e fazem `VTOR = 0x2800`.

---

## Exercícios
1. Comente a cópia de `.data` no `startup_123.s` e observe o LED piscar rápido.
2. Remova a habilitação da FPU, use um `float` no `main` e leia `SCB->CFSR` (bit 19, NOCP) no GDB.
3. Escreva um `HardFault_Handler` que pisca o LED vermelho (PF1) em SOS no 123.
4. Por que um erro na posição de uma IRQ na tabela só aparece quando aquela interrupção é habilitada?
