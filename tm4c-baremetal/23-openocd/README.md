# Módulo 23 — OpenOCD do zero (com o ICDI da TI)

> **Objetivo deste módulo:** deixar de usar o OpenOCD como caixa preta (`make flash`) e usá-lo como ferramenta: entender como ele conversa com o chip, ler e escrever a memória **com o programa rodando**, gravar e recuperar a placa, escrever configurações e comandos próprios em TCL e usar **semihosting** (imprimir mensagens pelo cabo de depuração).

Arquivos:

| Arquivo | Conteúdo |
|---|---|
| [`src/main.c`](src/main.c) | programa-alvo com variáveis feitas para serem lidas e escritas pelo depurador |
| [`openocd/curso.cfg`](openocd/curso.cfg) | configuração própria: placa, eventos, portas |
| [`openocd/curso.tcl`](openocd/curso.tcl) | comandos novos: `rd`, `wr`, `chip_info`, `gpio_dump`, `led`, `clocks`, `fault_info`, `gravar`, `semi` |
| [`Makefile`](Makefile) | alvos extras: `telnet`, `semihost`, `symbols`, `ocd-info` |

```sh
cd 23-openocd
make MCU=123              # compila
make MCU=123 flash        # grava (usando openocd/curso.cfg)
make MCU=123 openocd      # terminal 1: servidor
make telnet               # terminal 2: console
```

---

## 1. O que o OpenOCD faz

```
 você ──► telnet :4444 ─┐
 GDB  ──► TCP    :3333 ─┼─► OpenOCD ──USB──► ICDI ──JTAG/SWD──► porta de depuração do chip ──► núcleo, RAM, Flash, periféricos
 script ► TCL    :6666 ─┘   (no PC)        (na placa)
```

O OpenOCD (*Open On-Chip Debugger*) é um programa que roda no PC e conversa com o **ICDI**, o gravador/depurador embutido na LaunchPad. O ICDI, por sua vez, fala com a porta de depuração do chip alvo pelos fios JTAG/SWD. A partir daí o OpenOCD consegue: parar e continuar o processador, ler e escrever qualquer endereço de memória, gravar a Flash e servir de ponte para o GDB.

Por dentro, ele é organizado em camadas, e cada uma corresponde a um pedaço dos arquivos de configuração:

| Camada | O que é | No ICDI |
|---|---|---|
| **Adapter driver** | fala com o hardware do depurador por USB | driver `hla` com o layout `ti-icdi` (`interface/ti-icdi.cfg`) |
| **Transporte** | protocolo até o chip | `hla_jtag`: o ICDI recebe comandos de alto nível ("leia 4 bytes em X") e cuida do JTAG |
| **Target** | o núcleo Cortex-M: parar, continuar, breakpoints | `target/stellaris.cfg` cria o target `$_TARGETNAME` |
| **Flash driver** | sabe apagar e gravar a Flash daquele chip | `stellaris` (o nome da família antiga da TI) |
| **Servidores** | GDB (3333), telnet (4444), TCL (6666) | iguais para qualquer placa |

Os arquivos `board/ek-tm4c123gxl.cfg` e `board/ek-tm4c1294xl.cfg`, que vêm com o OpenOCD, juntam tudo isso.

---

## 2. Instalação e onde ficam os scripts

```sh
sudo pacman -S openocd          # Arch;  Debian: sudo apt install openocd
openocd --version               # o curso assume 0.12 ou mais novo
ls /usr/share/openocd/scripts/board/ek-tm4c*
cat /usr/share/openocd/scripts/board/ek-tm4c123gxl.cfg
```

O comando `find` do OpenOCD procura nesses diretórios (e nos passados com `-s`); por isso escrevemos `source [find board/ek-tm4c123gxl.cfg]` sem o caminho completo.

Alternativa só para gravar: `lm4flash build/123/x.bin` (pacote `lm4tools`).

---

## 3. Primeira execução, passo a passo

Com a LaunchPad ligada no USB "DEBUG":

```sh
openocd -f board/ek-tm4c123gxl.cfg       # ou board/ek-tm4c1294xl.cfg
```

Saída típica (varia com a versão):
```
Open On-Chip Debugger 0.12.0
Info : The selected transport took over low-level target control. ...
Info : clock speed ... kHz
Info : ICDI Firmware version: 9270                       <- o ICDI respondeu
Info : [tm4c123gh6pm.cpu] Cortex-M4 r0p1 processor detected   <- achou o núcleo!
Info : [tm4c123gh6pm.cpu] target has 6 breakpoints, 4 watchpoints
Info : starting gdb server for tm4c123gh6pm.cpu on 3333
Info : Listening on port 3333 for gdb connections
Info : Listening on port 4444 for telnet connections
```
- **processor detected**: a comunicação com o chip funcionou. Se falhar aqui, veja a seção 11.
- **breakpoints/watchpoints**: quantos pontos de parada de hardware o núcleo tem (módulo 24).

O servidor fica rodando; **Ctrl-C** encerra. Com a configuração do curso:
```sh
openocd -s openocd -c "set MCU 123" -f openocd/curso.cfg     # = make MCU=123 openocd
```

---

## 4. A linha de comando

```
openocd [-s dir] [-f arquivo.cfg]... [-c "comando"]... [-d nivel] [-l log.txt]
```

| Opção | Significado |
|---|---|
| `-f arq` | executa um arquivo de configuração (pode repetir) |
| `-c "cmd"` | executa um comando; vários no mesmo `-c` separados por `;` |
| `-s dir` | acrescenta um diretório de busca para o `find` |
| `-d3` | log detalhado (para problemas de conexão) |
| `-l arq` | log em arquivo |

Os `-f` e `-c` são executados **na ordem**. Até o comando `init` o OpenOCD só configura; `init` conecta no hardware (e é chamado sozinho no fim, se você não chamar).

**Modo batch** (executa e sai), que é o que o `make flash` faz:
```sh
openocd -f board/ek-tm4c123gxl.cfg -c "program build/123/23-openocd.elf verify reset exit"
openocd -s openocd -c "set MCU 123" -f openocd/curso.cfg -c "init; chip_info; exit"
```

---

## 5. O console (telnet)

Com o servidor rodando, em outro terminal:
```sh
telnet localhost 4444          # ou: nc localhost 4444   (ou: make telnet)
```

### Controle de execução
| Comando | O que faz |
|---|---|
| `halt` | para a CPU (mostra o PC e o motivo) |
| `resume` | continua |
| `step` | executa uma instrução |
| `reset run` | reset e roda |
| `reset halt` | reset e para na primeira instrução (`Reset_Handler`) |
| `poll` | estado atual (rodando/parado) |
| `targets` | lista os targets |

### Registradores do núcleo (com a CPU parada)
```
> halt
> reg                     # todos
> reg pc                  # um só
```

### Memória
| Comando | |
|---|---|
| `mdw addr [n]` | lê n words (32 bits) |
| `mdh` / `mdb` | 16 / 8 bits |
| `mww addr valor` | escreve uma word |
| `mwh` / `mwb` | 16 / 8 bits |
| `read_memory addr 32 n` / `write_memory addr 32 {v1 v2}` | versões TCL (retornam listas) |

> Ler e escrever memória **sem parar a CPU** é um recurso do Cortex-M. Com o ICDI, o suporte depende da versão do firmware do ICDI e do OpenOCD. Se `mdw` com o programa rodando der erro, use `halt`, leia/escreva, e `resume`.

### Breakpoints e watchpoints de hardware
```
> bp 0x000002cc 2 hw        # para quando executar este endereço
> rbp 0x000002cc            # remove
> wp 0x20000018 4 w         # para quando alguém ESCREVER neste endereço
> rwp 0x20000018
```
(O GDB faz isso de forma mais confortável, com os nomes das variáveis — módulo 24.)

`exit` fecha o telnet; `shutdown` encerra o servidor.

---

## 6. Prática com o programa do módulo

O programa pisca o LED e expõe quatro variáveis para o depurador:

| Variável | Para que serve |
|---|---|
| `periodo_ms` | ritmo do LED (escreva outro valor e veja mudar) |
| `contador` | quantas vezes o LED já piscou |
| `comando` | escreva 1 (inverte o LED), 2 (mensagem por semihosting) ou 3 (reset) |
| `usar_semihosting` | 1 = mensagens pelo semihosting (seção 10) |

**1. Descubra os endereços** (o programa também os imprime na serial):
```sh
make MCU=123 symbols
```

**2. Leia o contador várias vezes** e veja ele subir:
```
> mdw <endereço de contador>
```

**3. Mude o ritmo do LED:**
```
> mww <endereço de periodo_ms> 50       # rápido
> mww <endereço de periodo_ms> 1000     # devagar
```

**4. Mande um comando ao programa:**
```
> mww <endereço de comando> 1           # inverte o LED e responde na serial
```

**5. Mexa no hardware diretamente**, sem o programa saber. No 123 o LED verde é PF3; o endereço mascarado do pino 3 da porta F é `0x4002_5000 + (0x08 << 2)` = `0x4002_5020`:
```
> mww 0x40025020 0xFF       # acende
> mww 0x40025020 0x00       # apaga
```
No 129 (LED D1 = PN1): `0x4006_4000 + (0x02 << 2)` = `0x4006_4008`.

**6. Use os comandos do curso** (`curso.tcl`):
```
> ajuda_curso
> chip_info          # DID0/DID1, tamanho da Flash e da RAM
> gpio_dump F        # registradores da porta F
> led on
> clocks             # quais periféricos estão com clock ligado
```

---

## 7. Gravação da Flash

`program` é um atalho:
```
program arquivo.elf [verify] [reset] [exit]
   = init; reset halt; flash write_image erase arquivo.elf; verify_image ...; reset run; shutdown
```

| Comando | |
|---|---|
| `flash banks` | lista os bancos de Flash |
| `flash info 0` | blocos e proteção do banco 0 |
| `flash erase_sector 0 0 last` | apaga tudo |
| `flash write_image erase arq.elf` | apaga o necessário e grava |
| `flash write_image erase arq.bin 0` | `.bin` precisa do endereço (0 no TM4C) |
| `verify_image arq.elf` | compara a Flash com o arquivo |
| `dump_image copia.bin 0 0x40000` | lê a Flash para um arquivo |

---

## 8. Placa "travada": recuperação

Se o programa gravado reconfigura os pinos JTAG (PC0–PC3), entra em hibernação logo no início ou deixa o clock num estado inválido, o ICDI pode não conseguir conectar. Opções, da mais simples à mais radical:

1. **Segurar o botão RESET**, iniciar o OpenOCD (ou `make flash`) e soltar logo em seguida.
2. **Recuperar o chip** (apaga a Flash inteira e restaura os padrões de fábrica):
   ```sh
   openocd -f board/ek-tm4c123gxl.cfg -c "init; stellaris recover; exit"
   ```
   Depois, desligue e religue a placa.

---

## 9. Escrevendo a sua configuração: `curso.cfg`

Um `.cfg` é um **script TCL**. O do curso:

```tcl
if {![info exists MCU]} { set MCU 123 }          ;# parâmetro vindo de -c "set MCU 129"
if {$MCU eq "129"} {
    source [find board/ek-tm4c1294xl.cfg]        ;# adaptador + transporte + alvo
} else {
    source [find board/ek-tm4c123gxl.cfg]
}
$_TARGETNAME configure -event gdb-attach {       ;# evento: quando o GDB conectar...
    echo "GDB conectou"
    halt                                         ;# ...para o processador
}
source [file join [file dirname [info script]] curso.tcl]   ;# comandos próprios
```

### TCL em 5 minutos
| Construção | Exemplo |
|---|---|
| variável | `set x 10`; uso: `$x` |
| executar um comando e usar o resultado | `[rd 0x20000000]` |
| conta | `expr {$x * 2 + 1}` |
| texto sem substituição | `{...}` |
| condição | `if {$x > 3} { ... } else { ... }` |
| laço | `foreach {nome off} {DIR 0x400 DEN 0x51C} { ... }` |
| procedimento (função) | `proc dobro {v} { return [expr {$v * 2}] }` |
| formatar | `format "0x%08x" $v` |

### Eventos úteis
| Evento | Quando |
|---|---|
| `reset-start`, `reset-end` | início/fim de um reset |
| `halted`, `resumed` | CPU parou / voltou |
| `gdb-attach`, `gdb-detach` | GDB conectou / desconectou |

### Comandos próprios (`curso.tcl`)
```tcl
proc rd {addr} {
    return [format 0x%08x [lindex [read_memory $addr 32 1] 0]]
}
proc chip_class {} {                      ;# 5 = TM4C123, 10 = TM4C129
    return [expr {([rd 0x400FE000] >> 16) & 0xFF}]
}
```
Qualquer `proc` vira um comando do console, pode ser chamado por `-c` e também pelo GDB (`monitor gpio_dump F`).

---

## 10. Semihosting: mensagens pelo depurador

Semihosting permite ao programa pedir serviços ao PC (imprimir, ler um arquivo...) **pelo próprio cabo de depuração**, sem usar a serial.

Como funciona:
1. O programa coloca o número da operação em `r0` (`0x04` = "imprima esta string") e o endereço da string em `r1`.
2. Executa a instrução `bkpt 0xAB`, que para o processador.
3. O OpenOCD percebe que é um pedido de semihosting, lê a string da memória do chip, imprime no **terminal do OpenOCD** e manda o processador continuar.

```c
static void semi_puts(const char *s)
{
    register uint32_t    r0 __asm("r0") = 0x04;
    register const char *r1 __asm("r1") = s;
    __asm volatile ("bkpt 0xAB" : "+r"(r0) : "r"(r1) : "memory");
}
```

Passo a passo:
```sh
make MCU=123 semihost        # = openocd ... -c "init; arm semihosting enable; reset run"
```
Em outro terminal (`make telnet`), ligue a variável e mande o comando 2:
```
> mww <endereço de usar_semihosting> 1
> mww <endereço de comando> 2
```
No terminal do OpenOCD aparece `[alvo] ola pelo semihosting!` (e, a cada 10 piscadas, `[alvo] mais 10 piscadas`).

Cuidados:
- cada chamada **para o processador por vários milissegundos**: bom para depurar, ruim para tempo real;
- **sem depurador conectado, `bkpt` gera um HardFault** — por isso o programa só usa semihosting quando `usar_semihosting = 1`.

---

## 11. Problemas comuns

| Sintoma | Causa provável / solução |
|---|---|
| `LIBUSB_ERROR_ACCESS` | regra udev para `1cbe:00fd` (módulo 00) |
| `open failed` / ICDI não encontrado | cabo no conector USB errado (use o "DEBUG"); chave/jumper de alimentação; outro programa usando o ICDI |
| `Error connecting DP` / não detecta o processador | placa travada → seção 8 |
| `couldn't bind ... port 3333/4444/6666` | já há um OpenOCD rodando (`pkill openocd`) |
| `invalid command name "read_memory"` | OpenOCD antigo (< 0.12): atualize |
| programa trava ao usar semihosting | faltou `arm semihosting enable` no OpenOCD |

---

## Exercícios

1. Escreva um procedimento TCL `sysctl_dump` que mostre `RCC`/`RCC2` (123) ou `RSCLKCFG`/`PLLFREQ0`/`MEMTIM0` (129) e confira a configuração de clock do módulo 04.
2. Faça um script batch que grava o programa, espera 2 s, lê o `contador` e imprime "OK" se ele for maior que 3 — um teste automático de placa.
3. Use `wp` para parar quando `comando` for escrito, e descubra quem escreveu.
4. Acrescente ao `curso.cfg` um evento `reset-end` que acende o LED com `led on`.
