# Curso TM4C (Tiva C) Bare-Metal no Linux

**TM4C123 e TM4C129 (Cortex-M4F) sem TivaWare, sem driverlib, sem CMSIS, sem Code Composer.**
Só `arm-none-eabi-gcc`, `make`, um editor de texto e o datasheet.

É o "irmão" do curso `../stm32-baremetal`: **os mesmos 23 módulos, na mesma ordem**, com o mesmo sistema de build. Quem fizer os dois vai perceber o que é ARM (igual em qualquer fabricante) e o que é específico de cada fabricante (periféricos).

Cada módulo tem:
- `README.md` — teoria, tabelas de registradores, diferenças 123 × 129, armadilhas, exercícios;
- código para os dois chips (`src/` compartilhado e/ou `123/`, `129/`);
- um `Makefile` de 2 linhas que inclui `common/mk/rules.mk`.

---

## Hardware de referência

| | TM4C123 | TM4C129 |
|---|---|---|
| Placa | **EK-TM4C123GXL** (LaunchPad) | **EK-TM4C1294XL** (Connected LaunchPad) |
| Chip | TM4C123GH6PM | TM4C1294NCPDT |
| Núcleo | Cortex-M4F, **80 MHz** | Cortex-M4F, **120 MHz** |
| Flash / SRAM / EEPROM | 256 KB / 32 KB / 2 KB | 1 MB / 256 KB / 6 KB |
| Cristais | 16 MHz + 32,768 kHz | 25 MHz + 32,768 kHz |
| Debugger | ICDI (Stellaris In-Circuit Debug Interface) na placa | idem |
| LEDs | RGB: PF1 vermelho, **PF3 verde**, PF2 azul | **D1 = PN1**, D2 = PN0, D3 = PF4, D4 = PF0 |
| Botões | **SW1 = PF4**, SW2 = PF0 (pino protegido) | **USR_SW1 = PJ0**, USR_SW2 = PJ1 |
| Console | UART0 (PA0/PA1) → ICDI → `/dev/ttyACM0` | idem |

Os dois chips são Cortex-M4F: o núcleo, a FPU, o SysTick, o NVIC e a forma de compilar são idênticos. Mudam o mapa das portas GPIO (APB × AHB), o sistema de clock (RCC/RCC2 × RSCLKCFG/PLLFREQ/MEMTIM0), o número das IRQs e alguns periféricos.

---

## Diferenças importantes em relação ao curso STM32

| Assunto | STM32 | TM4C |
|---|---|---|
| Flash | `0x0800_0000` (alias em 0x0) | **`0x0000_0000`** direto |
| Clock dos periféricos | `RCC->xxxENR` | `SYSCTL_RCGCxxx` + esperar `SYSCTL_PRxxx` |
| GPIO | `MODER`, `BSRR`, `AFR` | `DIR`, **`DEN`** (obrigatório!), `AFSEL` + `PCTL`, **DATA mascarado por endereço** |
| Interrupção de pino | EXTI + SYSCFG | **cada porta tem a sua IRQ** (`GPIOIS/IBE/IEV/IM/ICR`) |
| UART | USART própria da ST | **ARM PL011** com FIFOs de 16 bytes |
| Timers | TIMx (PWM, captura, encoder num só bloco) | **GPTM** (timers) + **módulo PWM** separado + **QEI** separado |
| SPI | SPI | **SSI** (com loopback interno) |
| I2C | IP v1/v2 da ST | mestre "por comandos" (`MCS`) |
| ADC | sequência de canais | **sequenciadores de amostras** (SS0–SS3) |
| DMA | DMA canais/streams | **uDMA (ARM PL230)** com tabela de controle na RAM |
| CAN | bxCAN (mailboxes) | **Bosch C_CAN** (32 message objects via IF1/IF2) |
| RTC | RTC com calendário BCD | **contador de segundos** no módulo de Hibernação |
| Watchdog | IWDG + WWDG | WDT0/WDT1 de **dois estágios** (interrupção, depois reset) |
| **DAC** | sim | **não existe** → módulo 16 usa PWM + filtro RC |
| **I2S** | sim | **não existe** → módulo 17 gera I2S por software (uDMA → GPIO) |

---

## Estrutura

```
tm4c-baremetal/
├── Makefile                 # compila todos os módulos (123 e 129)
├── common/
│   ├── mk/rules.mk          # regras de build
│   ├── include/             # tm4c_common.h (periféricos), device.h, gpio.h, system.h
│   ├── lib/                 # printf.c (kprintf), systick.c, uart.c (UART0 é igual nos dois)
│   ├── 123/                 # tm4c123.h, startup.c, linker.ld, clock.c
│   └── 129/                 # tm4c129.h, startup.c, linker.ld, clock.c
├── 00-ambiente/ ... 22-flash-interna/
```

Os headers foram **escritos à mão** a partir dos datasheets: `common/include/tm4c_common.h` tem as structs dos periféricos (iguais nas duas famílias) e `common/123/tm4c123.h` / `common/129/tm4c129.h` têm endereços, IRQs e o controle de clock de cada chip.

---

## Módulos

| # | Pasta | Assunto |
|---|---|---|
| 00 | [00-ambiente](00-ambiente/README.md) | Toolchain, Make, OpenOCD/ICDI, lm4flash, GDB, serial |
| 01 | [01-arquitetura-memoria](01-arquitetura-memoria/README.md) | Cortex-M4F, mapa de memória do TM4C, APB×AHB, bit-banding, identificação do chip |
| 02 | [02-startup](02-startup/README.md) | Reset, tabela de vetores, startup em **assembly** e em C, FPU |
| 03 | [03-linker-script](03-linker-script/README.md) | MEMORY/SECTIONS, VMA×LMA, .data/.bss/.noinit, função na RAM |
| 04 | [04-rcc-clock](04-rcc-clock/README.md) | PIOSC/MOSC/PLL, RCC/RCC2 (123), RSCLKCFG/PLLFREQ/MEMTIM0 (129), DIVSCLK |
| 05 | [05-gpio-blink](05-gpio-blink/README.md) | **Hello world**: DIR/DEN/DATA mascarado |
| 06 | [06-systick-nvic](06-systick-nvic/README.md) | SysTick, exceções, NVIC (3 bits de prioridade), DWT |
| 07 | [07-uart](07-uart/README.md) | UART PL011: IBRD/FBRD, FIFOs, interrupção, shell |
| 08 | [08-gpio-entrada-exti](08-gpio-entrada-exti/README.md) | Entrada, pull-up, pinos protegidos (LOCK/CR), interrupção de GPIO |
| 09 | [09-timers-basicos](09-timers-basicos/README.md) | GPTM: periódico, one-shot, contador livre |
| 10 | [10-pwm](10-pwm/README.md) | Módulo PWM: geradores, LOAD/CMPA/GENA, fade, servo |
| 11 | [11-input-capture-encoder](11-input-capture-encoder/README.md) | GPTM edge-time, edge-count, QEI |
| 12 | [12-spi](12-spi/README.md) | SSI: CPSR/SCR, modos, loopback interno, W25Qxx |
| 13 | [13-i2c](13-i2c/README.md) | I2C mestre por comandos, scanner, sensores |
| 14 | [14-adc](14-adc/README.md) | ADC com sequenciadores, média por hardware, sensor de temperatura |
| 15 | [15-dma](15-dma/README.md) | uDMA: tabela de controle, AUTO/BASIC/PING-PONG, UART e ADC |
| 16 | [16-dac](16-dac/README.md) | **DAC por PWM + filtro RC** (o TM4C não tem DAC) |
| 17 | [17-i2s](17-i2s/README.md) | **I2S por software** com timer + uDMA (o TM4C não tem I2S) |
| 18 | [18-can](18-can/README.md) | C_CAN: bit timing, message objects, loopback |
| 19 | [19-watchdog](19-watchdog/README.md) | WDT de dois estágios, causa do reset |
| 20 | [20-rtc](20-rtc/README.md) | Módulo de hibernação: contador RTC, epoch ↔ data, alarme |
| 21 | [21-low-power](21-low-power/README.md) | Sleep, Deep-Sleep, Hibernação |
| 22 | [22-flash-interna](22-flash-interna/README.md) | Apagar/programar Flash (FMA/FMD/FMC), wear leveling |
| 23 | [23-openocd](23-openocd/README.md) | OpenOCD com o ICDI: console, memória com o programa rodando, TCL, semihosting |
| 24 | [24-gdb](24-gdb/README.md) | GDB do zero: breakpoints, watchpoints, `dprintf`, análise de HardFault, scripts |

---

## Uso rápido

```sh
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib arm-none-eabi-binutils \
               arm-none-eabi-gdb openocd picocom make
# opcional: lm4flash (AUR: lm4tools)

cd 05-gpio-blink
make MCU=123 flash        # EK-TM4C123GXL
make MCU=129 flash        # EK-TM4C1294XL

cd ../07-uart
make MCU=123 flash && picocom -b 115200 /dev/ttyACM0
```

## Documentos de referência

| Documento | TM4C123 | TM4C129 |
|---|---|---|
| Datasheet (registradores + elétrica, tudo num só) | TM4C123GH6PM, SPMS376 | TM4C1294NCPDT, SPMS433 |
| Errata | SPMZ849 | SPMZ850 |
| Manual da placa | EK-TM4C123GXL User's Guide (SPMU296) | EK-TM4C1294XL User's Guide (SPMU365) |
| Núcleo | ARM Cortex-M4 Generic User Guide; ARMv7-M Architecture Reference Manual | idem |

Na TI, o "datasheet" de 1200+ páginas faz o papel do Reference Manual da ST. Os códigos de documento acima podem ter sufixo de revisão (ex.: SPMS376E); busque pelo número em ti.com.

## Estado de verificação

Todos os módulos **compilam sem warnings** para os dois chips com o Arm GNU Toolchain 14.2 (`make` na raiz). O código **não foi executado em placa** durante a elaboração do curso (nem há emulador do TM4C no QEMU). Os pontos de maior risco estão sinalizados nos README de cada módulo — principalmente o módulo 17 (I2S por software).
