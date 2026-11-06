# Módulo 20 — Autoprogramação da Flash e bootloader

> **Objetivo deste módulo:** entender como um programa grava a própria Flash — a base de todo **bootloader** (como o do Arduino, que grava programas pela serial) e de atualizações de firmware em campo. No ATmega isso tem uma regra especial: só funciona de dentro da **seção de boot**.

Código:
- [`m328p/main.c`](m328p/main.c) + [`m328p/linker_boot.ld`](m328p/linker_boot.ld) — a rotina de gravação mora na seção de boot (0x7000); o programa grava uma página e lê de volta
- [`uc3a/main.c`](uc3a/main.c) — grava a última página da Flash pelo FLASHC

---

## 1. O que é um bootloader

Um pequeno programa que roda **antes** do programa principal e pode **substituí-lo**, recebendo o novo código por serial, USB, rede... Sem precisar de um gravador.

```
 Flash do Arduino Uno (32 KB)
 0x0000 ┌──────────────────────────┐
        │ seu programa              │  <- o bootloader grava aqui
        │                           │
 0x7E00 ├──────────────────────────┤
        │ optiboot (512 bytes)      │  <- roda no reset (fuse BOOTRST)
 0x7FFF └──────────────────────────┘
```
No reset, o optiboot espera ~0,5 s por um comando do avrdude pela serial. Se chegar, recebe o programa e o grava página por página; se não, salta para 0x0000.

Na EVK1100, o bootloader **DFU** ocupa os primeiros 8 KB e o programa começa em `0x8000_2000` (módulos 00 e 02).

---

## 2. ATmega328P

### 2.1 A seção de boot e o RWW
A Flash do ATmega é dividida em duas áreas:
- **RWW** (*Read-While-Write*): a área de aplicação. Enquanto ela está sendo gravada, a CPU **não consegue lê-la** (a CPU para se tentar executar código de lá).
- **NRWW** (*No-Read-While-Write*): inclui a **seção de boot**. A CPU pode continuar executando dali enquanto a área RWW é gravada.

E a regra principal: **a instrução `SPM` (*Store Program Memory*) só funciona se executada de dentro da seção de boot.** Um programa comum não consegue gravar a Flash — proteção contra um programa com defeito se apagar.

O tamanho da seção de boot vem dos fuses **BOOTSZ1:0** (hfuse):

| BOOTSZ1:0 | Tamanho | Começa em (bytes) |
|---|---|---|
| 11 | 256 words (512 B) | 0x7E00 (o optiboot) |
| 10 | 512 words (1 KB) | 0x7C00 |
| 01 | 1024 words (2 KB) | 0x7800 |
| **00** | **2048 words (4 KB)** | **0x7000** |

O exemplo usa **BOOTSZ = 00** e põe as funções de gravação em 0x7000.

### 2.2 Colocando código na seção de boot
No código:
```c
#define BOOT __attribute__((section(".bootloader"), noinline))
BOOT void flash_write_page(uint16_t addr, const uint8_t *data) { ... }
```
No linker script do módulo ([`linker_boot.ld`](m328p/linker_boot.ld)):
```ld
.bootloader 0x7000 : { KEEP(*(.bootloader)) } > text
```
Conferindo no binário:
```
$ avr-objdump -h build/m328p/20-autoprogramacao-e-bootloader.elf
  1 .text         000006e6  00000000 ...
  3 .bootloader   0000009c  00007000 ...
$ avr-nm build/m328p/*.elf | grep -E "flash_write_page|spm_"
00007020 T flash_write_page
00007000 t spm_cmd
00007010 t spm_fill
```
Atenção: tudo o que a rotina de boot chama também precisa estar na seção de boot (por isso `spm_cmd`/`spm_fill` também têm o atributo, e a rotina não chama funções da biblioteca).

### 2.3 SPMCSR (0x57) e a sequência de gravação
| Bit | Nome | Função |
|---|---|---|
| 7 | SPMIE | interrupção "SPM pronto" |
| 6 | RWWSB | área RWW ocupada |
| 5 | SIGRD | ler a assinatura |
| 4 | **RWWSRE** | reabilitar a leitura da área RWW |
| 3 | BLBSET | lock bits / leitura de fuses (módulo 04) |
| 2 | **PGWRT** | gravar a página |
| 1 | **PGERS** | apagar a página |
| 0 | **SPMEN** | habilita a instrução SPM |

A Flash é gravada por **páginas de 128 bytes** (64 words):
```
1. apagar a página:  Z = endereço, SPMCSR = PGERS | SPMEN, spm
2. reabilitar RWW:   SPMCSR = RWWSRE | SPMEN, spm
3. encher o buffer:  64 vezes: r1:r0 = word, Z = endereço, SPMCSR = SPMEN, spm
4. gravar:           Z = endereço, SPMCSR = PGWRT | SPMEN, spm
5. reabilitar RWW
```
Cada `spm` deve vir **até 4 ciclos** depois da escrita no SPMCSR (por isso é feito em assembly inline), com interrupções desligadas. Apagar/gravar leva ~4 ms cada.

### 2.4 Gravando o exemplo: precisa de ISP e muda os fuses
```sh
make MCU=m328p
avrdude -p m328p -c usbasp -U flash:w:build/m328p/20-autoprogramacao-e-bootloader.hex:i
avrdude -p m328p -c usbasp -U hfuse:w:0xD9:m        # BOOTSZ = 00, BOOTRST = 1 (veja abaixo)
```
- Pelo bootloader do Arduino **não dá**: o optiboot ocupa a seção de boot e se protege.
- Com **BOOTRST = 0** (como no Uno, hfuse `0xDE`), o reset iria para o início da seção de boot — a nossa rotina, que não é um bootloader! Por isso o exemplo usa **BOOTRST = 1** (desprogramado): hfuse = `0xD9` = `1101 1001` (BOOTSZ1:0 = 00, BOOTRST = 1, demais bits iguais aos do Uno). Confira com uma calculadora de fuses antes de gravar.
- O programa grava a página 0x6F80 e, a cada reset, o primeiro byte aumenta — prova de que está na Flash.

### 2.5 Devolvendo o bootloader do Arduino
```sh
avrdude -p m328p -c usbasp -e -U lock:w:0x3F:m -U efuse:w:0xFD:m -U hfuse:w:0xDE:m -U lfuse:w:0xFF:m
avrdude -p m328p -c usbasp -U flash:w:optiboot_atmega328.hex:i -U lock:w:0x0F:m
```
O `optiboot_atmega328.hex` vem com a IDE do Arduino ou do repositório do optiboot. (A IDE do Arduino faz isso em "Gravar bootloader".)

### 2.6 Como seria um bootloader completo
1. No reset (BOOTRST = 0), roda da seção de boot.
2. Decide: há um programa válido e ninguém pediu atualização? Salta para 0x0000.
3. Senão, recebe páginas pela serial (protocolo STK500 no optiboot), grava cada uma com a rotina acima, confere e responde.
4. Protege a si mesmo com os **lock bits** (BLB1x) para que a aplicação não o sobrescreva.

---

## 3. AT32UC3A

No UC3 não há restrição de "seção de boot": **qualquer código** pode gravar a Flash pelo **FLASHC** (módulo 16), com a mesma sequência de page buffer:
```c
flashc_cmd(FLASHC_CMD_EP, pagina);       /* apaga a página */
flashc_cmd(FLASHC_CMD_CPB, 0);           /* limpa o page buffer */
for (i...) pg[i] = valor;                /* escreve nos endereços da página -> vai para o buffer */
flashc_cmd(FLASHC_CMD_WP, pagina);       /* grava */
```
Páginas de **512 bytes**; a última página da Flash de 512 KB é a 1023 (`0x8007_FE00`). Enquanto a Flash grava, a CPU que executa da Flash fica parada (para gravar sem parar, execute a rotina da RAM).

Proteção: a Flash é dividida em 16 regiões que podem ser travadas pelos **lock bits** (o bootloader DFU trava os seus 8 KB). Gravar numa região travada dá `LOCKE` no FSR.

O bit **BOOTPROT** e os "general purpose fuse bits" (comando WGPB/EGPB do FLASHC) definem a área protegida do bootloader e a condição de entrada nele.

---

## Exercícios
1. Mude o exemplo do ATmega para gravar uma tabela de calibração que sobreviva a uma regravação pelo ISP (dica: lock bits e EESAVE não ajudam; onde ela teria de ficar?).
2. Implemente um "bootloader" mínimo no ATmega: na seção de boot, receba 128 bytes em hexadecimal pela serial e grave a página 0.
3. No UC3, copie a rotina de gravação para a RAM (`__attribute__((section(".data")))`) e mostre que a CPU continua atendendo interrupções durante a gravação.
4. Calcule quantos ciclos de apagar/gravar a sua aplicação faria por dia se gravasse a configuração a cada minuto, e a vida útil resultante.
