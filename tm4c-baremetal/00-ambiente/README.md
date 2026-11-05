# Módulo 00 — Ambiente de desenvolvimento no Linux (TM4C)

> **Objetivo deste módulo:** montar no seu Linux tudo o que é preciso para **escrever, compilar, gravar, depurar e conversar pela serial** com a LaunchPad, sem IDE, e entender o papel de cada ferramenta. No fim você terá gravado e visto funcionar o seu primeiro programa.

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

### A LaunchPad

A LaunchPad tem **dois** microcontroladores:

```
 ┌──────────────────────── placa LaunchPad ───────────────────────────┐
 │  ┌───────────────┐               ┌──────────────────────────────┐  │
 │  │    ICDI       │── JTAG/SWD ──►│  TM4C123 ou TM4C1294         │  │
 │  │ (gravador e   │               │  (o chip ALVO: onde o seu    │  │
 │  │  depurador)   │◄── serial ────│   programa roda)             │  │
 │  └───────┬───────┘               └──────────────────────────────┘  │
 │          │ USB "DEBUG"                LEDs, botões                 │
 └──────────┼─────────────────────────────────────────────────────────┘
            ▼
          seu PC
```

- O **ICDI** (*In-Circuit Debug Interface*) é um gravador/depurador feito pela TI com outro chip TM4C. Pelo cabo USB ele aparece no PC como um **gravador/depurador** e uma **porta serial** (`/dev/ttyACM0`), ligada à UART0 do chip alvo.
- O **chip alvo** é onde o seu programa roda.
- **JTAG/SWD** são os fios pelos quais o ICDI controla o chip alvo: grava a Flash, para e continua a execução, lê e escreve a memória.

Ligação: na EK-TM4C123GXL, a chave **POWER SELECT** em **DEBUG** e o cabo no micro-USB marcado "DEBUG". Na EK-TM4C1294XL, o jumper **JP1** na posição **ICDI** e o cabo no micro-USB "DEBUG". O LED verde de alimentação acende.

### As ferramentas, em uma frase cada

| Ferramenta | Para que serve |
|---|---|
| editor de texto | escrever o código (qualquer um) |
| **arm-none-eabi-gcc** | o compilador: transforma C em código de máquina ARM |
| **make** | automatiza a compilação: você digita `make` e ele chama o compilador com as opções certas |
| **OpenOCD** | o programa do PC que fala com o ICDI: grava o chip e permite depurar |
| **lm4flash** | alternativa simples ao OpenOCD, só para gravar |
| **GDB** | o depurador: para o programa, mostra variáveis, executa linha a linha |
| **picocom** | terminal serial: mostra o que o programa escreve na serial |

### Primeiro programa em 10 minutos

1. Instale as ferramentas (seção 2).
2. Ligue a LaunchPad e confira que o Linux a viu:
   ```sh
   lsusb | grep -i 1cbe           # deve aparecer "Luminary Micro ... In-Circuit Debug Interface"
   ls /dev/ttyACM*                # deve aparecer /dev/ttyACM0
   ```
3. Compile e grave o programa que pisca o LED:
   ```sh
   cd tm4c-baremetal/05-gpio-blink
   make MCU=123                   # compila; termina mostrando o tamanho do programa
   make MCU=123 flash             # grava com o OpenOCD
   ```
   O LED verde deve começar a piscar. (Com a EK-TM4C1294XL, troque `123` por `129`.)
4. Agora um programa que escreve na serial:
   ```sh
   cd ../01-arquitetura-memoria
   make MCU=123 flash
   picocom -b 115200 /dev/ttyACM0     # aperte o botão RESET da placa
   ```
   Aparecem informações sobre o chip. Para sair do picocom: **Ctrl-A** e depois **Ctrl-X**.

Se algo der errado, a seção "Permissões" e o módulo 23 (OpenOCD) cobrem os problemas mais comuns.

---

Mesma toolchain do curso STM32 — o que muda é o **debugger** (ICDI da TI em vez de ST-LINK) e os arquivos de configuração do OpenOCD.

---

## 1. Ferramentas

| Ferramenta | Função |
|---|---|
| `arm-none-eabi-gcc` | compilador C cruzado para Cortex-M (Thumb-2) |
| `arm-none-eabi-binutils` | `as`, `ld`, `objcopy`, `objdump`, `nm`, `size`, `readelf` |
| `newlib` (nano) | libc enxuta (o curso quase não usa; `kprintf` é próprio) |
| **OpenOCD** | fala com o **ICDI** por USB e com o chip por JTAG/SWD; grava a Flash; servidor GDB na porta 3333 |
| `lm4flash` (lm4tools) | gravador simples via ICDI (alternativa ao OpenOCD) |
| `arm-none-eabi-gdb` | depurador |
| `picocom` / `minicom` / `screen` | terminal para a UART0 → `/dev/ttyACM0` |

```
 .c ──gcc──► .o ──ld (+linker.ld)──► .elf ──objcopy──► .bin/.hex
                                       │
                         GDB ◄──TCP 3333──► OpenOCD ◄──USB──► ICDI ◄──JTAG/SWD──► TM4C
```

O **ICDI** (In-Circuit Debug Interface) é um segundo microcontrolador TM4C na própria LaunchPad, que implementa o debugger e uma porta serial virtual (CDC-ACM) ligada à UART0 do chip-alvo.

---

## 2. Instalação

### Arch Linux
```sh
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib arm-none-eabi-binutils \
               arm-none-eabi-gdb openocd picocom make
yay -S lm4tools           # opcional (AUR), fornece lm4flash
```

### Debian / Ubuntu
```sh
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi binutils-arm-none-eabi \
                 gdb-multiarch openocd lm4flash picocom make
```
(no Debian use `make GDB=gdb-multiarch debug`)

### Toolchain oficial da ARM (qualquer distro)
```sh
wget https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz
tar xf arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz -C ~/opt
export PATH=$HOME/opt/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi/bin:$PATH
```

### Permissões
- **ICDI**: VID:PID `1cbe:00fd`. Se o OpenOCD der `LIBUSB_ERROR_ACCESS`, crie `/etc/udev/rules.d/99-ti-icdi.rules`:
  ```
  SUBSYSTEM=="usb", ATTRS{idVendor}=="1cbe", ATTRS{idProduct}=="00fd", MODE="0666"
  ```
  e rode `sudo udevadm control --reload && sudo udevadm trigger`.
- **Serial**: grupo `uucp` (Arch) ou `dialout` (Debian/Fedora): `sudo usermod -aG uucp $USER` e faça logout/login.

### Ligando as placas
- EK-TM4C123GXL: chave **POWER SELECT** em **DEBUG**, cabo no conector micro-USB "DEBUG" (lado da chave).
- EK-TM4C1294XL: jumper **JP1** na posição **ICDI**, cabo no micro-USB "DEBUG" (U22).

---

## 3. Make

Veja e rode [`exemplo-makefile/Makefile`](exemplo-makefile/Makefile) — um Makefile completo, sem includes, comentado linha a linha, que compila um blink para a TM4C123.

```sh
cd 00-ambiente/exemplo-makefile
make           # compila
make -n        # mostra os comandos sem executar
make flash
```

Conceitos (variáveis `:=`/`?=`/`+=`, `$@ $< $^`, regras de padrão `%.o: %.c`, `-MMD -MP`, `.PHONY`, `vpath`, `ifeq`) estão explicados no README do módulo 00 do curso STM32 e nos comentários do exemplo.

### O build do curso

Cada módulo: `include ../common/mk/rules.mk`. Variáveis:

| Variável | Padrão | Uso |
|---|---|---|
| `MCU` | `123` | `123` (TM4C123GH6PM) ou `129` (TM4C1294NCPDT) |
| `SRCS` | `$(MCU)/*.c $(MCU)/*.s src/*.c` | fontes |
| `STARTUP` | `common/$(MCU)/startup.c` | o módulo 02 usa o `.s` |
| `LDSCRIPT` | `common/$(MCU)/linker.ld` | |
| `LIB` | `clock systick uart printf` | partes da biblioteca do curso (`LIB :=` = nada) |
| `OPT` | `-O2` | |
| `EXTRA` | — | ex.: `make clean && make EXTRA=-DDEMO=2` |

| Alvo | |
|---|---|
| `make MCU=129` | compila → `build/129/<módulo>.elf/.bin/.hex/.map` |
| `make MCU=129 flash` | grava com OpenOCD (`board/ek-tm4c1294xl.cfg`) |
| `make MCU=123 lmflash` | grava com `lm4flash` |
| `size`, `disasm`, `openocd`, `gdb`, `debug`, `clean` | como no curso STM32 |

### Flags
```
-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
```
Os dois chips usam exatamente as mesmas flags de CPU: só mudam `-DTM4C123`/`-DTM4C129`, o linker script e o startup.

---

## 4. Gravando

```sh
# OpenOCD (as configs de placa já escolhem interface e alvo)
openocd -f board/ek-tm4c123gxl.cfg -c "program build/123/x.elf verify reset exit"
openocd -f board/ek-tm4c1294xl.cfg -c "program build/129/x.elf verify reset exit"

# lm4flash (só .bin; endereço 0 implícito)
lm4flash build/123/x.bin
```

### Placa "travada"
Programas que desligam o JTAG (reconfigurando PC0–PC3 no 123), entram em deep-sleep/hibernação imediatamente ou mexem no clock de forma errada podem impedir a conexão. Opções:
1. Segurar o botão RESET, iniciar o OpenOCD/lm4flash e soltar.
2. **Unlock / mass erase** pela sequência de debug "unlock" do TM4C: `openocd -f board/ek-tm4c123gxl.cfg -c "init; reset halt; stellaris recover; exit"` (apaga tudo e restaura os padrões) — ou o utilitário LM Flash Programmer da TI (Windows) → "Debug Port Unlock".

> **Nunca reconfigure PC0–PC3** no TM4C123 (são TCK/TMS/TDI/TDO do JTAG): o datasheet as trata como pinos protegidos justamente por isso.

---

## 5. Depurando

```sh
make MCU=123 openocd          # terminal 1
make MCU=123 gdb              # terminal 2
# ou: make MCU=123 debug
```
Comandos úteis: `load`, `monitor reset halt`, `b main`, `c`, `n`, `s`, `si`, `info reg`, `p/x *GPIOF`, `x/8xw 0x400FE600`, `p/x $xpsr & 0x1ff` (exceção ativa; IRQ = valor − 16).

---

## 6. Serial
```sh
picocom -b 115200 /dev/ttyACM0      # sair: Ctrl-A Ctrl-X
```
Com as duas LaunchPads ligadas, a segunda aparece como `/dev/ttyACM1`.

---

## 7. Emulação

O QEMU **não** emula nenhum TM4C (só o antigo LM3S6965/LM3S811, Cortex-M3, com mapa de sistema diferente). Para este curso, use a placa.

---

## Exercícios
1. Rode `make -n` em `05-gpio-blink` e identifique as flags vindas do `rules.mk`.
2. Compare `arm-none-eabi-size` do blink para `MCU=123` e `MCU=129`. Por que a tabela de vetores tem tamanhos diferentes?
3. No GDB, pare após o `DEN` do blink e acenda o LED com `set var GPIOF->DATA = 0x08` (123) ou `set var GPION->DATA = 0x02` (129).
