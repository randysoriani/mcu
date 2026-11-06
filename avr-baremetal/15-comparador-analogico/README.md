# Módulo 15 — Comparador analógico (só ATmega328P)

> **Objetivo deste módulo:** usar o comparador analógico, um periférico típico dos AVR de 8 bits que responde **instantaneamente** à pergunta "esta tensão é maior que aquela?" — sem a demora de uma conversão do ADC — e pode gerar interrupções, disparar o timer e acordar o chip.

Código: [`m328p/main.c`](m328p/main.c) — compara a tensão no **pino 7** com a referência interna de **1,1 V**; o LED mostra o resultado.

Montagem: potenciômetro de 10 kΩ entre 5 V e GND, cursor no pino 7.

> O **AT32UC3A não tem comparador analógico**; `make MCU=uc3a` neste módulo só mostra uma mensagem. A alternativa no UC3 é o ADC com uma comparação por software.

---

## 1. O que é um comparador

Um circuito com duas entradas e uma saída digital:
```
 V+ ──┐
      │ \
      │  >── saída = 1 se V+ > V−, 0 caso contrário
      │ /
 V− ──┘
```
Analogia: uma balança de dois pratos — só diz qual lado pesa mais, mas diz na hora.

Usos: detectar que uma bateria caiu abaixo de um limite, um sinal cruzou o zero, um sensor passou de um nível, medir resistências/capacitâncias pelo tempo de carga de um capacitor.

Diferença para o ADC: o ADC dá **quanto** (10 bits, ~100 µs); o comparador dá **maior/menor** (1 bit, < 1 µs, contínuo).

---

## 2. Comparador do ATmega328P

### 2.1 Entradas
| Entrada | Opções |
|---|---|
| **positiva (AIN0)** | pino 6 (PD6) **ou** a referência interna de 1,1 V (bit ACBG) |
| **negativa (AIN1)** | pino 7 (PD7) **ou** qualquer entrada do multiplexador do ADC (A0–A7), se o ADC estiver desligado e ACME = 1 |

### 2.2 Registradores
| Registrador | Endereço | Bits |
|---|---|---|
| **ACSR** | 0x50 | ACD (7): **desliga** o comparador (1 = desligado); **ACBG** (6): + = 1,1 V; **ACO** (5): saída (só leitura); **ACI** (4): flag (limpa escrevendo 1); **ACIE** (3): interrupção; ACIC (2): liga ao **input capture** do Timer1; **ACIS1:0** (1:0): quando interromper |
| ADCSRB | 0x7B | ACME (6): usa o multiplexador do ADC para a entrada negativa |
| ADMUX | 0x7C | MUX2:0: qual pino A0–A7 (com ACME = 1 e ADEN = 0) |
| **DIDR1** | 0x7F | AIN1D (1), AIN0D (0): desliga a parte digital dos pinos |

ACIS1:0:
| 00 | 10 | 11 |
|---|---|---|
| qualquer mudança da saída | descida da saída | subida da saída |

### 2.3 O programa
```c
DIDR1 = (1 << AIN1D);          /* pino 7 só analógico */
ACSR  = (1 << ACBG)            /* + = 1,1 V */
      | (1 << ACI)             /* limpa flag antiga */
      | (1 << ACIE);           /* interrupção em qualquer mudança */
sei();

ISR(ANALOG_COMP_vect)
{
    if (ACSR & (1 << ACO)) led_on(); else led_off();
}
```
ACO = 1 quando **V+ > V−**, ou seja, quando o pino 7 está **abaixo** de 1,1 V.

Mudar o `ACBG` ou o `ACIS` pode gerar uma interrupção falsa: desligue o `ACIE`, mude, limpe o `ACI` e religue.

### 2.4 Histerese e ruído
Perto do limite, um sinal com ruído cruza várias vezes, gerando rajadas de interrupções (veja o contador de trocas subir rápido ao girar o potenciômetro devagar). O comparador do AVR não tem histerese interna. Soluções:
- histerese externa: um resistor alto da saída para a entrada positiva (num comparador externo) ou referência em dois níveis;
- software: ignorar mudanças por alguns milissegundos (como o debounce do módulo 08);
- um capacitor de filtro na entrada.

### 2.5 Integração com o Timer1
Com **ACIC = 1**, a saída do comparador vira a fonte do **input capture** do Timer1 (módulo 11): o instante exato do cruzamento é registrado em ICR1. Base de medidores de capacitância e de detectores de passagem por zero.

---

## Exercícios
1. Use o pino 6 como entrada positiva (ACBG = 0) com um divisor resistivo de 2,5 V.
2. Use o A0 como entrada negativa (ACME = 1, ADEN = 0, MUX = 0).
3. Meça um capacitor: carregue por um resistor, use ACIC e o Timer1 para medir quanto tempo leva até 1,1 V.
4. Adicione um debounce de 20 ms e compare o número de trocas.
