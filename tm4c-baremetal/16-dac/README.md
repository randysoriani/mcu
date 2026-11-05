# Módulo 16 — "DAC" sem DAC: PWM + filtro RC

> **A família TM4C não tem conversor digital-analógico.** O módulo mantém a numeração do curso STM32, mas ensina a técnica usada em todo microcontrolador sem DAC: **PWM de alta frequência filtrado**.

Código: [`src/main.c`](src/main.c)
- 123: **M0PWM0 = PB6** (PCTL 4) | 129: **M0PWM2 = PF2** (PCTL 6)
- PWM com clock = SYSCLK e `LOAD = 511` → **9 bits**, portadora de **156 kHz** (123) / **234 kHz** (129)
- TIMER0 a 32 kHz troca o `CMPA` → senoide de **1 kHz** com 32 pontos
- `EXTRA=-DDEMO=2`: rampa triangular

```sh
make MCU=123 flash
```

---

## 1. Como funciona

Um PWM com duty `D` tem valor médio `D × VDD`. Um filtro passa-baixas remove a portadora e deixa a média — que vira uma tensão analógica controlada pelo duty.

```
pino PWM ──[R 1 kΩ]──┬──[R 1 kΩ]──┬── saída (alta impedância!)
                    [C 100 nF]   [C 100 nF]
                     GND          GND
```

| Parâmetro | Escolha |
|---|---|
| Resolução | `log2(LOAD+1)` bits — mais bits = portadora mais baixa |
| Portadora | `PWMclk / (LOAD+1)` — precisa ficar **muito acima** da maior frequência do sinal |
| Filtro | `fc = 1/(2πRC)` entre o sinal (1 kHz) e a portadora (156 kHz). 1 kΩ + 100 nF → 1,6 kHz |
| Ondulação | 1 estágio RC atenua ~20 dB/década; 2 estágios ~40 dB/década |

Compromisso fundamental: com SYSCLK = 80 MHz, 12 bits dariam portadora de 19,5 kHz (difícil de filtrar sem matar o sinal); 9 bits dão 156 kHz. Técnicas mais avançadas: dois PWMs somados com pesos (MSB + LSB), ou *dithering*/sigma-delta.

## 2. Código essencial

```c
SYSCTL_RCC &= ~SYSCTL_RCC_USEPWMDIV;          /* 123: PWM a 80 MHz (129: PWM0->CC = 0) */
g->LOAD = 511;
g->GENA = 0x8C;                               /* alto no LOAD, baixo em CMPA descendo */
g->CTL  = 1;
PWM0->ENABLE |= 1 << out;

void TIMER0A_Handler(void)                    /* 32 kHz */
{
    TIMER0->ICR = 1;
    PWM0->GEN[g].CMPA = 511 - sine9[idx++ & 31];
}
```
O `CMPA` escrito no meio do período só vale no próximo `LOAD` — cada amostra dura um número inteiro de períodos da portadora (sem glitch).

Custo: uma interrupção de ~20 ciclos a 32 kHz = <1% de CPU. Para zerar a CPU: uDMA disparado pelo timer escrevendo em `CMPA` (exercício).

## 3. Comparação com o DAC do STM32

| | DAC STM32 | PWM + RC (TM4C) |
|---|---|---|
| Resolução | 12 bits | 8–10 bits práticos |
| Banda | ~1 MHz | limitada pelo filtro (kHz) |
| Componentes externos | nenhum (buffer interno) | R e C (e buffer op-amp se a carga não for alta impedância) |
| Pinos | 1–2 fixos | qualquer pino com PWM |

Para qualidade melhor, use um DAC externo: SPI (MCP4921, módulo 12) ou I2C (MCP4725, módulo 13).

---

## Exercícios
1. Monte o filtro de 2 estágios e compare a ondulação no osciloscópio com o de 1 estágio.
2. Troque o ISR por uDMA: TIMER0A (canal 18) escrevendo a tabela em `PWM0->GEN[g].CMPA` em modo ping-pong.
3. Gere 10 bits (`LOAD = 1023`) e calcule a nova portadora e o filtro necessário.
