# Módulo 00 — Ambiente de desenvolvimento no Linux

> **Objetivo deste módulo:** montar no seu Linux tudo o que é preciso para escrever, compilar, gravar e conversar pela serial com as placas, sem a IDE do Arduino e sem o Microchip Studio, e entender o papel de cada ferramenta. No fim você terá gravado o seu primeiro programa.

---

## 0. Para quem nunca programou um microcontrolador

### Como é diferente de programar para o PC

| No PC | No microcontrolador |
|---|---|
| Você compila e roda o programa **no mesmo computador** | Você compila no PC e o programa roda **em outro processador** (o da placa): **compilação cruzada** |
| O sistema operacional carrega o programa | Um **gravador** escreve o programa na memória Flash do chip |
| `printf` aparece no terminal | Não há tela: o programa "fala" pela **porta serial**, e um programa de terminal no PC mostra o texto |
| Um erro mostra uma mensagem e o programa fecha | Um erro geralmente **trava** o chip em silêncio |
| O programa termina | O programa **nunca termina**: roda num laço infinito enquanto houver energia |

### E o Arduino?
A placa Arduino Uno é só uma placa com um **ATmega328P**. A IDE do Arduino esconde tudo: a configuração do chip, a função `main()`, os registradores. Neste curso usamos a **mesma placa**, mas programamos o chip diretamente, como se fosse um ATmega328P avulso. Tudo o que você aprender aqui vale para o chip numa placa sua.

### As placas

**Arduino Uno (ATmega328P):**
```
 USB ──► conversor USB-serial (ATmega16U2) ──serial──► ATmega328P (o chip ALVO)
                                            ──reset──►   LED "L" no pino 13
```
O conversor faz duas coisas: aparece no PC como a porta serial `/dev/ttyACM0` e, quando o PC abre a porta, **reseta o ATmega328P**. Logo após o reset roda o **bootloader** (um pequeno programa gravado de fábrica no fim da Flash), que durante meio segundo aceita um programa novo pela serial e o grava. É assim que o Arduino é gravado sem nenhum programador.

**EVK1100 (AT32UC3A0512):** placa de avaliação da Atmel com o chip AVR32, LEDs, joystick, display e dois conectores seriais DB9. Vem com um **bootloader USB (DFU)** de fábrica nos primeiros 8 KB da Flash.

### As ferramentas, em uma frase cada

| Ferramenta | Para que serve |
|---|---|
| editor de texto | escrever o código |
| **avr-gcc** | compilador C para AVR de 8 bits |
| **avr32-gcc** | compilador C para AVR32 |
| **make** | automatiza a compilação |
| **avrdude** | grava o ATmega (pelo bootloader do Arduino ou por um programador ISP); lê e grava fuses e EEPROM |
| **dfu-programmer** | grava o UC3 pelo bootloader USB |
| **picocom** | terminal serial |
| **qemu-system-avr** | emula um Arduino Uno no PC (sem placa) |
| **avr-gdb** | depurador (módulo 21) |

### Primeiro programa em 10 minutos (Arduino Uno)

1. Instale as ferramentas (seção 1).
2. Ligue o Uno no USB e confira:
   ```sh
   ls /dev/ttyACM*            # deve aparecer /dev/ttyACM0
   ```
3. Compile e grave o pisca-LED:
   ```sh
   cd avr-baremetal/05-gpio-blink
   make MCU=m328p             # compila; mostra o tamanho do programa
   make MCU=m328p flash       # grava pelo bootloader
   ```
   O avrdude mostra uma barra de progresso e termina com `avrdude done. Thank you.`; o LED "L" pisca.
4. Um programa que escreve na serial:
   ```sh
   cd ../01-arquitetura-memoria
   make MCU=m328p flash
   picocom -b 115200 /dev/ttyACM0      # sair: Ctrl-A e depois Ctrl-X
   ```

### Sem placa: o QEMU
```sh
cd 07-usart
make clean && make MCU=m328p EXTRA=-DQEMU
make MCU=m328p qemu                     # Ctrl-A e depois X para sair
```
O `-DQEMU` troca o timer da base de tempo (o QEMU só emula os timers de 16 bits).

---

## 1. Instalação

### Linha de 8 bits

Arch Linux:
```sh
sudo pacman -S avr-gcc avr-libc avr-binutils avrdude avr-gdb qemu-system-avr picocom make
```
Debian/Ubuntu:
```sh
sudo apt install gcc-avr avr-libc binutils-avr avrdude gdb-avr qemu-system-misc picocom make
```
(O curso usa da avr-libc apenas o `<stdint.h>`; os registradores vêm dos nossos headers.)

### Linha de 32 bits (AVR32)

O compilador não está nos repositórios. Use o pacote arquivado da Microchip:
```sh
mkdir -p ~/opt && cd ~/opt
wget https://ww1.microchip.com/downloads/archive/avr32-gnu-toolchain-3.4.3.820-linux.any.x86_64.tar.gz
tar xzf avr32-gnu-toolchain-3.4.3.820-linux.any.x86_64.tar.gz
echo 'export PATH=$HOME/opt/avr32-gnu-toolchain-linux_x86_64/bin:$PATH' >> ~/.bashrc
avr32-gcc --version        # avr32-gcc (AVR_32_bit_GNU_Toolchain_3.4.3_820) 4.4.7
```
Para gravar: `dfu-programmer` (Arch: `sudo pacman -S dfu-programmer`; Debian: `sudo apt install dfu-programmer`).

### Permissões
- Serial: grupo `uucp` (Arch) ou `dialout` (Debian): `sudo usermod -aG uucp $USER` e faça logout/login.
- USBasp e DFU sem sudo: regras udev (o pacote do avrdude costuma instalar a do USBasp). Para o DFU do UC3 (VID 03eb):
  ```
  SUBSYSTEM=="usb", ATTRS{idVendor}=="03eb", MODE="0666"
  ```
  em `/etc/udev/rules.d/99-atmel.rules`, e `sudo udevadm control --reload`.

---

## 2. O build do curso

Cada módulo tem um `Makefile` de 2 linhas que inclui [`common/mk/rules.mk`](../common/mk/rules.mk).

| Variável | Padrão | Uso |
|---|---|---|
| `MCU` | `m328p` | `m328p` (ATmega328P) ou `uc3a` (AT32UC3A0512) |
| `PROG` | `arduino` | programador do avrdude: `arduino` (bootloader), `usbasp`, `avrisp2`... |
| `PORT` | `/dev/ttyACM0` | porta do bootloader do Arduino |
| `LIB` | `clock tick uart printf` | partes da biblioteca do curso |
| `OPT` | `-Os` (8 bits) / `-O2` (32 bits) | otimização |
| `EXTRA` | — | ex.: `make clean && make EXTRA=-DDEMO=2` |

| Alvo | O que faz |
|---|---|
| `make MCU=m328p` | compila → `build/m328p/<módulo>.elf`, `.hex` (Flash), `.eep` (EEPROM, se houver), `.map` |
| `make MCU=m328p flash` | grava com o avrdude |
| `make MCU=m328p flash PROG=usbasp` | grava por um programador ISP |
| `make MCU=uc3a flash` | grava pelo bootloader DFU do UC3 |
| `make MCU=m328p qemu` / `qemu-gdb` | roda no QEMU / espera o GDB |
| `make size`, `make disasm`, `make clean` | tamanho, listagem em assembly, limpeza |

Na raiz, `make` compila todos os módulos para os dois chips.

### Opções do compilador que importam
| Opção | Por quê |
|---|---|
| `-mmcu=atmega328p` | escolhe o conjunto de instruções (família "avr5") e o tamanho das memórias |
| `-mpart=uc3a0512` | idem para o AVR32 |
| `-Os` | otimiza para tamanho: no ATmega há só 32 KB de Flash |
| `-nostartfiles` | não usa o startup da avr-libc/do toolchain: **nós** escrevemos o nosso (módulo 02) |
| `-ffunction-sections -fdata-sections` + `--gc-sections` | o linker descarta o que não é usado |
| `-DF_CPU=16000000UL` | a frequência do clock, usada nos cálculos de baud rate e atrasos |

---

## 3. Gravando o ATmega328P

### Pelo bootloader do Arduino (sem programador)
```sh
avrdude -p m328p -c arduino -P /dev/ttyACM0 -b 115200 -U flash:w:build/m328p/x.hex:i
```
- `-p m328p`: o chip; `-c arduino`: o "programador" é o bootloader do Arduino; `-b 115200`: velocidade do bootloader do Uno (o Nano antigo usa 57600).
- `-U flash:w:arquivo:i`: na memória `flash`, **w**rite, arquivo no formato Intel HEX (`i`).

Limitação: pelo bootloader **não dá** para gravar fuses nem a seção de boot.

### Por um programador ISP (USBasp, AVRISP mkII, Arduino as ISP)
O ISP usa os pinos SPI do chip (MOSI, MISO, SCK, RESET), no conector de 6 pinos "ICSP" ao lado do ATmega no Uno.
```sh
avrdude -p m328p -c usbasp -U flash:w:build/m328p/x.hex:i
avrdude -p m328p -c usbasp -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h    # lê os fuses
```
Gravar por ISP **apaga o bootloader do Arduino** (o chip é apagado inteiro antes). Para devolvê-lo, veja o módulo 20.

### Modo terminal do avrdude
```sh
avrdude -p m328p -c usbasp -t
avrdude> dump eeprom 0 32
avrdude> read lfuse
avrdude> quit
```

---

## 4. Gravando o AT32UC3A0512 (EVK1100)

1. Entre no bootloader: segure o botão indicado no manual da EVK1100 para o modo ISP enquanto aperta o RESET (na configuração de fábrica, um dos botões de usuário/joystick).
2. O PC passa a ver um dispositivo USB "AT32UC3A0512 DFU" (`lsusb`).
3. Grave:
   ```sh
   dfu-programmer at32uc3a0512 erase
   dfu-programmer at32uc3a0512 flash --suppress-bootloader-mem build/uc3a/x.hex
   dfu-programmer at32uc3a0512 start
   ```
   (`make MCU=uc3a flash` faz os três.) O `--suppress-bootloader-mem` evita sobrescrever o bootloader nos primeiros 8 KB; por isso o programa do curso começa em `0x8000_2000` (módulos 02 e 03).

Com um JTAGICE mkII/AVR ONE! e a ferramenta `avr32program` (do antigo "AVR32 Studio") também é possível gravar e depurar por JTAG.

---

## 5. Inspecionando o binário

```sh
avr-size build/m328p/x.elf              # text / data / bss
avr-objdump -d -S build/m328p/x.elf     # assembly + C
avr-nm -n build/m328p/x.elf             # símbolos e endereços
avr-objdump -h build/m328p/x.elf        # seções
```
No AVR de 8 bits, o `data` do `avr-size` conta duas vezes: ocupa Flash (o valor inicial) **e** RAM, e a RAM do 328P tem só 2 KB. Fique de olho.

---

## Exercícios
1. Rode `make -n` num módulo e identifique as opções do compilador.
2. Grave o módulo 05 pelo bootloader e depois leia os fuses com o avrdude (precisa de um ISP).
3. Rode o módulo 07 no QEMU e use o shell.
