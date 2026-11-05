# Módulo 00 — Ambiente de desenvolvimento no Linux

> **Objetivo deste módulo:** montar no seu Linux tudo o que é preciso para **escrever, compilar, gravar, depurar e conversar pela serial** com a placa, sem IDE, e entender o papel de cada ferramenta. No fim você terá gravado e visto funcionar o seu primeiro programa.

---

## 0. Para quem nunca programou um microcontrolador

### Como é diferente de programar para o PC

| No PC | No microcontrolador |
|---|---|
| Você compila e roda o programa **no mesmo computador** | Você compila no PC e o programa roda **em outro processador** (o da placa). Isso se chama **compilação cruzada** (*cross-compiling*) |
| O sistema operacional carrega o programa na memória | Um **gravador** escreve o programa na memória Flash do chip |
| `printf` aparece no terminal | Não há tela: o programa "fala" pela **porta serial**, e o PC mostra num programa de terminal |
| Um erro mostra uma mensagem e o programa fecha | Um erro geralmente **trava** o chip em silêncio; para ver o que houve, usa-se um **depurador** |
| O programa termina | O programa **nunca termina**: roda num laço infinito enquanto houver energia |

### A placa Nucleo

A Nucleo tem **dois** microcontroladores:

```
 ┌──────────────────────── placa Nucleo-64 ───────────────────────────┐
 │  ┌───────────────┐               ┌──────────────────────────────┐  │
 │  │   ST-LINK     │── fios SWD ──►│  STM32F072 ou STM32F446      │  │
 │  │ (gravador e   │               │  (o chip ALVO: onde o seu    │  │
 │  │  depurador)   │◄── serial ────│   programa roda)             │  │
 │  └───────┬───────┘               └──────────────────────────────┘  │
 │          │ USB                      LED verde (PA5), botão azul    │
 └──────────┼─────────────────────────────────────────────────────────┘
            ▼
          seu PC
```

- A parte de cima da placa é o **ST-LINK**: um gravador/depurador. Pelo único cabo USB, ele aparece no PC como três coisas ao mesmo tempo: um **gravador/depurador**, uma **porta serial** (`/dev/ttyACM0`) e um pequeno disco USB (não usado no curso).
- A parte de baixo tem o **chip alvo**, onde o seu programa roda.
- **SWD** (*Serial Wire Debug*) são dois fios pelos quais o ST-LINK controla o chip alvo: grava a Flash, para e continua a execução, lê e escreve a memória.

Ao ligar a placa no USB: o LED **LD1** (ST-LINK) fica aceso ou piscando e o LED **LD3** (alimentação, vermelho) acende. O LED **LD2** (verde) é o que os programas do curso controlam.

### As ferramentas, em uma frase cada

| Ferramenta | Para que serve |
|---|---|
| editor de texto | escrever o código (qualquer um: VS Code, Vim, Kate, Gedit...) |
| **arm-none-eabi-gcc** | o compilador: transforma C em código de máquina ARM |
| **make** | automatiza a compilação: você digita `make` e ele chama o compilador com as opções certas |
| **OpenOCD** | o programa do PC que fala com o ST-LINK: grava o chip e permite depurar |
| **GDB** | o depurador: para o programa, mostra variáveis, executa linha a linha |
| **picocom** | terminal serial: mostra o que o programa escreve na serial |

### Primeiro programa em 10 minutos

1. Instale as ferramentas (seção 2).
2. Ligue a Nucleo-F446RE no USB e confira que o Linux a viu:
   ```sh
   lsusb | grep -i st-link        # deve aparecer "STMicroelectronics ST-LINK/V2.1"
   ls /dev/ttyACM*                # deve aparecer /dev/ttyACM0
   ```
3. Compile e grave o programa que pisca o LED:
   ```sh
   cd stm32-baremetal/05-gpio-blink
   make MCU=f4                    # compila; termina mostrando o tamanho do programa
   make MCU=f4 flash              # grava; termina com "** Verified OK **"
   ```
   O LED verde deve começar a piscar. (Com a Nucleo-F072RB, troque `f4` por `f0`.)
4. Agora um programa que escreve na serial:
   ```sh
   cd ../01-arquitetura-memoria
   make MCU=f4 flash
   picocom -b 115200 /dev/ttyACM0     # aperte o botão RESET preto da placa
   ```
   Aparecem informações sobre o chip. Para sair do picocom: **Ctrl-A** e depois **Ctrl-X**.

Se algo der errado, a seção "Permissões" e o módulo 23 (OpenOCD) cobrem os problemas mais comuns.

---

## 1. O que é cada peça (em detalhe)

```
  main.c ──► arm-none-eabi-gcc ──► main.o ─┐
startup.c ─► arm-none-eabi-gcc ──► startup.o ─┼─► arm-none-eabi-ld (via gcc) ──► programa.elf
                                   libc/libgcc ─┘         ▲                          │
                                                     linker.ld                       │
                                                                 objcopy ◄───────────┤
                                                                    │                │
                                                         programa.bin/.hex       GDB ◄──► OpenOCD ◄─USB─► ST-LINK ◄─SWD─► MCU
                                                                    │
                                                              st-flash / OpenOCD
```

| Ferramenta | Função |
|---|---|
| `arm-none-eabi-gcc` | Compilador C cruzado: roda no PC x86-64, gera código **Thumb** para Cortex-M. "none" = sem sistema operacional; "eabi" = Embedded ABI (convenção de chamada, layout de structs). |
| `arm-none-eabi-as` | Montador (chamado pelo gcc para `.s`/`.S`). |
| `arm-none-eabi-ld` | Linker: junta os `.o`, resolve símbolos e posiciona tudo nos endereços definidos pelo **linker script**. |
| `newlib` / `newlib-nano` | Biblioteca C para embarcados (`memcpy`, `strcmp`, ...). O curso quase não usa; o `printf` é próprio (`kprintf`). |
| `libgcc` | Rotinas de suporte do compilador (ex.: divisão inteira no Cortex-M0, que não tem instrução de divisão). |
| `objcopy` | Converte o ELF em imagem crua (`.bin`) ou Intel HEX (`.hex`). |
| `objdump`, `nm`, `size`, `readelf` | Inspeção: desassembly, tabela de símbolos, tamanho das seções. |
| **OpenOCD** | Servidor que fala com o ST-LINK via USB e com o chip via **SWD**. Grava a Flash e expõe um servidor GDB na porta 3333. |
| `st-flash` (stlink-tools) | Alternativa simples ao OpenOCD só para gravar/apagar. |
| `arm-none-eabi-gdb` | Depurador: breakpoints, passo a passo, ver/alterar registradores e memória. |
| `picocom`/`minicom`/`screen` | Terminal serial para a USART2 que o ST-LINK expõe como `/dev/ttyACM0`. |

---

## 2. Instalação

### Arch Linux / Manjaro
```sh
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib arm-none-eabi-binutils \
               arm-none-eabi-gdb openocd stlink picocom make
```

### Debian / Ubuntu
```sh
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi binutils-arm-none-eabi \
                 gdb-multiarch openocd stlink-tools picocom make
```
No Debian/Ubuntu o GDB chama-se `gdb-multiarch`: use `make GDB=gdb-multiarch debug`.

### Fedora
```sh
sudo dnf install arm-none-eabi-gcc-cs arm-none-eabi-newlib arm-none-eabi-binutils-cs \
                 openocd stlink picocom make
```

### Toolchain oficial da ARM (qualquer distro, sem root)
```sh
cd ~/opt
wget https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz
tar xf arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz
echo 'export PATH=$HOME/opt/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi/bin:$PATH' >> ~/.bashrc
```
Este pacote já inclui newlib, newlib-nano e o GDB.

### Conferindo
```sh
arm-none-eabi-gcc --version
openocd --version
st-info --probe          # com a placa ligada: mostra o chip detectado
```

### Permissões (USB e serial)
- **ST-LINK sem sudo**: os pacotes `stlink`/`openocd` instalam regras udev em `/usr/lib/udev/rules.d/`. Se `openocd` reclamar de `LIBUSB_ERROR_ACCESS`, recarregue as regras: `sudo udevadm control --reload && sudo udevadm trigger` e reconecte a placa.
- **Serial**: `/dev/ttyACM0` pertence ao grupo `uucp` (Arch) ou `dialout` (Debian/Fedora).
  ```sh
  sudo usermod -aG uucp $USER      # Arch
  sudo usermod -aG dialout $USER   # Debian/Ubuntu/Fedora
  ```
  Faça logout/login depois.

---

## 3. Make em 10 minutos

O `make` lê um `Makefile` com **regras**:

```make
alvo: dependências
	receita (linhas começando com TAB)
```

Ele só refaz um alvo se alguma dependência for **mais nova** que ele (compara datas de modificação). É isso que torna a compilação incremental.

Veja e rode [`exemplo-makefile/Makefile`](exemplo-makefile/Makefile): um Makefile completo, sem includes, comentado linha a linha.

```sh
cd 00-ambiente/exemplo-makefile
make            # compila
make -n         # mostra os comandos SEM executar (ótimo para aprender)
make clean
```

Conceitos usados no curso:

| Conceito | Exemplo | Significado |
|---|---|---|
| Variável imediata | `CC := arm-none-eabi-gcc` | avaliada uma vez, na leitura |
| Variável recursiva | `X = $(Y)` | avaliada toda vez que é usada |
| Valor padrão | `MCU ?= f4` | só define se ainda não existir (permite `make MCU=f0`) |
| Acrescentar | `CFLAGS += -O2` | |
| Variáveis automáticas | `$@` alvo, `$<` 1ª dependência, `$^` todas | |
| Regra de padrão | `%.o: %.c` | "qualquer .o vem do .c de mesmo nome" |
| Funções | `$(wildcard f0/*.c)`, `$(notdir ...)`, `$(addprefix ...)`, `$(foreach ...)` | manipulação de listas |
| Condicional | `ifeq ($(MCU),f0) ... else ... endif` | |
| Include | `include ../common/mk/rules.mk` | |
| Dependências de headers | `-MMD -MP` + `-include $(DEPS)` | o gcc gera `.d` listando os `.h` que cada `.c` incluiu |
| `vpath` | `vpath %.c dir1 dir2` | onde procurar fontes |
| `.PHONY` | `.PHONY: clean flash` | alvos que não são arquivos |

### O sistema de build do curso

Cada módulo tem um `Makefile` minúsculo:

```make
# Módulo 10 - PWM
include ../common/mk/rules.mk
```

e o trabalho fica em [`common/mk/rules.mk`](../common/mk/rules.mk). Variáveis que o módulo pode definir **antes** do include:

| Variável | Padrão | Uso |
|---|---|---|
| `MCU` | `f4` | `f0` (STM32F072) ou `f4` (STM32F446) |
| `SRCS` | `$(MCU)/*.c $(MCU)/*.s src/*.c` | fontes do módulo |
| `STARTUP` | `common/$(MCU)/startup.c` | o módulo 02 troca pelo startup em assembly |
| `LDSCRIPT` | `common/$(MCU)/linker.ld` | |
| `LIB` | `clock systick uart printf` | partes da "biblioteca" do curso a linkar; `LIB :=` = nada |
| `OPT` | `-O2` | use `make OPT=-O0` para depurar passo a passo com mais fidelidade |
| `EXTRA` | vazio | flags extras, ex.: `make EXTRA=-DDEMO=2` (faça `make clean` antes) |

Alvos:

| Comando | O que faz |
|---|---|
| `make MCU=f0` | compila → `build/f0/<modulo>.elf/.bin/.hex/.map` |
| `make MCU=f0 flash` | grava com OpenOCD (`program ... verify reset exit`) |
| `make MCU=f0 stflash` | grava com `st-flash` |
| `make MCU=f0 erase` | apaga a Flash inteira |
| `make MCU=f0 size` | tamanho de cada seção |
| `make MCU=f0 disasm` | gera `build/f0/<modulo>.lst` com assembly intercalado com C |
| `make MCU=f0 openocd` | só sobe o servidor OpenOCD |
| `make MCU=f0 gdb` | conecta o GDB a um OpenOCD já rodando |
| `make MCU=f0 debug` | sobe OpenOCD em background + GDB parado em `main` |
| `make clean` | apaga `build/` |

Na raiz do curso, `make` compila **todos** os módulos para os dois MCUs.

---

## 4. Flags de compilação explicadas

```
-mcpu=cortex-m0 -mthumb                     # F0: ARMv6-M, só Thumb
-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard   # F4: FPU de precisão simples
```

| Flag | Por quê |
|---|---|
| `-mthumb` | Cortex-M **só** executa Thumb/Thumb-2. Sem isso o gcc gera código ARM (32 bits) e o chip dá HardFault. |
| `-mfloat-abi=hard` | `float` em registradores da FPU (s0..s31). `softfp` usa a FPU mas passa argumentos em r0..r3; `soft` emula tudo em software. **Todos os objetos linkados precisam usar a mesma ABI.** |
| `-O2` | otimiza. Em embarcados o otimizador remove acessos "inúteis" — por isso registradores precisam ser `volatile`. |
| `-g3` | informação de depuração (inclusive macros) no ELF. **Não** vai para a Flash. |
| `-ffunction-sections -fdata-sections` | cada função/variável em sua própria seção (`.text.main`, `.bss.contador`)... |
| `-Wl,--gc-sections` | ...para o linker descartar as que ninguém usa. |
| `-nostartfiles` | não linka o `crt0.o` da newlib: **nós** escrevemos o startup. |
| `--specs=nano.specs` | newlib-nano (versão enxuta da libc). |
| `--specs=nosys.specs` | stubs vazios de syscalls (`_write`, `_sbrk`...) caso algo da libc as peça. |
| `-Wl,-Map=x.map` | gera o map file (módulo 03). |
| `-Wl,--print-memory-usage` | imprime o uso de FLASH/RAM ao final do link. |
| `-MMD -MP` | gera arquivos de dependência `.d`. |

---

## 5. Inspecionando o binário

```sh
cd 05-gpio-blink && make MCU=f4
E=build/f4/05-gpio-blink.elf

arm-none-eabi-size $E               # text / data / bss
arm-none-eabi-size -A -x $E         # cada seção, com endereço
arm-none-eabi-nm -n -S $E           # símbolos ordenados por endereço, com tamanho
arm-none-eabi-objdump -h $E         # cabeçalhos de seção: VMA, LMA, flags
arm-none-eabi-objdump -d -S $E      # desassembly + fonte C
arm-none-eabi-objdump -s -j .isr_vector $E   # hexdump da tabela de vetores
arm-none-eabi-readelf -l $E         # program headers (o que vai para a Flash)
xxd build/f4/05-gpio-blink.bin | head        # o .bin começa com SP inicial e Reset_Handler
```

Leitura do `size`:
- `text` = código + constantes + tabela de vetores (+ imagem inicial de `.data`) → **Flash**
- `data` = variáveis inicializadas → ocupa **Flash** (valor inicial) **e RAM**
- `bss` = variáveis zeradas + pilha reservada → **RAM**

---

## 6. Gravando

### OpenOCD
```sh
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program build/f4/x.elf verify reset exit"
```
- `interface/stlink.cfg` — o adaptador (ST-LINK V2/V2-1/V3)
- `target/stm32f0x.cfg` ou `target/stm32f4x.cfg` — o chip
- O ELF já contém os endereços; para `.bin` é preciso informar: `program x.bin 0x08000000 verify reset exit`

### st-flash
```sh
st-flash --reset write build/f0/x.bin 0x08000000
st-flash erase
st-info --probe
```

### Chip "travado"?
Se um programa reconfigurar os pinos PA13/PA14 (SWD), entrar em low-power profundo ou desligar o clock do debug, o ST-LINK não conecta. Solução: conectar **segurando o reset**:
```sh
openocd -f interface/stlink.cfg -c "reset_config srst_only connect_assert_srst" \
        -f target/stm32f4x.cfg -c "init; reset halt; flash erase_sector 0 0 last; exit"
# ou
st-flash --connect-under-reset erase
```

---

## 7. Depurando com GDB

Terminal 1:
```sh
make MCU=f4 openocd
```
Terminal 2:
```sh
make MCU=f4 gdb
```
(ou `make MCU=f4 debug` faz os dois.)

Comandos essenciais:

| Comando | |
|---|---|
| `load` | grava o ELF |
| `monitor reset halt` | reseta e para na primeira instrução |
| `break main` / `b arquivo.c:42` | breakpoint (o Cortex-M tem 4 (M0) ou 6 (M4) breakpoints de hardware) |
| `continue` (`c`), `next` (`n`), `step` (`s`), `finish` | execução |
| `stepi` / `nexti` | uma instrução assembly |
| `info registers` | r0-r12, sp, lr, pc, xPSR |
| `p variavel`, `p/x variavel` | imprime (`/x` hexadecimal, `/t` binário) |
| `p/x *GPIOA` | **struct inteira do periférico** — funciona porque os headers do curso definem tipos |
| `x/8xw 0x40020000` | examina 8 words a partir de um endereço |
| `set var x = 5` | altera variável |
| `set *(uint32_t*)0x40020014 = 0x20` | escreve num registrador (acende o LED do F4) |
| `watch contador` | para quando a variável muda (watchpoint de hardware) |
| `bt` | pilha de chamadas |
| `disassemble` | assembly da função atual |
| `layout src` / `layout asm` / `layout regs` | modo TUI |
| `monitor reg` | registradores via OpenOCD |

Descobrindo **qual interrupção** caiu no `Default_Handler`:
```
(gdb) p/x $xpsr & 0x1ff       # número da exceção ativa (IRQn = valor - 16)
```

---

## 8. Terminal serial

```sh
picocom -b 115200 /dev/ttyACM0        # sair: Ctrl-A Ctrl-X
screen /dev/ttyACM0 115200            # sair: Ctrl-A k
minicom -D /dev/ttyACM0 -b 115200
```
Formato usado no curso: **115200 bps, 8 bits, sem paridade, 1 stop bit (8N1)**.

---

## 9. Testando sem placa: QEMU (opcional, só F4)

O QEMU emula a placa `netduinoplus2` (STM32F405, Cortex-M4). Ela **não** emula RCC, DMA, I2C, ADC real, etc., mas o núcleo, SysTick, NVIC e as USARTs funcionam — suficiente para testar startup, linker, printf, interrupções de UART:

```sh
cd 07-uart
make clean && make MCU=f4 EXTRA=-DQEMU     # -DQEMU pula a configuração do RCC
qemu-system-arm -M netduinoplus2 -nographic -serial null -serial stdio \
                -monitor none -kernel build/f4/07-uart.elf
# Ctrl-A X para sair. A USART2 é a segunda "-serial".
```

Depurando no QEMU: acrescente `-s -S` e conecte com `arm-none-eabi-gdb -ex "target remote :1234" build/f4/07-uart.elf`.

Testado: módulos 02, 03, 06 e 07 rodam no QEMU. O 01 para ao ler a área de calibração de fábrica (não emulada).

---

## Exercícios

1. Rode `make -n` em `05-gpio-blink` e identifique cada comando do gcc. Quais flags vêm do `rules.mk`?
2. Compile o blink com `OPT=-O0` e com `OPT=-Os`; compare o `size`. Olhe a função `delay` no `.lst` (`make disasm`) nos dois casos.
3. Em `05-gpio-blink/f4/main.c`, remova o `volatile` do parâmetro de `delay()` **e** o `__asm volatile ("nop")`, compile com `-O2` e veja no disassembly o que aconteceu com o laço (e com o LED).
4. No GDB, pare numa linha **depois** da configuração do `MODER` (ex.: `b main.c:24`) e acenda/apague o LED escrevendo direto no `GPIOA->ODR` com `set var GPIOA->ODR = 0x20`.
