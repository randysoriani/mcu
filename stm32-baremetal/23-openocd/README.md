# Módulo 23 — OpenOCD do zero

Até aqui o OpenOCD foi uma caixa preta chamada por `make flash`. Neste módulo ele vira ferramenta: entender como ele conversa com o chip, usar o console para ler e escrever memória **com o programa rodando**, gravar e proteger a Flash, recuperar uma placa travada, escrever arquivos de configuração e comandos próprios em TCL e usar **semihosting**.

Arquivos:

| Arquivo | Conteúdo |
|---|---|
| [`src/main.c`](src/main.c) | programa-alvo com variáveis feitas para serem lidas/escritas pelo debugger |
| [`openocd/curso.cfg`](openocd/curso.cfg) | configuração própria: adaptador, alvo, eventos, portas |
| [`openocd/curso.tcl`](openocd/curso.tcl) | comandos novos: `rd`, `wr`, `chip_info`, `gpio_dump`, `led`, `fault_info`, `gravar`, `semi` |
| [`Makefile`](Makefile) | alvos extras: `telnet`, `semihost`, `symbols`, `ocd-info` |

```sh
cd 23-openocd
make MCU=f4                # compila
make MCU=f4 flash          # grava (agora usando openocd/curso.cfg)
make MCU=f4 openocd        # terminal 1: servidor
make telnet                # terminal 2: console
```

---

## 1. O que o OpenOCD faz

```
 você ──► telnet :4444 ─┐
 GDB  ──► TCP    :3333 ─┼─► OpenOCD ──USB──► ST-LINK ──SWD (SWDIO/SWCLK)──► DAP do chip ──► núcleo, RAM, Flash, periféricos
 script ► TCL    :6666 ─┘   (no PC)        (na Nucleo)                       (CoreSight)
```

Por dentro, o OpenOCD é organizado em camadas, e cada uma aparece num pedaço dos arquivos de configuração:

| Camada | O que é | Onde é configurada |
|---|---|---|
| **Adapter driver** | fala com o hardware do debugger por USB (ST-LINK, J-Link, CMSIS-DAP, FTDI...) | `interface/stlink.cfg` → `adapter driver hla` ou `st-link` |
| **Transporte** | protocolo elétrico até o chip: JTAG, SWD, ou "HLA" (o adaptador entende comandos de alto nível) | `transport select ...` |
| **DAP / AP** | *Debug Access Port* do ARM CoreSight: o bloco dentro do chip que dá acesso ao barramento | `dap create` nos scripts de target |
| **Target** | o núcleo (cortex_m): parar, continuar, breakpoints, registradores | `target create $_TARGETNAME cortex_m ...` |
| **Flash driver** | sabe apagar/gravar a Flash daquele chip (`stm32f1x` serve ao F0; `stm32f2x` ao F4) | `flash bank ...` |
| **Servidores** | GDB (3333), telnet (4444), TCL (6666) | `gdb_port`, `telnet_port`, `tcl_port` |

O ponto-chave: o acesso à memória pelo DAP **não precisa parar a CPU**. O debugger consegue ler e escrever RAM e registradores de periféricos enquanto o programa roda — é isso que este módulo explora.

---

## 2. Instalação e onde ficam os scripts

```sh
sudo pacman -S openocd          # Arch;  Debian: sudo apt install openocd
openocd --version               # o curso assume 0.12 ou mais novo
```

Os scripts que vêm com o OpenOCD ficam em `/usr/share/openocd/scripts/` (ou `/usr/local/share/...`):

```sh
ls /usr/share/openocd/scripts/
#   board/  chip/  cpu/  interface/  target/  ...
cat /usr/share/openocd/scripts/interface/stlink.cfg
cat /usr/share/openocd/scripts/target/stm32f4x.cfg     # vale a leitura: TCL de verdade
```

O comando `find` do OpenOCD procura nesses diretórios (e nos passados com `-s`), por isso escrevemos `source [find target/stm32f4x.cfg]` sem caminho completo.

---

## 3. Primeira execução, passo a passo

Com a Nucleo ligada no USB:

```sh
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
```

Saída típica (os detalhes variam com a versão e o firmware do ST-LINK):

```
Open On-Chip Debugger 0.12.0
Info : auto-selecting first available session transport "hla_swd". ...   <- transporte escolhido
Info : The selected transport took over low-level target control. ...
Info : clock speed 2000 kHz                                              <- velocidade do SWD
Info : STLINK V2J40M27 (API v2) VID:PID 0483:374B                        <- firmware do ST-LINK
Info : Target voltage: 3.256868                                          <- o chip está alimentado
Info : [stm32f4x.cpu] Cortex-M4 r0p1 processor detected                  <- achou o núcleo!
Info : [stm32f4x.cpu] target has 6 breakpoints, 4 watchpoints            <- recursos de hardware
Info : starting gdb server for stm32f4x.cpu on 3333
Info : Listening on port 3333 for gdb connections
Info : Listening on port 4444 for telnet connections
```

Leitura linha a linha:
- **transport**: com o ST-LINK, o OpenOCD usa "HLA" (*high level adapter*): manda ao ST-LINK comandos como "leia 4 bytes em X", e o firmware do ST-LINK faz o SWD.
- **Target voltage ~3,3 V**: se aparecer 0 V, a placa não está alimentada ou o cabo é só de carga.
- **processor detected**: a comunicação com o DAP funcionou. Se falhar aqui, veja a seção 11.
- **breakpoints/watchpoints**: quantidade de comparadores de hardware (FPB e DWT). No F0 (Cortex-M0): 4 breakpoints e 2 watchpoints.

O servidor fica rodando. **Ctrl-C** encerra.

Agora com a configuração do curso:

```sh
openocd -s openocd -c "set MCU f4" -f openocd/curso.cfg
# é o que "make MCU=f4 openocd" executa
```

---

## 4. A linha de comando

```
openocd [-s dir] [-f arquivo.cfg]... [-c "comando"]... [-d nivel] [-l log.txt]
```

| Opção | Significado |
|---|---|
| `-f arq` | executa um arquivo de configuração (pode repetir) |
| `-c "cmd"` | executa um comando (pode repetir). Vários comandos no mesmo `-c` separados por `;` |
| `-s dir` | acrescenta um diretório de busca para `find` |
| `-d0`..`-d4` | nível de log (3 = debug; útil para problemas de conexão) |
| `-l arq` | log em arquivo |

Os `-f` e `-c` são executados **na ordem**. Até o comando `init`, o OpenOCD está em "fase de configuração" (pode declarar adaptador, target, flash); `init` conecta no hardware. Se você não chamar `init`, ele é chamado sozinho depois do último `-f/-c`.

**Modo batch** (executa e sai), o que o `make flash` faz:
```sh
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
        -c "program build/f4/23-openocd.elf verify reset exit"
openocd -s openocd -c "set MCU f4" -f openocd/curso.cfg -c "init; chip_info; exit"
```

---

## 5. O console (telnet) — comandos essenciais

Com o servidor rodando (`make MCU=f4 openocd`), em outro terminal:

```sh
telnet localhost 4444          # ou: nc localhost 4444   (ou: make telnet)
```
```
Open On-Chip Debugger
>
```

### Controle de execução
| Comando | O que faz |
|---|---|
| `halt` | para a CPU (mostra PC e o motivo) |
| `resume` / `resume 0x08000200` | continua (opcionalmente a partir de um endereço) |
| `step` | executa uma instrução |
| `reset run` | reset e roda |
| `reset halt` | reset e para na primeira instrução (`Reset_Handler`) |
| `reset init` | reset + o evento `reset-init` do target (configura clock para gravar mais rápido) |
| `poll` | estado atual (running/halted) |
| `targets` | lista os targets e o estado |
| `wait_halt [ms]` | espera parar |

### Registradores do núcleo (com a CPU parada)
```
> halt
> reg                     # todos
> reg pc                  # um só
> reg r0 0x1234           # altera
```

### Memória — funciona com a CPU RODANDO
| Comando | |
|---|---|
| `mdw addr [n]` | lê n words (32 bits) |
| `mdh` / `mdb` | 16 / 8 bits |
| `mww addr valor [n]` | escreve word (n vezes) |
| `mwh` / `mwb` | 16 / 8 bits |
| `read_memory addr 32 n` / `write_memory addr 32 {v1 v2}` | versões TCL (retornam listas) |

### Breakpoints e watchpoints de hardware
```
> bp 0x080001d4 2 hw        # breakpoint no endereço (2 = tamanho da instrução Thumb)
> rbp 0x080001d4            # remove
> wp 0x20000018 4 w         # para quando alguém ESCREVER no contador
> rwp 0x20000018
```
(O GDB faz isso de forma bem mais confortável, com símbolos — módulo 24.)

### Encerrar
`exit` fecha o telnet; `shutdown` encerra o servidor.

---

## 6. Prática com o programa do módulo

**1. Descubra os endereços** (o programa também os imprime na UART):
```sh
make MCU=f4 symbols
# 20000000 D periodo_ms
# 20000010 B usar_semihosting
# 20000014 B comando
# 20000018 B contador
```
(Os endereços podem mudar se você alterar o código; confira sempre.)

**2. Leia o contador com o programa rodando:**
```
> mdw 0x20000018
0x20000018: 0000002a
> mdw 0x20000018
0x20000018: 0000002d          <- subiu: o programa não parou
```

**3. Mude o ritmo do LED escrevendo na RAM:**
```
> mww 0x20000000 50           # periodo_ms = 50 -> pisca rápido
> mww 0x20000000 1000         # devagar
```

**4. Mande um comando ao programa:**
```
> mww 0x20000014 1            # comando = 1: inverte o LED e responde na UART
```

**5. Mexa no hardware direto, sem o programa saber.** O LED é PA5; o `BSRR` do GPIOA do F4 fica em `0x40020018` (F0: `0x48000018`):
```
> mww 0x40020018 0x00000020   # bit 5: acende
> mww 0x40020018 0x00200000   # bit 21: apaga
> mdw 0x40020000 10           # MODER, OTYPER, ..., ODR do GPIOA
```

**6. Use os comandos do curso** (definidos em `curso.tcl`):
```
> ajuda_curso
> chip_info
CPUID  = 0x410fc241 (Cortex-M4)
IDCODE = 0x10006421 (DEV_ID 0x421)
Flash  = 512 KB
> gpio_dump A
> led on
> led off
```

**7. Pare e examine:**
```
> halt
[stm32f4x.cpu] halted due to debug-request, current mode: Thread
xPSR: 0x01000000 pc: 0x08000362 msp: 0x2001ffd8
> reg pc
> resume
```
(Como o programa passa a maior parte do tempo em `__WFI()`, o PC costuma estar logo depois dele.)

---

## 7. Gravação da Flash

`program` é um atalho que faz várias coisas:
```
program arquivo.elf [verify] [reset] [exit] [offset]
   = init; reset halt; flash write_image erase arquivo.elf; verify_image ...; reset run; shutdown
```

Os comandos individuais:
| Comando | |
|---|---|
| `flash banks` | lista os bancos (endereço, tamanho, driver) |
| `flash info 0` | setores e proteção do banco 0 |
| `flash probe 0` | identifica o tamanho real |
| `flash erase_sector 0 0 last` | apaga tudo |
| `flash erase_address 0x08060000 0x20000` | apaga um intervalo |
| `flash write_image erase arq.elf` | apaga o necessário e grava |
| `flash write_image erase arq.bin 0x08000000` | `.bin` precisa do endereço |
| `verify_image arq.elf` | compara a Flash com o arquivo |
| `dump_image copia.bin 0x08000000 0x10000` | lê a Flash para um arquivo |
| `load_image arq.bin 0x20000000` | carrega na RAM (sem gravar a Flash) |

> `.elf` já contém os endereços de cada seção; `.bin` é só uma sequência de bytes, então é preciso dizer onde ela começa.

### Proteção de leitura (RDP) — cuidado
Os STM32 têm *option bytes* que protegem a Flash contra leitura pelo debugger (nível 1) ou permanentemente (nível 2, **irreversível**). Comandos:
```
> stm32f4x options_read 0        # F4
> stm32f1x options_read 0        # F0 usa o driver stm32f1x
> stm32f4x unlock 0              # RDP 1 -> 0: APAGA TODA A FLASH
> stm32f1x unlock 0
```
Depois de `unlock`, faça um power-cycle. Nunca experimente o nível 2.

---

## 8. Reset e "connect under reset"

Se o programa desliga os pinos SWD (PA13/PA14), entra em Stop/Standby logo no início, ou trava o clock, o OpenOCD não consegue conectar com o chip rodando. A saída é segurar o chip **em reset** durante a conexão (a Nucleo liga o NRST do alvo ao ST-LINK):

```sh
openocd -f interface/stlink.cfg \
        -c "reset_config srst_only srst_nogate connect_assert_srst" \
        -f target/stm32f4x.cfg \
        -c "init; reset halt; flash erase_sector 0 0 last; exit"
```

| `reset_config` | Significado |
|---|---|
| `none` | sem pino de reset: usa SYSRESETREQ (software) |
| `srst_only` | há o pino de reset do sistema (NRST) |
| `srst_nogate` | o debug continua funcionando com o reset ativo |
| `connect_assert_srst` | segura o reset enquanto conecta |

Alternativa: segurar o botão RESET da Nucleo, iniciar o OpenOCD e soltar logo depois.

---

## 9. Escrevendo a sua configuração: `curso.cfg`

Um `.cfg` é um **script TCL**. O do curso, comentado em [`openocd/curso.cfg`](openocd/curso.cfg):

```tcl
if {![info exists MCU]} { set MCU f4 }          ;# parâmetro vindo de -c "set MCU f0"
source [find interface/stlink.cfg]              ;# 1. adaptador
adapter speed 4000                              ;# 2. velocidade (kHz)
if {$MCU eq "f0"} {                             ;# 3. alvo
    source [find target/stm32f0x.cfg]
} else {
    source [find target/stm32f4x.cfg]
}
$_TARGETNAME configure -event gdb-attach {      ;# 4. eventos
    echo "GDB conectou"
    halt
}
source [file join [file dirname [info script]] curso.tcl]   ;# 5. comandos próprios
```

### TCL em 5 minutos
| Construção | Exemplo |
|---|---|
| variável | `set x 10` ; uso: `$x` |
| substituição de comando | `[rd 0x20000000]` (executa e usa o resultado) |
| expressão | `expr {$x * 2 + 1}` |
| string literal sem substituição | `{...}` |
| condição | `if {$x > 3} { ... } else { ... }` |
| laços | `foreach {nome off} {A 0 B 4} { ... }`, `for {set i 0} {$i < 4} {incr i} { ... }` |
| procedimento | `proc dobro {v} { return [expr {$v * 2}] }` |
| listas | `lindex $l 0`, `llength $l`, `lsearch $l X` |
| formatação | `format "0x%08x" $v` |

### Eventos úteis
| Evento | Quando |
|---|---|
| `reset-start`, `reset-end` | início/fim de um reset |
| `reset-init` | depois de `reset init` (o target do F4 sobe o clock aqui para gravar mais rápido) |
| `halted`, `resumed` | CPU parou / voltou |
| `gdb-attach`, `gdb-detach` | GDB conectou / desconectou |
| `examine-end` | o núcleo foi identificado |

### Comandos próprios: `curso.tcl`
```tcl
proc rd {addr} {
    return [format 0x%08x [lindex [read_memory $addr 32 1] 0]]
}
proc gpio_dump {porta} {
    set b [gpio_base $porta]
    foreach {nome off} {MODER 0x00 OTYPER 0x04 ... ODR 0x14} {
        echo [format "GPIO%s_%-8s = %s" $porta $nome [rd [expr {$b + $off}]]]
    }
}
```
Qualquer `proc` vira um comando do console e pode ser chamado por `-c`, pelo GDB (`monitor gpio_dump A`) ou pela porta TCL.

---

## 10. Semihosting: printf pelo debugger

Semihosting permite ao programa pedir serviços ao PC (imprimir, ler arquivo, ler o relógio) **pelo próprio cabo de debug**, sem UART.

Como funciona:
1. O programa coloca o número da operação em `r0` (`0x04` = SYS_WRITE0, "imprime string") e o argumento em `r1`.
2. Executa `bkpt 0xAB`.
3. A CPU para (debug halt). O OpenOCD vê o `0xAB`, lê a string da RAM do alvo, imprime no **terminal do OpenOCD** e manda a CPU continuar.

```c
static void semi_puts(const char *s)
{
    register uint32_t    r0 __asm("r0") = 0x04;
    register const char *r1 __asm("r1") = s;
    __asm volatile ("bkpt 0xAB" : "+r"(r0) : "r"(r1) : "memory");
}
```

Passo a passo com o programa do módulo:
```sh
make MCU=f4 semihost        # = openocd ... -c "init; arm semihosting enable; reset run"
```
Em outro terminal (`make telnet`):
```
> mww 0x20000010 1          # usar_semihosting = 1
> mww 0x20000014 2          # comando 2: manda uma mensagem
```
No terminal do OpenOCD aparece `[alvo] ola pelo semihosting!` e, a cada 10 piscadas, `[alvo] mais 10 piscadas`.

Prós e contras:
- não precisa de UART nem de fios extras;
- **cada chamada para a CPU por vários milissegundos** (bom para depurar, ruim para tempo real);
- **sem debugger, `bkpt` gera HardFault** — por isso o programa só usa semihosting quando `usar_semihosting = 1`. Em produção, retire.

Também dá para ligar pelo GDB: `monitor arm semihosting enable` (módulo 24).

---

## 11. A porta TCL (6666): automatizando pelo Python

A porta 6666 aceita comandos terminados pelo byte `0x1a` e responde da mesma forma — ideal para scripts de teste de produção:

```python
import socket
s = socket.create_connection(("localhost", 6666))
def ocd(cmd):
    s.sendall(cmd.encode() + b"\x1a")
    data = b""
    while not data.endswith(b"\x1a"):
        data += s.recv(4096)
    return data[:-1].decode()
print(ocd("capture {mdw 0x20000018}"))     # lê o contador
ocd("mww 0x20000000 100")                   # muda periodo_ms
```
(`capture` devolve a saída do comando como texto.)

---

## 12. SWO/ITM (avançado, só F4)

O Cortex-M4 tem um canal de trace de 1 fio (SWO, pino PB3 no F446) por onde o programa pode mandar texto rapidamente pelo ITM, sem parar a CPU. O ST-LINK/V2-1 da Nucleo consegue capturá-lo. Na versão 0.12 do OpenOCD:
```
> stm32f4x.tpiu configure -protocol uart -traceclk 180000000 -pin-freq 2000000 -output saida.txt
> stm32f4x.tpiu enable
> itm port 0 on
```
O programa precisa habilitar o ITM (`DEMCR.TRCENA`, `ITM->TER`) e escrever em `ITM->PORT[0]`. Não foi testado neste curso; o Cortex-M0 não tem SWO.

---

## 13. Outros adaptadores

Basta trocar o arquivo de interface; o resto continua igual:
| Adaptador | Arquivo |
|---|---|
| ST-LINK (Nucleo, Discovery, clones) | `interface/stlink.cfg` |
| CMSIS-DAP (DAPLink, Raspberry Pi Pico "debugprobe") | `interface/cmsis-dap.cfg` |
| SEGGER J-Link | `interface/jlink.cfg` |
| FTDI FT2232 | `interface/ftdi/...` |

Com várias placas ligadas: `adapter serial <número>` escolhe uma (o número aparece em `lsusb -v` ou no log do OpenOCD).

---

## 14. Problemas comuns

| Sintoma | Causa provável / solução |
|---|---|
| `LIBUSB_ERROR_ACCESS` | regra udev (módulo 00) |
| `open failed` / adaptador não encontrado | cabo só de carga; outro processo (outro OpenOCD, STM32CubeIDE) usando o ST-LINK |
| `Target voltage: 0.0` | placa sem alimentação (jumper de alimentação, JP5 na Nucleo) |
| `init mode failed (unable to connect to the target)` | chip em low-power/SWD desativado → "connect under reset" (seção 8) |
| `Error: address+size wrapped` / falha ao gravar | proteção RDP ativa → `stm32fXx unlock 0` |
| `Error: couldn't bind tcl to socket on port 6666` | já há um OpenOCD rodando (`pkill openocd`) |
| `invalid command name "read_memory"` | OpenOCD antigo (< 0.12): atualize ou use `mem2array` |
| o programa "trava" ao ligar semihosting no código | sem `arm semihosting enable` o `bkpt` vira HardFault |

---

## Exercícios

1. Escreva um procedimento TCL `rcc_dump` que mostre os registradores de clock (F4: `RCC->CR`, `PLLCFGR`, `CFGR`, `AHB1ENR`, `APB1ENR`, `APB2ENR`) e confira a configuração de 180 MHz do módulo 04.
2. Faça um script batch que grava o programa, espera 2 s, lê o `contador` e imprime "OK" se ele for maior que 3 (um teste automático de placa).
3. Use `wp` no telnet para parar quando `comando` for escrito, e descubra o PC de quem escreveu.
4. Acrescente ao `curso.cfg` um evento `reset-end` que acende o LED via `led on`.
5. Implemente no programa a operação de semihosting `SYS_CLOCK` (0x10) e imprima o tempo em centésimos de segundo pela UART.
