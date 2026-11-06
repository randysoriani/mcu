# Curso AVR Bare-Metal no Linux (8 e 32 bits)

**ATmega328P (AVR de 8 bits) e AT32UC3A0512 (AVR32 UC3, 32 bits), sem Arduino, sem avr-libc `<avr/io.h>`, sem ASF, sem IDE.**
Só `avr-gcc` / `avr32-gcc`, `make`, um editor de texto e o datasheet.

É o terceiro curso da série (`../stm32-baremetal`, `../tm4c-baremetal`), com o mesmo estilo: cada módulo é uma pasta com um `README.md` que explica do zero, código para os dois chips e um `Makefile` de 2 linhas. Os módulos foram escolhidos pelos periféricos **típicos da família AVR** (fuses, EEPROM, comparador analógico, autoprogramação...), não copiados dos outros cursos.

---

## Hardware

| | Linha de 8 bits | Linha de 32 bits |
|---|---|---|
| Chip | **ATmega328P** | **AT32UC3A0512** |
| Placa sugerida | **Arduino Uno** (ou Nano) | **EVK1100** (Atmel) |
| Núcleo | AVR de 8 bits, Harvard, 16 MHz | AVR32 UC, 32 bits, até 66 MHz (48 MHz no curso) |
| Flash / RAM | 32 KB / 2 KB + 1 KB de EEPROM | 512 KB / 64 KB |
| Gravação | bootloader do Arduino pela USB (avrdude) ou programador ISP (USBasp) | bootloader USB DFU (dfu-programmer) ou JTAG |
| Serial do console | USART0 → conversor USB da placa → `/dev/ttyACM0` | USART0 → conector DB9 "UART_0" (RS-232) |
| LED / botão | LED "L" no pino 13 (PB5); botão externo no pino 2 | LED0 = PB27 (ativo em baixo); botão do joystick em PA20 |
| Emulador | **QEMU `-M uno`** (usado para testar o curso) | nenhum |

### Sobre a linha AVR32
A linha AVR32 (UC3) é antiga e **não é recomendada para projetos novos** pela Microchip; o compilador oficial (avr32-gcc 4.4.7, de 2015) não está nos repositórios das distribuições e as placas são difíceis de achar. Ela entra no curso porque você pediu as duas linhas e porque é uma ótima comparação: mostra um AVR "de 32 bits" muito diferente do de 8 bits. Os periféricos dela (USART, SPI, TWI, TC, PWM, ADC) são os mesmos blocos dos ARM SAM7 da Atmel. Para um AVR moderno de 8 bits, a família AVR-Dx/ATmega4809 usa as mesmas ideias deste curso.

---

## Estrutura

```
avr-baremetal/
├── Makefile                   # compila todos os módulos para os dois chips
├── common/
│   ├── mk/rules.mk            # regras de build (avr-gcc ou avr32-gcc conforme MCU=)
│   ├── include/               # device.h, system.h (interface comum)
│   ├── lib/printf.c           # kprintf() para os dois
│   ├── m328p/                 # m328p.h (registradores), startup.S, linker.ld, clock, tick, uart, board.h
│   └── uc3a/                  # uc3a.h, startup.S, linker.ld, intc.c, clock, tick, uart, board.h
└── 00-ambiente/ ... 21-depuracao-e-simulacao/
```

Os headers de registradores (`m328p.h`, `uc3a.h`) foram **escritos à mão** a partir dos datasheets. O startup e o linker script também são do curso: não usamos o `crt` da avr-libc.

---

## Módulos

| # | Pasta | Assunto | 328P | UC3A |
|---|---|---|---|---|
| 00 | [00-ambiente](00-ambiente/README.md) | Ferramentas, gravação (avrdude, DFU), serial, QEMU | ✓ | ✓ |
| 01 | [01-arquitetura-memoria](01-arquitetura-memoria/README.md) | Harvard × mapa único, registradores, espaços de memória | ✓ | ✓ |
| 02 | [02-startup](02-startup/README.md) | Vetores, startup em assembly, rotinas de interrupção | ✓ | ✓ |
| 03 | [03-linker-script](03-linker-script/README.md) | Seções, `.data` na Flash, `const` na RAM, PROGMEM, `.eeprom` | ✓ | ✓ |
| 04 | [04-clock-e-fuses](04-clock-e-fuses/README.md) | Fuses, CLKPR; oscilador, PLL e wait states do UC3 | ✓ | ✓ |
| 05 | [05-gpio-blink](05-gpio-blink/README.md) | **Hello world**: DDR/PORT/PIN; GPIO do UC3 | ✓ | ✓ |
| 06 | [06-interrupcoes-e-tempo](06-interrupcoes-e-tempo/README.md) | Interrupções, `millis()`, atomicidade; INTC e COUNT/COMPARE | ✓ | ✓ |
| 07 | [07-usart](07-usart/README.md) | Serial: baud rate, interrupções, buffer circular, shell | ✓ | ✓ |
| 08 | [08-entradas-e-interrupcoes-externas](08-entradas-e-interrupcoes-externas/README.md) | Pull-up, INT0, PCINT, debounce; interrupção de pino no UC3 | ✓ | ✓ |
| 09 | [09-timers](09-timers/README.md) | Timer0/1/2, CTC, prescaler; TC do UC3 | ✓ | ✓ |
| 10 | [10-pwm](10-pwm/README.md) | Fast PWM, servo; bloco PWM do UC3 | ✓ | ✓ |
| 11 | [11-captura-e-contador](11-captura-e-contador/README.md) | Input capture (ICP1), contador externo; TC em captura | ✓ | ✓ |
| 12 | [12-spi](12-spi/README.md) | SPI mestre, loopback | ✓ | ✓ |
| 13 | [13-twi-i2c](13-twi-i2c/README.md) | TWI (I2C) com códigos de status; TWI do UC3 | ✓ | ✓ |
| 14 | [14-adc](14-adc/README.md) | ADC, referências, Vcc e temperatura internos | ✓ | ✓ |
| 15 | [15-comparador-analogico](15-comparador-analogico/README.md) | Comparador analógico | ✓ | — |
| 16 | [16-eeprom-e-memoria-nao-volatil](16-eeprom-e-memoria-nao-volatil/README.md) | EEPROM; user page da Flash no UC3 | ✓ | ✓ |
| 17 | [17-watchdog](17-watchdog/README.md) | Watchdog e causa do reset | ✓ | ✓ |
| 18 | [18-sleep-e-energia](18-sleep-e-energia/README.md) | Modos de sleep, PRR | ✓ | ✓ |
| 19 | [19-dma-pdca](19-dma-pdca/README.md) | DMA (PDCA) | — | ✓ |
| 20 | [20-autoprogramacao-e-bootloader](20-autoprogramacao-e-bootloader/README.md) | SPM e seção de boot; FLASHC | ✓ | ✓ |
| 21 | [21-depuracao-e-simulacao](21-depuracao-e-simulacao/README.md) | QEMU + GDB, debugWIRE, JTAG | ✓ | ✓ |

---

## Uso rápido

```sh
# Arch: ferramentas AVR de 8 bits
sudo pacman -S avr-gcc avr-libc avr-binutils avrdude avr-gdb qemu-system-avr picocom

cd 05-gpio-blink
make MCU=m328p flash            # Arduino Uno pela USB
cd ../07-usart
make MCU=m328p flash && picocom -b 115200 /dev/ttyACM0

# sem placa: no QEMU (use -DQEMU nos módulos que usam a base de tempo)
make clean && make MCU=m328p EXTRA=-DQEMU && make MCU=m328p qemu
```
O avr32-gcc é instalado à mão (módulo 00).

## Documentos de referência

| Documento | ATmega328P | AT32UC3A0512 |
|---|---|---|
| Datasheet | "ATmega48A/PA/88A/PA/168A/PA/328/P" (Microchip DS40002061) | "AT32UC3A Series" (doc32058) |
| Arquitetura | "AVR Instruction Set Manual" (DS40002198) | "AVR32 Architecture Document" (doc32000) e "AVR32UC Technical Reference Manual" (doc32002) |
| Placa | esquemático do Arduino Uno R3 | "EVK1100 User Guide" (doc32005) e esquemático |

## Estado de verificação

- Todos os módulos **compilam sem avisos** para os dois chips (`make` na raiz), com avr-gcc 7.3 e avr32-gcc 4.4.7.
- **ATmega328P: executado no QEMU** (`-M uno`): módulos 01, 03, 07 e 21. Isso cobre startup, `.data`/`.bss`, PROGMEM, `.noinit`, serial com interrupções e base de tempo. A sessão GDB do 21 (breakpoint, watchpoint, breakpoint condicional) também rodou de verdade, e as saídas citadas nos README vêm dessas execuções. O QEMU não emula Timer0/Timer2, TWI, SPI, ADC, EEPROM, comparador, watchdog nem sleep: os módulos 06 e 08–20 só foram compilados. O 06 trava no QEMU, porque usa o Timer0.
- **AT32UC3A: nada foi executado** (não há emulador e não há placa aqui). O código segue o datasheet, mas os pontos marcados "confira" nos README (pinos da EVK1100, grupos de interrupção, polaridade do PWM) devem ser verificados na bancada.
