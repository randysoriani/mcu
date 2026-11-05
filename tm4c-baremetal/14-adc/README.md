# Módulo 14 — ADC (sequenciadores de amostras)

O ADC do TM4C (12 bits, até 1 Msps nos dois chips) é organizado em **sample sequencers**: listas de passos que o hardware executa sozinho a cada disparo, guardando os resultados numa FIFO.

| Sequenciador | Passos | FIFO |
|---|---|---|
| SS0 | 8 | 8 |
| SS1 | 4 | 4 |
| SS2 | 4 | 4 |
| SS3 | 1 | 1 |

Código: [`src/main.c`](src/main.c) — **AIN0 = PE3** (potenciômetro entre 3V3 e GND) e o **sensor de temperatura interno**, pelo SS3, disparo por software, média de 16 amostras por hardware.

```sh
make MCU=123 flash && picocom -b 115200 /dev/ttyACM0
```

---

## 1. Pino analógico
```c
GPIOE->DIR   &= ~(1 << 3);
GPIOE->AFSEL |=  (1 << 3);
GPIOE->DEN   &= ~(1 << 3);     /* desliga o buffer digital */
GPIOE->AMSEL |=  (1 << 3);     /* liga a chave analógica   */
```
Mapa (iguais nos dois chips nos 12 primeiros): AIN0 PE3, AIN1 PE2, AIN2 PE1, AIN3 PE0, AIN4 PD3, AIN5 PD2, AIN6 PD1, AIN7 PD0, AIN8 PE5, AIN9 PE4, AIN10 PB4, AIN11 PB5. O 129 tem até AIN19.

## 2. Registradores (base ADCn)

| Offset | Reg. | |
|---|---|---|
| 0x000 | `ACTSS` | bit n = SSn ativo; (129) bits [11:8] ADENn = DMA do SSn |
| 0x004 | `RIS` | bit n = SSn terminou (com IE) |
| 0x008 | `IM` | bit n = interrupção do SSn no NVIC |
| 0x00C | `ISC` | escreve 1 → limpa |
| 0x010 | `OSTAT` | overflow da FIFO |
| 0x014 | **`EMUX`** | 4 bits por SS: fonte de disparo (0 processador, 4 GPIO, 5 timer, 6–9 PWM, **0xF contínuo**) |
| 0x020 | `SSPRI` | prioridade entre SS |
| 0x028 | **`PSSI`** | escreve 1 no bit n → dispara o SSn |
| 0x030 | `SAC` | média por hardware: 2^SAC amostras (0..6 → 1..64) |
| 0x038 | `CTL` | referência (VREF interno = VDDA, ou externo) |
| 0x040 + 0x20·n | **`SSMUXn`** | 4 bits por passo: canal |
| 0x044 + 0x20·n | **`SSCTLn`** | 4 bits por passo: D (diferencial), **END** (último passo), **IE** (seta RIS), **TS** (sensor de temperatura) |
| 0x048 + 0x20·n | **`SSFIFOn`** | resultado (12 bits) |
| 0x04C + 0x20·n | `SSFSTATn` | estado da FIFO |
| 0xFC4 | `PC` | taxa máxima (1 Msps, 500 k, 250 k, 125 k) |
| 0xFC8 | `CC` | (129) clock: 0 PLL/n, **1 ALTCLK (PIOSC)**, 2 MOSC |

`SSCTL3 = IE0 | END0` → um passo, que é o último e seta a flag.

## 3. Sequência (SS3, software)
```c
ADC0->ACTSS &= ~(1 << 3);          /* desliga para configurar      */
ADC0->EMUX  &= ~(0xF << 12);       /* SS3: disparo por PSSI        */
ADC0->SS[3].MUX = 0;               /* AIN0                         */
ADC0->SS[3].CTL = IE0 | END0;
ADC0->SAC  = 4;                    /* média de 16                  */
ADC0->ACTSS |= (1 << 3);

ADC0->PSSI = (1 << 3);             /* dispara                      */
while (!(ADC0->RIS & (1 << 3))) { }
v = ADC0->SS[3].FIFO;
ADC0->ISC = (1 << 3);
```

## 4. Sensor de temperatura
Com `TS0 = 1` no passo, o canal amostrado é o sensor interno. Fórmula do datasheet:
```
TEMP (°C) = 147,5 − (75 × (VREFP − VREFN) × ADC) / 4096       (VREFP − VREFN = 3,3 V)
```
Em décimos inteiros: `1475 − 2475·ADC/4096`. A precisão é de ±5 °C — serve para monitoramento, não para medir ambiente.

## 5. Sem VREFINT
Diferente do STM32, o TM4C não tem uma referência interna medível: a conversão é relativa a VDDA (VREFA+ = 3,3 V na LaunchPad). Variações da alimentação afetam diretamente a leitura em mV.

## 6. Disparos por hardware
- `EMUX = 5` + `GPTM CTL.TAOTE` (123) / `ADCEV` (129): amostragem a taxa fixa, sem jitter de software.
- `EMUX = 0xF`: conversão contínua — combine com uDMA (módulo 15).
- PWM: amostragem sincronizada à forma de onda (corrente de motor).

---

## Exercícios
1. Use o SS0 para ler AIN0, AIN1 e o sensor numa única sequência de 3 passos.
2. Dispare o SS3 com o TIMER0 a 1 kHz e calcule a média em interrupção.
3. Compare o ruído com `SAC = 0` e `SAC = 6`.
