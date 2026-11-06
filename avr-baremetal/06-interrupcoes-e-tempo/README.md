# Módulo 06 — Interrupções e base de tempo

> **Objetivo deste módulo:** entender o que é uma interrupção, como escrever uma rotina de interrupção (ISR) e criar um relógio de milissegundos (`millis()`) — sem o SysTick que os ARM têm. Também o problema da **atomicidade**, que num processador de 8 bits aparece até para ler uma simples variável.

Código: [`src/main.c`](src/main.c) — implementa a própria base de tempo (sem `common/<mcu>/tick.c`) e faz duas "tarefas" cooperativas.
- ATmega328P: Timer0 em modo CTC a 1 kHz
- AT32UC3A: registradores COUNT/COMPARE do núcleo

```sh
make MCU=m328p flash && picocom -b 115200 /dev/ttyACM0
```
(No QEMU este módulo trava no laço de medição: o QEMU não emula o Timer0. Os demais módulos usam `-DQEMU` para trocar pelo Timer1.)

---

## 1. O que é uma interrupção

Sem interrupções, para saber se algo aconteceu (chegou um byte, passou 1 ms) o programa precisa perguntar o tempo todo — isso se chama *polling*. Com interrupções, **o hardware avisa**: a CPU para o que está fazendo, executa uma função curta (a **ISR**) e volta exatamente de onde parou.

Analogia: em vez de ir até a porta a cada minuto ver se o carteiro chegou, você instala uma campainha.

```
programa principal:  ──────────────┐            ┌──────────────
                                   │ interrupção│
ISR:                               └──── ISR ───┘
                                   salva       restaura
```

Três coisas precisam estar ligadas para uma interrupção acontecer:
1. a **fonte** no periférico (ex.: "interrupção na comparação do Timer0", bit `OCIE0A` do `TIMSK0`);
2. as interrupções **globais** (bit I do SREG no AVR8; bit GM no AVR32);
3. a **ISR** existir (senão vale a rotina padrão do startup, módulo 02).

---

## 2. ATmega328P

### 2.1 Como a CPU atende
1. Termina a instrução atual.
2. Salva o PC na pilha e **desliga o bit I** do SREG (por isso, por padrão, uma ISR não é interrompida por outra).
3. Salta para a posição da tabela de vetores (módulo 02).
4. A ISR salva os registradores que vai usar (código gerado pelo atributo `signal`), faz o trabalho e termina com `RETI`, que religa o bit I e volta.

Tempo de entrada: ~4 ciclos de hardware + o salvamento de registradores, totalizando algo como 2–5 µs a 16 MHz. **ISRs devem ser curtas.**

```c
ISR(TIMER0_COMPA_vect)      /* macro do curso: define __vector_14 com o atributo signal */
{
    g_ms++;
}
sei();                      /* SREG.I = 1: liga as interrupções globais */
cli();                      /* SREG.I = 0: desliga */
```
Prioridade: fixa, pela posição na tabela (vetor menor = atendido primeiro se houver duas pendentes).

### 2.2 O Timer0 como relógio de 1 ms
O Timer0 é um contador de 8 bits que soma 1 a cada pulso do seu clock. No **modo CTC** (*Clear Timer on Compare*), ao chegar ao valor de `OCR0A` ele volta a zero e gera a interrupção de comparação.

```
f_interrupção = F_CPU / (prescaler × (OCR0A + 1))
1000 Hz       = 16 000 000 / (64 × 250)
```
```c
TCCR0A = (1 << WGM01);                   /* modo CTC */
OCR0A  = 249;                            /* conta 0..249 = 250 passos */
TCCR0B = (1 << CS01) | (1 << CS00);      /* prescaler ÷64 (liga o timer) */
TIMSK0 = (1 << OCIE0A);                  /* interrupção na comparação A */
sei();
```
O Timer0 é o mesmo timer que o Arduino usa para o seu `millis()`. Os timers são detalhados no módulo 09.

### 2.3 Atomicidade: o problema dos 4 bytes
`g_ms` é um `uint32_t`: **4 bytes**. O AVR de 8 bits lê um byte por instrução, então ler `g_ms` leva 4 instruções. Se a interrupção acontecer no meio, você pode ler 2 bytes antigos e 2 novos:

```
g_ms = 0x0000_00FF ; o main lê o byte baixo: 0xFF
          ← interrupção: g_ms vira 0x0000_0100
                   ; o main lê os outros bytes: 0x01, 0x00, 0x00
resultado: 0x0000_01FF  (errado! nem o valor antigo nem o novo)
```
A solução: desligar as interrupções durante a cópia, e restaurar o estado anterior:
```c
uint32_t millis(void)
{
    uint8_t s = SREG;      /* guarda o estado (as interrupções podiam estar desligadas) */
    cli();
    uint32_t v = g_ms;
    SREG = s;              /* restaura */
    return v;
}
```
Regra: no AVR de 8 bits, **qualquer variável de mais de 1 byte compartilhada com uma ISR** precisa dessa proteção (a avr-libc tem a macro `ATOMIC_BLOCK` com esse fim).

E sempre **`volatile`** nessas variáveis: avisa o compilador que elas mudam "por fora" do fluxo normal.

---

## 3. AT32UC3A

### 3.1 COUNT e COMPARE
O núcleo AVR32 tem dois registradores de sistema para isso:
- **COUNT** (registrador de sistema 264): soma 1 a cada ciclo da CPU, sozinho.
- **COMPARE** (268): quando `COUNT == COMPARE`, a CPU gera uma interrupção (grupo 0 do INTC). Escrever em COMPARE limpa o pedido. COMPARE = 0 desliga.

```c
ISR_ATTR static void compare_handler(void)     /* __attribute__((interrupt)): termina com rete */
{
    mtsr(SYSREG_COUNT, 0);                     /* recomeça a contagem */
    mtsr(SYSREG_COMPARE, ciclos_por_ms);       /* rearma (e limpa o pedido) */
    g_ms++;
}
intc_register(IRQ_GRP_COMPARE, compare_handler, 0);   /* grupo 0, nível 0 */
irq_enable();                                         /* limpa o bit GM do status */
```

### 3.2 Níveis de prioridade
O INTC do UC3 tem **4 níveis** (0 a 3, sendo 3 o mais urgente). Uma interrupção de nível maior **interrompe** uma de nível menor que esteja em andamento. Cada grupo de periféricos recebe um nível em `intc_register()`. Detalhes no módulo 02.

### 3.3 Atomicidade no AVR32
Ler um `uint32_t` alinhado é **uma** instrução no AVR32: `millis()` não precisa de proteção. Mas operações de ler-modificar-escrever (`x++` numa variável compartilhada) continuam precisando.

---

## 4. Multitarefa cooperativa
Com o `millis()`, várias "tarefas" rodam no mesmo laço sem bloquear umas às outras:
```c
for (;;) {
    uint32_t agora = millis();
    if (agora - t_led >= 250)  { t_led += 250;  led_toggle(); }
    if (agora - t_msg >= 1000) { t_msg += 1000; kprintf("uptime %lu s\n", ++seg); }
}
```
Sempre compare **diferenças** (`agora - t >= intervalo`): funciona mesmo quando o contador de 32 bits dá a volta (a cada ~49 dias).

O programa também mede quantas voltas um laço vazio dá em 100 ms: rodar com e sem o tick mostra quanto tempo de CPU a interrupção consome.

---

## Exercícios
1. Remova a proteção `cli()`/`SREG` do `millis()` do ATmega e imprima as leituras que "voltam no tempo" (compare cada leitura com a anterior).
2. Faça o Timer0 gerar interrupção a 10 kHz e meça o impacto no laço de medição.
3. Pisque o LED a 1 Hz **dentro** da ISR e compare com a versão no laço principal.
4. No UC3, registre uma segunda interrupção com nível maior e mostre que ela interrompe a de nível 0.
