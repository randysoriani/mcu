# Curso STM32 Bare-Metal no Linux

**STM32F0 (Cortex-M0) e STM32F4 (Cortex-M4F) sem HAL, sem LL, sem CMSIS, sem IDE.**
Só `arm-none-eabi-gcc`, `make`, um editor de texto e o Reference Manual.

Cada módulo é uma pasta com:

- `README.md` — teoria, diagramas, **tabelas de registradores bit a bit**, diferenças F0 × F4, armadilhas e exercícios;
- código para os dois microcontroladores (`f0/`, `f4/` e/ou `src/` compartilhado);
- um `Makefile` de 2 linhas que inclui as regras comuns (`common/mk/rules.mk`).

---

## Hardware de referência

| | STM32F0 | STM32F4 |
|---|---|---|
| Placa | **Nucleo-F072RB** | **Nucleo-F446RE** |
| Núcleo | Cortex-M0 (ARMv6-M), 48 MHz | Cortex-M4F (ARMv7E-M) + FPU, 180 MHz |
| Flash / RAM | 128 KB / 16 KB | 512 KB / 128 KB |
| Debugger | ST-LINK/V2-1 na própria placa | idem |

As duas Nucleo-64 têm **o mesmo pinout**: LED verde LD2 em **PA5**, botão azul B1 em **PC13**, e a **USART2 (PA2/PA3)** ligada ao ST-LINK, que aparece no Linux como `/dev/ttyACM0`. Por isso o mesmo programa quase sempre roda nas duas mudando poucas linhas.

> Por que F072 e F446? São os membros das famílias F0/F4 que têm **todos** os periféricos do curso (CAN, I2S, DAC, USB...). O F030 não tem CAN/I2S/DAC; o F401/F411 não tem CAN/DAC. Os README indicam o que muda para outros chips (F030, F103-like, F401, F411, F407).

Material extra (opcional, por módulo): potenciômetro, jumpers, módulo I2C qualquer (MPU6050, BME280, EEPROM 24C32...), memória SPI W25Qxx, transceiver CAN (SN65HVD230), DAC I2S (PCM5102A ou MAX98357A), osciloscópio ou analisador lógico barato (Saleae-clone + PulseView/sigrok).

---

## Estrutura

```
stm32-baremetal/
├── Makefile                 # compila todos os módulos (teste de sanidade)
├── common/
│   ├── mk/rules.mk          # regras de build compartilhadas
│   ├── include/             # device.h, gpio.h, system.h (interface comum)
│   ├── lib/                 # printf.c (kprintf), systick.c (portáveis)
│   ├── f0/                  # stm32f0.h, startup.c, linker.ld, clock.c, uart.c
│   └── f4/                  # stm32f4.h, startup.c, linker.ld, clock.c, uart.c
├── 00-ambiente/
├── 01-arquitetura-memoria/
├── ...
└── 22-flash-interna/
```

O que está em `common/` é exatamente o código **construído nos módulos**: o startup do módulo 02, o linker script do 03, o clock do 04, o SysTick do 06, a UART do 07. A partir do momento em que um assunto foi estudado, os módulos seguintes passam a reutilizá-lo para focar no periférico novo.

Os headers `common/f0/stm32f0.h` e `common/f4/stm32f4.h` foram **escritos à mão** a partir dos Reference Manuals: cada periférico é uma `struct` com os registradores no offset correto. Leia-os — eles são parte do curso.

---

## Módulos

| # | Pasta | Assunto |
|---|---|---|
| 00 | [00-ambiente](00-ambiente/README.md) | Toolchain, Make, OpenOCD, st-flash, GDB, terminal serial, QEMU |
| 01 | [01-arquitetura-memoria](01-arquitetura-memoria/README.md) | Cortex-M0 × M4, registradores do núcleo, mapa de memória, barramentos, bit-banding, `volatile` |
| 02 | [02-startup](02-startup/README.md) | Sequência de reset, tabela de vetores, startup em **assembly** e em C, weak symbols, FPU |
| 03 | [03-linker-script](03-linker-script/README.md) | MEMORY/SECTIONS, VMA×LMA, .text/.rodata/.data/.bss/.noinit, função na RAM, map file |
| 04 | [04-rcc-clock](04-rcc-clock/README.md) | Árvore de clock, HSI/HSE/PLL/HSI48, wait states, over-drive, prescalers, MCO |
| 05 | [05-gpio-blink](05-gpio-blink/README.md) | **Hello world**: GPIO como saída, MODER/OTYPER/OSPEEDR/PUPDR/ODR/BSRR |
| 06 | [06-systick-nvic](06-systick-nvic/README.md) | SysTick, exceções, prioridades, `millis()`, multitarefa cooperativa, DWT |
| 07 | [07-uart](07-uart/README.md) | USART: baud rate, polling, interrupção, buffer circular, shell interativo |
| 08 | [08-gpio-entrada-exti](08-gpio-entrada-exti/README.md) | GPIO entrada, pull-up/down, debounce, EXTI, SYSCFG, NVIC |
| 09 | [09-timers-basicos](09-timers-basicos/README.md) | Timers: PSC/ARR, update interrupt, contador 32 bits, one-pulse |
| 10 | [10-pwm](10-pwm/README.md) | PWM: modos de saída, CCMR/CCER, fade de LED, servo, timers avançados |
| 11 | [11-input-capture-encoder](11-input-capture-encoder/README.md) | Input capture, PWM input, contador de eventos externos, encoder |
| 12 | [12-spi](12-spi/README.md) | SPI: CPOL/CPHA, mestre, FIFO do F0, loopback, Flash W25Qxx |
| 13 | [13-i2c](13-i2c/README.md) | I2C: protocolo, IP v2 (F0, TIMINGR) × IP v1 (F4, SR1/SR2), scanner, sensores |
| 14 | [14-adc](14-adc/README.md) | ADC: amostragem, calibração, VREFINT, sensor de temperatura |
| 15 | [15-dma](15-dma/README.md) | DMA: canais (F0) × streams (F4), circular, ADC→RAM, UART TX, mem→mem |
| 16 | [16-dac](16-dac/README.md) | DAC: trigger por timer, senoide por DMA, onda triangular por hardware |
| 17 | [17-i2s](17-i2s/README.md) | I2S: áudio digital, PLLI2S, Philips, DMA circular |
| 18 | [18-can](18-can/README.md) | bxCAN: bit timing, filtros, mailboxes, loopback, modo normal |
| 19 | [19-watchdog](19-watchdog/README.md) | IWDG e WWDG, causa do reset |
| 20 | [20-rtc](20-rtc/README.md) | RTC: LSE/LSI, backup domain, calendário BCD, alarme |
| 21 | [21-low-power](21-low-power/README.md) | Sleep, Stop, Standby, pinos de wakeup |
| 22 | [22-flash-interna](22-flash-interna/README.md) | Apagar/programar a Flash, wear leveling |

---

## Uso rápido

```sh
# instalar (Arch)
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib arm-none-eabi-binutils \
               arm-none-eabi-gdb openocd stlink picocom

cd 05-gpio-blink
make MCU=f0            # compila para a Nucleo-F072RB
make MCU=f0 flash      # grava via OpenOCD
make MCU=f4 flash      # compila e grava na Nucleo-F446RE

cd ../07-uart
make MCU=f4 flash && picocom -b 115200 /dev/ttyACM0
```

Na raiz, `make` compila todos os módulos para os dois MCUs.

## Documentos de referência (baixe antes de começar)

| Documento | F072 | F446 |
|---|---|---|
| Reference Manual (registradores) | RM0091 | RM0390 |
| Datasheet (pinos, funções alternativas, elétrica) | DS9826 | DS10693 |
| Errata (bugs de silício conhecidos) | "STM32F07x errata sheet" | ES0298 |
| Manual da placa Nucleo-64 | UM1724 | UM1724 |
| Programming Manual do núcleo | PM0215 (M0) | PM0214 (M4) |
| Arquitetura ARM | ARMv6-M ARM (DDI0419) | ARMv7-M ARM (DDI0403) |

Tudo está em st.com / developer.arm.com, gratuito. O curso cita seções desses documentos: tê-los abertos lado a lado é a habilidade mais importante que você vai desenvolver.
