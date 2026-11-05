# Módulo 08 — GPIO como entrada e interrupções de GPIO

Código: [`src/main.c`](src/main.c)
- botão 1 com interrupção na borda de descida + debounce: **SW1 = PF4** (123) / **USR_SW1 = PJ0** (129)
- botão 2 lido por polling: **SW2 = PF0** (123, pino protegido → unlock) / **USR_SW2 = PJ1** (129)

Os botões das LaunchPads vão ao GND e **não têm pull-up externo**: o pull-up interno (`PUR`) é obrigatório.

```sh
make MCU=123 flash && picocom -b 115200 /dev/ttyACM0
```

---

## 1. Entrada digital
```c
gpio_port_enable(PORTF);
GPIOF->DIR &= ~(1U << 4);     /* entrada  */
GPIOF->PUR |=  (1U << 4);     /* pull-up  */
GPIOF->DEN |=  (1U << 4);     /* digital! */
if (GPIOF->DATA_BITS[1U << 4] == 0) { /* apertado */ }
```
Pull-ups internos: ~13–30 kΩ (ver datasheet, *GPIO electrical characteristics*). Pinos 5 V tolerantes: a maioria (exceto PB0/PB1 no modo USB, PD4/PD5 e pinos analógicos — consulte a tabela de pinos).

## 2. Pinos protegidos: LOCK e CR

Alguns pinos têm funções críticas (JTAG, NMI) e seus registradores `AFSEL`, `PUR`, `PDR`, `DEN` só podem ser alterados depois de destravar:

```c
GPIOF->LOCK = 0x4C4F434B;     /* "LOCK" em ASCII */
GPIOF->CR  |= (1U << 0);      /* commit: libera PF0 */
GPIOF->LOCK = 0;              /* trava de novo (opcional) */
```

| Chip | Protegidos |
|---|---|
| 123 | PC0–PC3 (JTAG), PD7 (NMI), **PF0** (NMI) |
| 129 | PC0–PC3 (JTAG), PD7 (NMI), PE7 (NMI) |

Sem o unlock, configurar o SW2 (PF0) do 123 simplesmente não tem efeito — os pull-ups não ligam e o pino flutua.

## 3. Interrupções de GPIO

No TM4C **cada porta tem a sua IRQ** (GPIOA = 0, ..., GPIOF = 30, GPIOJ = 51 no 129...). Os 8 pinos da porta compartilham o vetor. Não existe EXTI nem SYSCFG.

| Offset | Reg. | 0 | 1 |
|---|---|---|---|
| 0x404 | `IS` | borda | nível |
| 0x408 | `IBE` | uma borda (IEV decide) | **ambas** as bordas |
| 0x40C | `IEV` | descida / nível baixo | subida / nível alto |
| 0x410 | `IM` | mascarada | habilitada |
| 0x414 | `RIS` | | evento ocorreu (mesmo mascarado) |
| 0x418 | `MIS` | | evento ocorreu E habilitado |
| 0x41C | `ICR` | | escreve 1 → limpa (só para borda) |

Sequência (mascare antes de configurar para não gerar interrupção espúria):
```c
GPIOF->IM  &= ~(1U << 4);
GPIOF->IS  &= ~(1U << 4);     /* borda   */
GPIOF->IBE &= ~(1U << 4);     /* uma só  */
GPIOF->IEV &= ~(1U << 4);     /* descida */
GPIOF->ICR  =  (1U << 4);
GPIOF->IM  |=  (1U << 4);
NVIC_EnableIRQ(GPIOF_IRQn);
```
Handler:
```c
void GPIOF_Handler(void)
{
    uint32_t mis = GPIOF->MIS;    /* quais pinos */
    GPIOF->ICR = mis;             /* limpa só esses */
    if (mis & (1U << 4)) { ... }
}
```
O TM4C129 ainda tem, nas portas P e Q, **uma IRQ por pino** (GPIOP0..P7, GPIOQ0..Q7) para latência mínima.

## 4. Debounce
Botões quicam 1–10 ms. O exemplo aceita uma borda e ignora outras por 50 ms (`millis()`). Alternativas: amostragem periódica com contador, ou RC + Schmitt.

## 5. Interrupção também acorda
Uma IRQ de GPIO acorda o núcleo de Sleep e Deep-Sleep (módulo 21) — desde que a porta tenha clock no modo correspondente (`SCGCGPIO`/`DCGCGPIO`).

---

## Exercícios
1. Interrupção nas duas bordas (`IBE = 1`) medindo a duração do aperto na ISR.
2. Tire o unlock de PF0 (123) e observe o botão 2 "travado".
3. Use os dois botões para escolher a cor do LED RGB (123) ou o LED aceso (129).
4. No 129, faça o botão gerar interrupção por **nível** baixo e explique por que isso trava o programa se a ISR não mascarar o pino.
