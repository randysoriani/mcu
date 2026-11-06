# Módulo 11 — Captura de entrada e contador de eventos

> **Objetivo deste módulo:** usar os timers "ao contrário": em vez de gerar sinais, **medir** sinais externos — período, frequência e largura de pulso — com precisão de hardware.

Código:
- [`m328p/main.c`](m328p/main.c)
  - DEMO 1 (padrão): **captura** pelo pino ICP1 (pino 8)
  - DEMO 2: **contador** de pulsos pelo pino T1 (pino 5) — `make clean && make MCU=m328p EXTRA=-DDEMO=2`
- [`uc3a/main.c`](uc3a/main.c) — canal 0 do TC em modo captura

Em todos, o próprio chip gera o sinal de teste (um PWM que muda de frequência a cada 2 s) e você liga um **jumper** da saída à entrada:

| Chip | Gerador | Entrada |
|---|---|---|
| ATmega328P, DEMO 1 | pino 3 (OC2B) | pino 8 (ICP1) |
| ATmega328P, DEMO 2 | pino 3 | pino 5 (T1) |
| UC3A | PB20 (PWM canal 1) | PB23 (TIOA0) |

---

## 1. Duas formas de medir

**Captura (*input capture*):** o timer corre livre; numa borda do pino, o hardware **copia o valor do contador** para um registrador, no exato instante. A diferença entre duas capturas é o período. Analogia: um fotógrafo que tira foto do cronômetro sempre que um corredor cruza a linha.

**Contador de eventos:** o timer usa o **próprio sinal externo** como clock. Contando por 1 segundo, o valor é a frequência. Melhor para frequências altas; a captura é melhor para frequências baixas.

---

## 2. ATmega328P

### 2.1 Captura (Timer1, ICP1 = PB0)
| Bit/registrador | Função |
|---|---|
| **ICR1** | recebe o valor do TCNT1 no momento da borda |
| **ICES1** (TCCR1B bit 6) | borda: 1 = subida, 0 = descida |
| **ICNC1** (TCCR1B bit 7) | filtro de ruído: exige 4 amostras iguais (atrasa 4 ciclos) |
| ICIE1 (TIMSK1 bit 5) | interrupção `TIMER1_CAPT_vect` |
| ICF1 (TIFR1 bit 5) | flag |

Medindo período **e** tempo em alto, alternando a borda:
```
pino   ___|‾‾‾‾‾|_______|‾‾‾‾‾|____
          ▲     ▲       ▲
        subida descida subida
          t0    t1      t2
 alto = t1 − t0      período = t2 − t0
```
```c
ISR(TIMER1_CAPT_vect)
{
    uint16_t t = ICR1;
    if (TCCR1B & (1 << ICES1)) {       /* foi uma subida */
        periodo = t - t_subida;        /* subtração de 16 bits: funciona mesmo se o TCNT1 deu a volta */
        t_subida = t;
        TCCR1B &= ~(1 << ICES1);       /* próxima: descida */
    } else {
        alto = t - t_subida;
        TCCR1B |= (1 << ICES1);
    }
    TIFR1 = (1 << ICF1);               /* trocar a borda pode marcar uma captura falsa */
}
```
Com ÷8 (2 MHz), resolução de **0,5 µs** e períodos até 32 ms (65 536 contagens). Para períodos maiores, conte os estouros (TOIE1).

### 2.2 Contador externo (T1 = PD5)
```c
TCCR1B = (1 << CS12) | (1 << CS11) | (1 << CS10);   /* CS1 = 111: clock = pino T1, borda de subida */
```
O sinal é amostrado pelo clock da CPU: a frequência máxima é ~F_CPU/2,5 (≈ 6 MHz a 16 MHz).

### 2.3 O gerador de teste
Timer2 em Fast PWM com **TOP = OCR2A** (modo 7): o período é ajustável e a saída é a **OC2B** (pino 3):
```
período = OCR2A + 1 contagens de 4 µs (÷64)
alto    = OCR2B + 1 contagens
```

---

## 3. AT32UC3A: TC em modo captura (WAVE = 0)

| Campo do CMR | Bits | Função |
|---|---|---|
| TCCLKS | 2:0 | clock (2 = PBA/8 = 6 MHz no exemplo) |
| LDBSTOP / LDBDIS | 6 / 7 | parar/desligar o clock ao carregar RB |
| ETRGEDG | 9:8 | borda do TIOA que reinicia o contador |
| ABETRG | 10 | gatilho externo em TIOA (1) ou TIOB (0) |
| CPCTRG | 14 | reinicia quando CV == RC |
| **LDRA** | 17:16 | borda do TIOA que carrega o **RA**: 1 = subida, 2 = descida, 3 = ambas |
| **LDRB** | 19:18 | idem para o **RB** |

Com `LDRA = subida` e `LDRB = descida`, o hardware faz sozinho o que o ATmega faz trocando a borda na ISR. As flags **LDRAS** e **LDRBS** do `SR` indicam cada carga (ler o `SR` limpa).

Resolução: 6 MHz → **0,17 µs**.

> Confira o pino TIOA0 do seu encapsulamento na tabela "GPIO Controller Function Multiplexing" do datasheet.

---

## Exercícios
1. Meça a frequência de um sinal de 50 Hz da rede (através de um optoacoplador!).
2. Estenda a captura para 32 bits contando os estouros do Timer1, tratando o caso de estouro e captura simultâneos.
3. Faça um tacômetro: um sensor de efeito Hall num ventilador, mostrando RPM.
4. Meça a largura de pulso de um receptor de rádio-controle (1–2 ms).
