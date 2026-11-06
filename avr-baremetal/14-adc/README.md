# Módulo 14 — ADC (conversor analógico-digital)

> **Objetivo deste módulo:** transformar uma tensão (de um potenciômetro, sensor de luz, temperatura) num número, entendendo resolução, referência e tempo de conversão. O ATmega328P ainda permite um truque clássico: medir a **própria tensão de alimentação** usando a referência interna.

Código:
- [`m328p/main.c`](m328p/main.c) — A0, Vcc real (pela referência de 1,1 V) e sensor de temperatura interno
- [`uc3a/main.c`](uc3a/main.c) — sensores da EVK1100: temperatura (AD0), potenciômetro (AD1), luz (AD2)

---

## 1. O que o ADC faz

O mundo é **analógico** (qualquer tensão entre 0 e 5 V); o processador só entende **números**. O ADC compara a tensão de entrada com uma **referência** e devolve um número proporcional:
```
valor = Vin × 1024 / Vref           (10 bits: 0 a 1023)
Vin   = valor × Vref / 1024
```
Com Vref = 5 V, cada passo vale 5 / 1024 ≈ **4,9 mV** (a **resolução**). Analogia: uma régua com 1024 marcas; a referência é o comprimento total da régua.

### Aproximação sucessiva
O ADC dos dois chips descobre o número bit a bit, como no jogo de adivinhar um número com "maior/menor": é maior que metade da referência? E que 3/4?... São 10 passos para 10 bits. Antes disso, um capacitor interno é carregado com a tensão de entrada (amostragem): fontes com resistência alta (> 10 kΩ) não o carregam a tempo e dão leituras erradas.

---

## 2. ATmega328P

### 2.1 Características
- 10 bits, **um** conversor com um multiplexador de 8 canais externos (A0–A5 no Uno DIP; A6/A7 só no SMD/Nano) e canais internos.
- Clock do ADC **entre 50 e 200 kHz** para 10 bits completos: a 16 MHz, prescaler ÷128 = 125 kHz.
- Uma conversão = 13 ciclos de ADC ≈ **104 µs** (a primeira após ligar, 25 ciclos).

### 2.2 Registradores
| Registrador | Endereço | Bits |
|---|---|---|
| **ADMUX** | 0x7C | **REFS1:0** (7:6): referência; ADLAR (5): alinha à esquerda; **MUX3:0** (3:0): canal |
| **ADCSRA** | 0x7A | **ADEN** (7): liga; **ADSC** (6): inicia (volta a 0 no fim); ADATE (5): disparo automático; ADIF (4): flag; ADIE (3): interrupção; **ADPS2:0**: prescaler (111 = ÷128) |
| **ADCL / ADCH** | 0x78 / 0x79 | resultado (**ler ADCL primeiro**: trava o ADCH até ser lido) |
| ADCSRB | 0x7B | ADTS2:0: fonte do disparo automático (timer, comparador...) |
| **DIDR0** | 0x7E | desliga a entrada digital dos pinos analógicos (menos consumo e ruído) |

Referências (REFS1:0):
| 00 | 01 | 11 |
|---|---|---|
| pino AREF externo | **AVcc** (5 V no Uno) | **1,1 V interna** |

Canais (MUX3:0): 0–7 = ADC0–ADC7; **8 = sensor de temperatura**; **14 = 1,1 V (bandgap)**; 15 = GND.

### 2.3 A conversão
```c
ADCSRA = (1 << ADEN) | (7 << ADPS0);      /* liga, ÷128 */
ADMUX  = (1 << REFS0) | 0;                /* referência AVcc, canal 0 */
ADCSRA |= (1 << ADSC);                    /* inicia */
while (ADCSRA & (1 << ADSC)) { }          /* espera */
uint16_t v = ADC;                          /* ADCL e ADCH */
```

### 2.4 Medindo o Vcc sem nenhum pino
A referência de 1,1 V é fixa; o Vcc varia (USB pode dar 4,7 V; uma bateria descarrega). Medindo **o canal de 1,1 V usando o Vcc como referência**:
```
leitura = 1,1 × 1024 / Vcc     →     Vcc = 1,1 × 1024 / leitura
```
Com isso, as leituras do A0 ficam corretas mesmo se o Vcc não for exatamente 5 V. A precisão da referência de 1,1 V é ±10% entre chips: calibre medindo o Vcc com um multímetro uma vez.

Ao **trocar a referência**, as primeiras leituras saem erradas (o capacitor do AREF precisa assentar): o exemplo descarta uma e espera 2 ms.

### 2.5 Sensor de temperatura
Canal 8 com a referência de 1,1 V. O datasheet dá ~314 mV a 25 °C e ~1 mV/°C, mas o erro sem calibração é de **±10 °C**: serve para tendências, não para um termômetro.

---

## 3. AT32UC3A

- 10 bits, 8 canais, referência no pino ADVREF (3,3 V na EVK1100).
- Base `0xFFFF_3C00`.

| Offset | Registrador | Função |
|---|---|---|
| 0x00 | **CR** | SWRST (0), **START** (1) |
| 0x04 | **MR** | TRGEN/TRGSEL: disparo por hardware; LOWRES (4): 8 bits; SLEEP (5); **PRESCAL** (15:8); **STARTUP** (20:16); **SHTIM** (27:24): tempo de amostragem |
| 0x10 / 0x14 | **CHER / CHDR** | liga / desliga canais |
| 0x1C | **SR** | EOC0–7: fim de conversão de cada canal; OVRE; DRDY |
| 0x20 | LCDR | último resultado |
| 0x30–0x4C | **CDR0..7** | resultado de cada canal (ler limpa o EOC) |

```
ADCclk = PBA / ((PRESCAL + 1) × 2)   ≤ 5 MHz em 10 bits
48 MHz / ((4 + 1) × 2) = 4,8 MHz
```
Um `START` converte **todos** os canais habilitados em sequência; cada resultado fica no seu `CDRn`. O exemplo habilita um canal por vez para simplificar. Com o PDCA (módulo 19), as conversões podem ir direto para a memória.

> Confira no esquemático da EVK1100 quais sensores estão em quais canais.

---

## Exercícios
1. Faça uma média de 16 leituras e compare o ruído com a leitura simples.
2. Use o ADC em modo livre (ADATE, ADTS = 000) com a interrupção `ADC_vect`.
3. Dispare o ADC pelo Timer1 (ADTS = 101) a exatamente 1 kHz e mande os dados pela serial.
4. Calibre o sensor de temperatura em dois pontos (água com gelo e temperatura ambiente).
