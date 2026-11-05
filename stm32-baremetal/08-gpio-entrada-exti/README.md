# Módulo 08 — GPIO como entrada, EXTI e NVIC

Ler um botão parece trivial — até aparecerem o *bounce*, o pino flutuando e a pergunta "polling ou interrupção?".

Código: [`src/main.c`](src/main.c) (F0 e F4)
- botão **B1** (PC13, ativo em **nível baixo**) gera interrupção na borda de descida pela **EXTI13**
- *debounce* por software com `millis()`
- distingue clique curto (alterna o LED) de longo (> 1 s)
- **PA10** (Arduino D2) lido por *polling* com pull-up interno — encoste um fio do D2 ao GND

```sh
make MCU=f0 flash && picocom -b 115200 /dev/ttyACM0
```

---

## 1. Entrada digital

```c
gpio_mode(GPIOC, 13, GPIO_MODE_INPUT);   /* MODER13 = 00 */
gpio_pull(GPIOC, 13, GPIO_NOPULL);       /* a Nucleo já tem pull-up externo */
if ((GPIOC->IDR & (1U << 13)) == 0) { /* apertado */ }
```

### Pull-up / pull-down (`PUPDR`)

Um pino de entrada desconectado **flutua**: lê 0 ou 1 aleatoriamente e consome mais corrente. Sempre defina um nível de repouso:
- botão para GND → **pull-up** (repouso = 1, apertado = 0) — caso do B1 e do exemplo em PA10;
- botão para VDD → pull-down.

Os pull-ups internos são fracos (~40 kΩ típicos, 30–50 kΩ). Para fios longos ou ambientes ruidosos, use resistor externo (4,7–10 kΩ).

### Níveis e tolerância a 5 V

Pinos marcados **FT** no datasheet aceitam 5 V como entrada (quando não estão em modo analógico). Pinos **TT**/**TC** não. Limiar típico: V_IL < 0,3·VDD, V_IH > 0,7·VDD (CMOS); há histerese (Schmitt trigger).

---

## 2. Bounce (trepidação)

```
apertou                     estável
   │  ┌┐ ┌─┐┌┐              │
───┘  └┘ └┘ └──────────────────  (0 a ~10 ms de pulsos)
```
Contatos mecânicos quicam. Sem tratamento, um aperto conta 3, 5, 10 vezes. Técnicas:
1. **Janela de tempo** (usado aqui): após aceitar uma borda, ignora outras por 50 ms.
2. **Amostragem periódica**: lê a cada 5–10 ms e só aceita após N leituras iguais.
3. **Hardware**: RC (10 kΩ + 100 nF) + entrada Schmitt.

---

## 3. EXTI — External Interrupt/Event Controller

A EXTI transforma bordas em pinos (e eventos internos: PVD, RTC, USB wakeup, comparadores) em interrupções ou eventos.

```
 PA13 ┐
 PB13 ┤
 PC13 ┼─► mux SYSCFG_EXTICR4[7:4] ─► linha EXTI13 ─► detector de borda ─► PR13 ─► IMR13 ─► NVIC
 ...  ┘                                              (RTSR/FTSR)                  (EMR13 ─► evento p/ WFE)
```

**Ponto-chave**: há **uma** linha EXTI por número de pino. PA13, PB13, PC13... compartilham a EXTI13 — só **um** deles pode estar ligado a ela de cada vez (escolhido no SYSCFG).

### Registradores da EXTI (iguais no F0 e no F4)

Base: F0 `0x4001_0400`, F4 `0x4001_3C00`.

| Offset | Registrador | Descrição |
|---|---|---|
| 0x00 | `IMR` | 1 = interrupção da linha habilitada |
| 0x04 | `EMR` | 1 = evento habilitado (acorda WFE sem executar ISR) |
| 0x08 | `RTSR` | 1 = dispara na borda de subida |
| 0x0C | `FTSR` | 1 = dispara na borda de descida |
| 0x10 | `SWIER` | 1 = gera a interrupção por software |
| 0x14 | `PR` | 1 = borda detectada (pendente). **Limpa escrevendo 1** (rc_w1) |

Linhas internas (exemplos): F4 — 16 PVD, 17 RTC alarm, 18 USB FS wakeup, 21 RTC tamper, 22 RTC wakeup. F0 — 17 RTC alarm, 19 RTC tamper, 20 RTC wakeup (F07x), 21/22 comparadores, 23 I2C1 wakeup, 25 USART1 wakeup, ... (algumas são "direct lines" sem bit de borda).

### SYSCFG_EXTICR — escolhendo a porta

`SYSCFG` precisa de clock (F0: `RCC_APB2ENR.SYSCFGCOMPEN` bit 0; F4: `RCC_APB2ENR.SYSCFGEN` bit 14).

| Registrador | Linhas | Campo de 4 bits por linha |
|---|---|---|
| `EXTICR[0]` (offset 0x08) | 0–3 | |
| `EXTICR[1]` (0x0C) | 4–7 | 0000 = PA, 0001 = PB, |
| `EXTICR[2]` (0x10) | 8–11 | 0010 = PC, 0011 = PD, |
| `EXTICR[3]` (0x14) | 12–15 | 0100 = PE, 0101 = PF (F0) / 0111 = PH (F4) |

PC13: `EXTICR[13/4] = EXTICR[3]`, bits `4*(13%4) = [7:4]`, valor `2`.

### IRQs da EXTI no NVIC

| Linhas | F072 | F446 |
|---|---|---|
| 0 | `EXTI0_1_IRQn` (5) | `EXTI0_IRQn` (6) |
| 1 | idem | `EXTI1_IRQn` (7) |
| 2, 3 | `EXTI2_3_IRQn` (6) | `EXTI2_IRQn` (8), `EXTI3_IRQn` (9) |
| 4 | `EXTI4_15_IRQn` (7) | `EXTI4_IRQn` (10) |
| 5–9 | idem | `EXTI9_5_IRQn` (23) |
| 10–15 | idem | `EXTI15_10_IRQn` (40) |

Quando várias linhas compartilham um vetor, o handler **precisa testar** o `PR` para saber qual disparou e limpar só aquele bit:

```c
void EXTI15_10_IRQHandler(void)        /* F4. No F0: EXTI4_15_IRQHandler */
{
    if (EXTI->PR & (1U << 13)) {
        EXTI->PR = (1U << 13);         /* NÃO use |= : limparia todas as pendentes */
        ...
    }
}
```

> `EXTI->PR |= (1 << 13)` é um bug sutil: lê o PR (com outras linhas pendentes = 1) e escreve de volta → limpa todas.

---

## 4. Sequência completa de configuração

```c
/* 1. clocks */
RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;          /* F0: RCC->AHBENR |= GPIOCEN */
RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
/* 2. pino como entrada */
GPIOC->MODER &= ~(3U << 26);
/* 3. mux */
SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~(0xF << 4)) | (2 << 4);
/* 4. borda e máscara */
EXTI->FTSR |= 1U << 13;
EXTI->RTSR &= ~(1U << 13);
EXTI->PR    = 1U << 13;                       /* descarta pendência antiga */
EXTI->IMR  |= 1U << 13;
/* 5. NVIC */
NVIC_SetPriority(EXTI15_10_IRQn, 3);
NVIC_EnableIRQ(EXTI15_10_IRQn);
```

---

## 5. Polling × interrupção

| | Polling | Interrupção |
|---|---|---|
| Latência | depende do laço | ~µs |
| CPU | gasta lendo sempre | só quando há evento |
| Pode dormir (WFI/Stop)? | não | **sim** — a EXTI acorda até do Stop |
| Complexidade | baixa | variáveis `volatile`, concorrência, debounce |
| Bom para | sinais lentos, muitos botões em matriz | eventos raros, wake-up, sinais rápidos |

O exemplo mostra o padrão recomendado: **ISR curta** (marca um evento e registra o tempo) e o processamento pesado (medir a duração, imprimir) no `main`.

---

## Exercícios

1. Configure a EXTI para as **duas** bordas e meça a duração do aperto dentro da própria ISR (sem o laço no `main`).
2. Ligue um segundo botão em PB13 (mesma linha EXTI13 que PC13). O que acontece? Mova-o para PB12.
3. Implemente o debounce por amostragem (a cada 5 ms no SysTick, aceita após 4 leituras iguais) e compare.
4. Use `EXTI->SWIER` para disparar a interrupção por software a cada 2 s.
5. Faça um teclado matricial 4×4 com linhas como saída e colunas como entrada com pull-up.
