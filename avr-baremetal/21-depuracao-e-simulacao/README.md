# Módulo 21 — Depuração e simulação: QEMU, GDB, debugWIRE e JTAG

> **Objetivo deste módulo:** parar o programa no meio, olhar variáveis e registradores e descobrir **onde** um bug acontece, em vez de encher o código de `kprintf`. O ATmega328P pode ser depurado **sem nenhuma placa**, no QEMU; o hardware real usa o **debugWIRE** (ATmega) ou o **JTAG** (UC3A).

Código: [`src/main.c`](src/main.c) — um programa com **três bugs de propósito**, e [`gdb/sessao.gdb`](gdb/sessao.gdb), o roteiro usado abaixo.

| Tecla | Função | Sintoma | Ferramenta do GDB |
|---|---|---|---|
| `m` | `media()` de {10,20,30,40,50} | resultado 2548 em vez de 30 | `break`, `finish` |
| `o` | `copia_nome()` | `cfg.limite` muda "sozinha" | `watch` |
| `p` | soma de `processa(i)` para i = 0..999 | soma errada | breakpoint condicional |

---

## 1. Conceitos

| Termo | Significado |
|---|---|
| **depurador** (GDB) | programa no PC que controla a execução de outro programa |
| **breakpoint** | "pare quando chegar nesta linha" |
| **watchpoint** | "pare quando esta variável mudar" |
| **step / next** | executar uma linha (entrando ou não nas funções) |
| **backtrace** | a pilha de chamadas: quem chamou quem até aqui |
| **stub / servidor GDB** | o lado que realmente para o chip (o QEMU, o avarice, o OpenOCD...) e conversa com o GDB pela rede (porta 1234) |
| **informação de depuração** (`-g`) | tabela no ELF que liga endereços a linhas do C e nomes de variáveis; o build do curso sempre usa `-g` (ela não vai para o chip) |

```
 ┌──────── PC ────────┐        TCP :1234       ┌──────────────────────────┐
 │ avr-gdb + .elf     │◄──────────────────────►│ QEMU (Uno emulado)       │
 └────────────────────┘                        │ ou avarice + debugWIRE   │
                                               └──────────────────────────┘
```

### Otimização atrapalha
Com `-Os`, o compilador reordena, junta e elimina variáveis: o GDB mostra `<optimized out>` e pula linhas. Para depurar, compile com **`OPT=-Og`** (otimização amigável à depuração) ou `-O0`.

---

## 2. ATmega328P no QEMU (sem placa)

### 2.1 Instalação
```sh
sudo pacman -S avr-gdb qemu-system-avr          # Arch
sudo apt install gdb-avr qemu-system-misc       # Debian/Ubuntu
```
O `gdb` de algumas distribuições já vem com suporte a todas as arquiteturas (teste: `gdb -batch -ex "set architecture avr"`); nesse caso use `make gdb GDB=gdb`.

### 2.2 Dois terminais
```sh
# terminal 1: compila e inicia o QEMU PARADO, esperando o GDB (-s -S)
make clean && make MCU=m328p EXTRA=-DQEMU OPT=-Og
make MCU=m328p qemu-gdb

# terminal 2: o GDB
make MCU=m328p gdb                     # = avr-gdb -q -ex "target remote :1234" build/m328p/...elf
```
O terminal 1 é a serial: é lá que você digita `m`, `o`, `p`. Para sair do QEMU: `Ctrl-A` e depois `X`.

### 2.3 A sessão (saída real)
Executando o roteiro [`gdb/sessao.gdb`](gdb/sessao.gdb) e digitando `m`, `o`, `p` na serial. Os avisos "Target-supplied registers are not supported" do GDB com o QEMU AVR podem ser ignorados.

**Conectando** — o chip está parado na primeira instrução, a tabela de vetores (módulo 02):
```
(gdb) target remote :1234
__vectors () at .../common/m328p/startup.S:17
17	    jmp __init                  ;  0 RESET
```

**Bug 1 — `media()`:**
```
(gdb) break media
Breakpoint 1 at 0xb4: file src/main.c, line 24.
(gdb) continue                       <- digite 'm' na serial
Breakpoint 1, media (v=0x8001b5 <amostras>, n=n@entry=5 '\005') at src/main.c:24
(gdb) bt
#0  media (v=0x8001b5 <amostras>, n=n@entry=5 '\005') at src/main.c:24
#1  0x00000132 in main () at src/main.c:49
(gdb) finish
Value returned is $1 = 2548
```
`finish` roda até a função retornar e mostra o valor. Com `next` dentro da função e `print i`, `print soma`, você veria o laço rodar **6 vezes** (`i <= n`): o sexto elemento lê o que estiver depois do vetor na memória. Repare também no endereço `0x8001b5`: no ELF, a RAM do AVR soma 0x800000 (módulo 03).

**Bug 2 — quem altera `cfg.limite`?**
```
(gdb) set can-use-hw-watchpoints 0
(gdb) watch cfg.limite
(gdb) continue                       <- digite 'o'
Watchpoint 2: cfg.limite
Old value = 100
New value = 112
0x000000aa in copia_nome (dst=0x800107 <cfg+7> "", src=0x800172 "rincipal") at src/main.c:32
32	    while ((*dst++ = *src++) != '\0') { }
(gdb) bt 2
#0  0x000000aa in copia_nome (...) at src/main.c:32
#1  0x00000170 in main () at src/main.c:51
(gdb) print cfg
$2 = {nome = "motor-", limite = 112}
```
O GDB parou **na instrução exata** que mudou a variável: `copia_nome` passou do fim de `nome[6]` e escreveu o `'p'` (0x70 = 112) de "motor-principal" no byte baixo de `limite`. O QEMU AVR não tem watchpoints de hardware: com `can-use-hw-watchpoints 0`, o GDB executa passo a passo e compara — lento, mas funciona.

**Bug 3 — em qual iteração `processa()` falha?** Um breakpoint simples pararia 1000 vezes; um **condicional** para só quando a condição é verdadeira:
```
(gdb) break processa if (i & 0x2E1) == 0x2E1
(gdb) continue                       <- digite 'p'
Breakpoint 3, processa (i=737) at src/main.c:37
(gdb) info registers r24 r25 SREG SP pc
r24            0xe1                225
r25            0x2                 2
SREG           0xf5                245
SP             0x8fb               0x8008fb
pc             0x72                0xe4 <processa>
(gdb) x/8xb &cfg
0x800100 <cfg>:	0x6d	0x6f	0x74	0x6f	0x72	0x2d	0x70	0x72
```
Observe, ligando aos módulos anteriores:
- o argumento `i` = 737 = 0x02E1 chega em **r25:r24** (convenção do avr-gcc: primeiro argumento em r24/r25);
- o **pc** vale `0x72` (contado em words) e o GDB mostra `0xe4` (bytes) — módulo 01;
- o **SP** está perto do fim da RAM (0x8FF), pois a pilha desce;
- o `x/8xb` mostra "motor-pr" sobre a estrutura: o estrago do bug 2.

### 2.4 Comandos essenciais
| Comando | Abreviação | Faz |
|---|---|---|
| `break arquivo.c:linha` / `break função` | `b` | breakpoint |
| `break f if cond` | | breakpoint condicional |
| `continue` | `c` | continua |
| `next` / `step` | `n` / `s` | uma linha (pula / entra em funções) |
| `finish` | | até a função retornar |
| `print expr` / `print/x expr` | `p` | mostra valor (em hexa) |
| `display expr` | | mostra a cada parada |
| `watch var` | | para quando a variável muda |
| `x/16xb endereço` | | examina memória (16 bytes em hexa) |
| `info registers` | `i r` | registradores |
| `bt` | | pilha de chamadas |
| `layout src` / `layout asm` | | modo TUI: código na tela (`Ctrl-X A` sai) |
| `stepi` / `x/5i $pc` | `si` | uma instrução de máquina / desmonta |

Registradores de periféricos são só memória (no ELF, `0x800000 + endereço`):
```
(gdb) x/1xb 0x800025          # PORTB
(gdb) set {char}0x800025 = 0x20   # liga o PB5
```

### 2.5 Limites do QEMU AVR
Emula CPU, GPIO, USART e os timers de 16 bits (Timer1). **Não** emula Timer0/Timer2, ADC, TWI, SPI, EEPROM, comparador, watchdog nem sleep. Por isso alguns módulos usam `-DQEMU` (a base de tempo passa ao Timer1) e outros não rodam nele. Ótimo para lógica, startup, memória e serial.

---

## 3. ATmega328P em hardware: debugWIRE

O ATmega328P tem o **debugWIRE**: depuração por **um único fio**, o pino RESET.

Precisa de:
- uma ferramenta compatível: Atmel-ICE, AVR Dragon, MPLAB Snap/PICkit 4, ou projetos abertos como o **dw-link** (um Arduino vira depurador) — use com **avarice** ou o servidor do próprio projeto, que expõem a porta GDB;
- programar o fuse **DWEN** (hfuse bit 6) por ISP.

Atenções:
- No Uno, o capacitor de 100 nF do circuito de **auto-reset** impede o debugWIRE: corte o jumper "RESET EN" da placa (e solde de volta para usar o bootloader).
- Com o debugWIRE ligado, o pino RESET deixa de ser reset e **o ISP para de funcionar**. Para voltar, a ferramenta envia um comando pelo debugWIRE que desliga temporariamente o modo; então desprograme o DWEN por ISP. Não esqueça disso antes de guardar a placa.
- Breakpoints são feitos regravando a Flash (instrução `BREAK`): cada um gasta ciclos de gravação.

---

## 4. AT32UC3A: JTAG e Nexus

O UC3A tem **JTAG** completo com a interface de depuração **Nexus/OCD** (*On-Chip Debug*): breakpoints e watchpoints de hardware, leitura de memória com o chip rodando e trace.

- Ferramentas: **JTAGICE mkII**, **AVR ONE!**, Atmel-ICE (no modo AVR32 JTAG).
- Software clássico: `avr32gdbproxy` (servidor GDB) e `avr32-gdb`, do antigo pacote "AVR32 GNU Toolchain"/"AVR32 Studio":
  ```sh
  avr32gdbproxy -t jtagicemkii -a :4711 &
  avr32-gdb -ex "target remote :4711" build/uc3a/21-depuracao-e-simulacao.elf
  ```
  O OpenOCD tem suporte experimental a AVR32 (`target avr32_ap7k`), limitado a outra família; para o UC3, use as ferramentas da Atmel.
- Conector JTAG de 10 pinos na EVK1100.
- Gravando por JTAG, o programa começa em `0x8000_0000` pelo trampolim (módulo 02), e o bootloader DFU pode ser apagado.

Sem ferramenta JTAG, a depuração do UC3 fica com `kprintf`, LEDs e as tabelas de exceção do startup (cada exceção é um laço próprio: o endereço onde o programa travou diz qual foi).

---

## Exercícios
1. Corrija os três bugs e confirme com o GDB.
2. Use `display soma` e `next` no laço de `media()` e veja a sexta iteração.
3. Coloque um breakpoint em `__vector_18` (recepção da serial do módulo 07) e veja o byte em `UDR0` (`x/1xb 0x8000c6`).
4. No QEMU, use `layout asm` e `stepi` para seguir o startup desde `__vectors` até o `main` (módulo 02).
