# Módulo 24 — GDB do zero

O GDB é o depurador que transforma "o LED não pisca" em "a linha 42 lê `v[5]`, que não existe". Este módulo começa do primeiro comando e vai até analisar um HardFault e escrever comandos próprios, usando um programa com **bugs de propósito**.

Arquivos:

| Arquivo | Conteúdo |
|---|---|
| [`src/main.c`](src/main.c) | programa com 4 bugs e 4 falhas, escolhidos por uma letra na serial |
| [`src/fault.c`](src/fault.c) | `HardFault_Handler` que salva o contexto da falha em `fault_info` |
| [`gdb/curso.gdb`](gdb/curso.gdb) | comandos novos: `recarregar`, `reiniciar`, `nucleo`, `fault`, `gpio`, `pilha`, `semi` |
| [`Makefile`](Makefile) | compila com `-Og` e carrega `curso.gdb` em `make gdb`/`make debug` |

| Tecla | Cenário | Técnica |
|---|---|---|
| `m` | `media()` dá resultado absurdo | breakpoint, `step`, `print`, `finish` |
| `o` | `cfg.limite` muda "sozinho" | **watchpoint** |
| `p` | `processa()` erra em uma iteração | **breakpoint condicional** |
| `e` | máquina de estados | **`dprintf`**, `commands` |
| `c` | leitura em endereço inválido | análise de **BusFault** |
| `n` | chamada por ponteiro nulo | **UsageFault** (INVSTATE) |
| `z` | divisão por zero | **UsageFault** (DIVBYZERO, só M4) |
| `u` | acesso desalinhado | HardFault no M0; UsageFault no M4 com trap |

As saídas mostradas neste README são **reais**: foram obtidas com este programa compilado para F4 e executado no QEMU (máquina `netduinoplus2`, um STM32F405), com o `arm-none-eabi-gdb` 15.2. Na placa real os endereços podem diferir um pouco.

---

## 1. Como as peças se encaixam

```
 arm-none-eabi-gdb  ──"Remote Serial Protocol" (TCP 3333)──►  OpenOCD  ──SWD──►  chip
   lê o ELF:                                                    executa:
   símbolos, linhas,                                            parar, continuar,
   tipos (DWARF)                                                ler/escrever memória,
                                                                breakpoints de hardware
```

- O GDB nunca fala com o hardware: ele pede ao OpenOCD ("leia 4 bytes em 0x20000004", "coloque um breakpoint em 0x08000320").
- Tudo que o GDB sabe sobre **o seu código** (nomes, tipos, linhas) vem da informação de depuração **DWARF** do ELF, gerada por `-g`. Ela não vai para a Flash; por isso o GDB sempre recebe o `.elf`, nunca o `.bin`.
- **Use o mesmo ELF que está gravado.** Se você recompilar e não regravar, o GDB vai mostrar linhas que não correspondem ao código que está rodando.

### Por que `-Og`?
Com `-O2`, o compilador reordena instruções, mantém variáveis só em registradores, junta funções (inlining) e elimina variáveis. O GDB mostra então `<optimized out>` e "pula" linhas ao andar passo a passo. `-Og` otimiza sem atrapalhar a depuração. O `Makefile` deste módulo define `OPT := -Og`; em qualquer módulo você pode usar `make OPT=-Og` (ou `OPT=-O0`).

---

## 2. Primeira sessão, passo a passo

**Terminal 1** — servidor:
```sh
cd 24-gdb
make MCU=f4
make MCU=f4 openocd
```

**Terminal 2** — serial (para escolher os cenários):
```sh
picocom -b 115200 /dev/ttyACM0
```

**Terminal 3** — GDB, digitando tudo à mão na primeira vez:
```sh
arm-none-eabi-gdb build/f4/24-gdb.elf
```
```
(gdb) target extended-remote :3333       # conecta ao OpenOCD
(gdb) monitor reset halt                 # "monitor" repassa o comando ao OpenOCD
(gdb) load                               # grava o ELF na Flash
Loading section .isr_vector, size 0x1c4 lma 0x8000000
Loading section .text, size 0x... lma 0x80001c4
...
(gdb) monitor reset halt                 # começa do zero com o programa novo
(gdb) source gdb/curso.gdb               # comandos do curso
(gdb) break main
Breakpoint 1 at 0x80003e8: file src/main.c, line 128.
(gdb) continue
Breakpoint 1, main () at src/main.c:128
128         system_init();
```

O que aconteceu:
1. `target extended-remote` abriu a conexão. (`target remote` também funciona; `extended-remote` mantém a conexão quando o programa "termina" e permite `run`/`kill` repetidos.)
2. `monitor reset halt` parou o chip na primeira instrução do `Reset_Handler`.
3. `load` gravou as seções do ELF (o OpenOCD apaga e grava a Flash).
4. `break main` colocou um breakpoint; `continue` rodou o startup inteiro (cópia de `.data`, zeragem de `.bss`) até o `main`.

Daqui em diante, `make MCU=f4 gdb` faz os quatro primeiros passos (e carrega `curso.gdb`); `make MCU=f4 debug` sobe também o OpenOCD.

---

## 3. Andando pelo código

| Comando | Abrev. | O que faz |
|---|---|---|
| `continue` | `c` | roda até o próximo breakpoint |
| **Ctrl-C** | | interrompe o programa onde ele estiver |
| `next` | `n` | executa a linha, **sem entrar** em funções |
| `step` | `s` | executa a linha, **entrando** em funções |
| `finish` | `fin` | roda até a função atual retornar e mostra o valor retornado |
| `until` | `u` | roda até uma linha maior que a atual (sai de laços) |
| `advance 45` | | roda até a linha 45 |
| `stepi` / `nexti` | `si` / `ni` | uma **instrução** assembly |
| `list` | `l` | mostra o código em volta; `list media` mostra a função |
| `info line` | | endereço da linha atual |
| `where` / `bt` | | onde estou (pilha de chamadas) |

Pressionar **Enter** repete o último comando — ótimo para `n` e `s`.

---

## 4. Vendo dados

### `print` (`p`)
```
(gdb) p velocidade
$1 = 0
(gdb) p cfg
$2 = {
  nome = "motor\000\000",
  limite = 100,
  versao = 3
}
(gdb) p estado
$3 = ESTADO_PARADO                 <- o GDB conhece o enum
(gdb) p/x cfg.limite               <- /x hexadecimal
$4 = 0x64
(gdb) p/t 10                       <- /t binário   (também /d /u /o /c /a /f)
$5 = 1010
(gdb) p amostras                   <- arrays inteiros
$6 = {10, 20, 30, 40, 50}
(gdb) p *v@5                       <- "5 elementos a partir do ponteiro v"
(gdb) p sizeof(cfg)
(gdb) p &cfg.limite                <- endereços
(gdb) p $1 + 5                     <- resultados anteriores ficam em $1, $2...
```

### Tipos
```
(gdb) ptype cfg          # mostra a definição da struct
(gdb) whatis estado      # só o nome do tipo
(gdb) info locals        # variáveis locais da função atual
(gdb) info args          # argumentos
```

### `display`: imprimir a cada parada
```
(gdb) display soma
(gdb) display/x i
(gdb) undisplay 1
```

### `x`: examinar memória crua
```
x/<quantidade><formato><tamanho> endereço
   formato: x hex, d decimal, u, t binário, c char, s string, i instrução
   tamanho: b (8), h (16), w (32), g (64)
(gdb) x/8xw 0x20000000        # 8 words em hexa
(gdb) x/s cfg.nome            # como string
(gdb) x/4i $pc                # 4 instruções a partir do PC
(gdb) x/16xb &cfg             # bytes da struct (veja o layout na memória)
```

### Registradores
```
(gdb) info registers          # r0-r12, sp, lr, pc, xpsr, msp, psp, primask...
(gdb) p/x $pc
(gdb) p/x $sp
(gdb) p/x $xpsr & 0x1ff       # número da exceção ativa (0 = thread mode)
(gdb) info all-registers      # inclui s0-s31 e fpscr da FPU (F4)
```

### Periféricos
Como os headers do curso definem cada periférico como uma struct, o GDB entende os registradores pelo nome:
```
(gdb) p/x *GPIOA
$7 = {MODER = 0xa8000400, OTYPER = 0x0, OSPEEDR = 0xc000000, PUPDR = 0x64000000, IDR = 0xc020, ODR = 0x20, ...}
(gdb) p/t GPIOA->MODER
(gdb) p/x RCC->CFGR
(gdb) gpio GPIOA              # comando do curso.gdb
```
(Para isso funcionar, o símbolo `GPIOA` precisa existir: no curso ele é uma macro, e o `-g3` guarda as macros no ELF. Se aparecer `No symbol "GPIOA"`, use `p/x *(GPIO_TypeDef *)0x40020000`.)

---

## 5. Alterando o programa em execução

```
(gdb) set var velocidade = 42          # altera variável
(gdb) set var cfg.limite = 100
(gdb) set var GPIOA->ODR = 0x20        # escreve num registrador: acende o LED
(gdb) p media(amostras, 4)             # CHAMA uma função no alvo
(gdb) call passo_estado()
(gdb) jump 140                         # pula para a linha 140 (perigoso: pilha/registradores não se ajustam)
(gdb) return                           # sai da função atual agora (pede confirmação)
```

`print`/`call` de funções executam código de verdade no chip, com o estado atual — útil para testar uma função com outros argumentos sem recompilar.

---

## 6. Breakpoints

```
(gdb) break media                  # função
(gdb) break main.c:140             # arquivo:linha
(gdb) break *0x08000320            # endereço
(gdb) tbreak main                  # temporário: some depois da 1ª parada
(gdb) rbreak ^provoca_             # todas as funções que casam com a regex
(gdb) info breakpoints             # lista (Num, Enb, endereço, quantas vezes parou)
(gdb) disable 2 / enable 2 / delete 2 / delete
```

### Condições, contagem e ações
```
(gdb) break processa if i == 737          # só para quando a condição é verdadeira
(gdb) condition 3 i > 500                 # muda a condição do breakpoint 3
(gdb) ignore 3 10                         # ignora as próximas 10 paradas
(gdb) commands 3                          # comandos executados a cada parada
> silent
> printf "i=%u\n", i
> continue
> end
(gdb) dprintf passo_estado,"estado=%d vel=%d\n", estado, velocidade   # "printf dinâmico"
```
`dprintf` é um breakpoint que imprime e continua sozinho: um `printf` inserido sem recompilar.

### Limites de hardware
Na Flash não dá para inserir instruções `bkpt` (seria preciso apagar e regravar a página), então o OpenOCD usa os comparadores do **FPB** (*Flash Patch and Breakpoint*):

| Núcleo | Breakpoints de hardware | Watchpoints (DWT) |
|---|---|---|
| Cortex-M0 (F0) | **4** | **2** |
| Cortex-M4 (F4) | **6** | **4** |

Passando do limite: `Cannot insert hardware breakpoint`. Lembre que `step`/`next`/`finish` também usam breakpoints temporários. O comando `hbreak` força hardware explicitamente.

Breakpoints condicionais são avaliados **pelo GDB**: a CPU para em toda passagem, o GDB testa a condição e manda continuar. Num laço apertado isso deixa o programa milhares de vezes mais lento. Se precisar de velocidade, coloque o teste no próprio código (`if (i == 737) __asm("bkpt 0");`).

---

## 7. Watchpoints

Param o programa quando uma posição de memória é **escrita** (ou lida) — a ferramenta para "quem está mudando essa variável?".

```
(gdb) watch cfg.limite          # para quando for ESCRITA (e o valor mudar)
(gdb) rwatch contador           # quando for LIDA
(gdb) awatch contador           # leitura ou escrita
(gdb) watch -l v[i]             # observa o ENDEREÇO (não reavalia a expressão)
```

Watchpoints de hardware usam os comparadores do DWT (2 no M0, 4 no M4) e só cobrem regiões alinhadas de 1, 2, 4... bytes. Se o GDB disser "Software watchpoint", ele está executando passo a passo e conferindo — extremamente lento: reduza o tamanho da expressão.

---

## 8. Pilha de chamadas

```
(gdb) bt                 # backtrace
#0  copia_nome (dst=0x20000000 <cfg> "motor-pri", src=0x8000bb0 "motor-principal") at src/main.c:51
#1  0x080004a4 in main () at src/main.c:140
(gdb) frame 1            # "entra" no frame 1 para ver as variáveis do main
(gdb) up / down
(gdb) info frame         # endereço de retorno, registradores salvos
(gdb) pilha              # comando do curso: MSP, quanto da pilha está em uso
```

Dentro de um handler de exceção, o GDB entende o *exception frame* do Cortex-M e mostra `<signal handler called>` entre o handler e o código interrompido.

---

## 9. Caçando os bugs (exercícios guiados)

Use `make MCU=f4 debug` (ou `make gdb` com o OpenOCD já rodando), dê `continue` e escolha a letra na serial.

### 9.1 `m` — média absurda
```
(gdb) break media
(gdb) continue                       # tecle "m" no picocom
Breakpoint 1, media (v=v@entry=0x8000c60 <amostras>, n=n@entry=5) at src/main.c:42
42          for (uint32_t i = 0; i <= n; i++) {
(gdb) finish
Run till exit from #0  media (...)
Value returned is $1 = 373980365     <- esperado 30
```
Agora ande pelo laço:
```
(gdb) break media
(gdb) continue                       # tecle "m" de novo
(gdb) display i
(gdb) display soma
(gdb) next                           # Enter, Enter, Enter...
```
Quando `i` chega a 5, `v[5]` lê o que vier depois de `amostras` na Flash. Correção: `i < n`. (Confira com `p v[5]` e `x/8xw v`.)

### 9.2 `o` — variável que muda sozinha
```
(gdb) watch cfg.limite
Hardware watchpoint 2: cfg.limite
(gdb) continue                       # tecle "o"
Hardware watchpoint 2: cfg.limite

Old value = 100
New value = 105
0x08000272 in copia_nome (dst=0x20000000 <cfg> "motor-pri", src=0x8000bb0 "motor-principal") at src/main.c:51
51          while ((*dst++ = *src++) != '\0') { }
(gdb) bt
#0  0x08000272 in copia_nome (...) at src/main.c:51
#1  0x080004a4 in main () at src/main.c:140
(gdb) p cfg
$2 = {nome = "motor-pr", limite = 105, versao = 3}
```
O watchpoint pegou a primeira escrita fora de `nome[8]` (105 = 0x69 = `'i'` de "motor-principal"). A UART mostra o estrago final: `limite=1768124009`.

### 9.3 `p` — falha em uma iteração entre 1000
```
(gdb) break processa if i == 737
(gdb) continue                       # tecle "p"
Breakpoint 1, processa (i=i@entry=737) at src/main.c:56
(gdb) finish
Value returned is $2 = 0             <- deveria ser 737*3+1 = 2212
```
Ande com `step` pela função e veja qual condição desvia.

### 9.4 `e` — seguindo a máquina de estados sem parar
```
(gdb) dprintf passo_estado,"estado=%d velocidade=%d\n", estado, velocidade
(gdb) continue                       # tecle "e"
estado=0 velocidade=0
estado=1 velocidade=0
estado=1 velocidade=10
...
estado=2 velocidade=50
estado=3 velocidade=50
estado=3 velocidade=25
estado=0 velocidade=0
```
Alternativa com `commands`:
```
(gdb) break passo_estado
(gdb) commands
> silent
> p estado
> continue
> end
```

### 9.5 `c` — BusFault
```
(gdb) continue                       # tecle "c"; a UART mostra *** HardFault! e o LED pisca rápido
^C                                   # Ctrl-C
(gdb) fault
--- contexto salvo pelo HardFault_Handler (fault_info) ---
pc   = 0x0800027e   <- onde aconteceu
lr   = 0x080004f3   <- quem chamou
xpsr = 0x81000000   r0=0x00000063 r1=0xfffffff6 r2=0x08000430 r3=0x30000000
provoca_busfault + 4 in section .text
Line 79 of "src/main.c" starts at address 0x800027a <provoca_busfault> ...
--- registradores de falha (Cortex-M4) ---
CFSR = 0x00008200   HFSR = 0x40000000
  HFSR.FORCED   : falha configurável escalada para HardFault
  BFSR.PRECISERR: erro de barramento PRECISO em 0x30000000 (BFAR)
--- instruções em volta do pc ---
   0x800027a <provoca_busfault>:    mov.w  r3, #805306368    @ 0x30000000
   0x800027e <provoca_busfault+4>:  ldr    r3, [r3, #0]      <- a instrução que falhou
```

### 9.6 `n` — ponteiro nulo
O salto para o endereço 0 tem bit 0 = 0 (não é Thumb). No M4: `UFSR.INVSTATE`; o `pc` salvo é 0 e o `lr` aponta para quem fez a chamada (`info symbol fault_info.lr`).

### 9.7 `z` — divisão por zero (M4)
Com `SCB->CCR.DIV_0_TRP = 1`, `SDIV` com divisor 0 gera UsageFault. Resultado real: `CFSR = 0x02000000` → `UFSR.DIVBYZERO`. No Cortex-M0 não existe instrução de divisão: a libgcc devolve 0 e nada falha — rode no F0 e compare.

### 9.8 `u` — acesso desalinhado
No M0 qualquer `LDR` de 32 bits em endereço não múltiplo de 4 é HardFault. No M4 é permitido, a menos que `CCR.UNALIGN_TRP = 1` (o programa liga) → `UFSR.UNALIGNED`.

---

## 10. Entendendo um HardFault

Quando uma exceção acontece, o hardware empilha 8 words na pilha em uso:

```
SP + 0x00  r0
SP + 0x04  r1
SP + 0x08  r2
SP + 0x0C  r3
SP + 0x10  r12
SP + 0x14  lr     (de quem chamou a função que falhou)
SP + 0x18  pc     (a instrução que falhou, em falhas precisas)
SP + 0x1C  xpsr
```
e coloca em LR o valor **EXC_RETURN** (`0xFFFFFFF9` = voltar para thread mode usando MSP; `...FD` = PSP; `...E9/ED` = com contexto da FPU). O bit 2 diz qual pilha foi usada.

O handler de [`src/fault.c`](src/fault.c) é `naked` (o compilador não gera prólogo, que alteraria o SP), testa o bit 2 do LR, pega o MSP ou PSP e passa para uma função C que copia tudo em `fault_info`. O código usa só instruções que existem no Cortex-M0 (`movs`, `tst`, `ldr =`/`bx` em vez de `b` longo).

### Registradores de falha do Cortex-M4

| Registrador | Endereço | |
|---|---|---|
| `CFSR` | `0xE000ED28` | MMFSR [7:0] + BFSR [15:8] + UFSR [31:16] |
| `HFSR` | `0xE000ED2C` | bit 30 FORCED (falha configurável escalada), bit 1 VECTTBL |
| `MMFAR` | `0xE000ED34` | endereço da violação da MPU (se MMARVALID) |
| `BFAR` | `0xE000ED38` | endereço do erro de barramento (se BFARVALID) |

| Bit do CFSR | Nome | Causa típica |
|---|---|---|
| 0 | IACCVIOL | executar em região XN / MPU |
| 1 | DACCVIOL | acesso a dado proibido pela MPU |
| 8 | IBUSERR | buscar instrução em endereço inválido |
| 9 | PRECISERR | leitura/escrita em endereço inválido; `BFAR` tem o endereço |
| 10 | IMPRECISERR | escrita "bufferizada" em endereço inválido; o PC salvo é posterior |
| 15 | BFARVALID | `BFAR` válido |
| 16 | UNDEFINSTR | instrução indefinida (código corrompido, ponteiro de função lixo) |
| 17 | INVSTATE | salto para endereço com bit 0 = 0 |
| 18 | INVPC | EXC_RETURN inválido (pilha corrompida) |
| 19 | NOCP | FPU desligada (módulo 02) |
| 24 | UNALIGNED | desalinhado com UNALIGN_TRP |
| 25 | DIVBYZERO | divisão por 0 com DIV_0_TRP |

Por padrão MemManage/BusFault/UsageFault estão desabilitadas e **escalam para HardFault** (por isso `HFSR.FORCED`). Para ter handlers separados: `SCB->SHCSR |= (1<<16)|(1<<17)|(1<<18)`.

**Cortex-M0**: não há CFSR/HFSR/BFAR. Só sobra o PC empilhado — por isso o handler que salva o frame é ainda mais importante no F0.

Dica para falhas imprecisas: `SCB->ACTLR |= 2` (DISDEFWBUF, endereço `0xE000E008`) desliga o buffer de escrita no M4 e torna os BusFaults precisos (mais lento; só para depurar).

---

## 11. Assembly

```
(gdb) disassemble                 # função atual
(gdb) disassemble /s media        # intercalado com o fonte
(gdb) x/10i $pc
(gdb) stepi                       # uma instrução
(gdb) info registers r0 r1 r3
(gdb) display/i $pc               # mostra a próxima instrução a cada passo
```

---

## 12. Modo TUI (interface em tela cheia)

```
(gdb) layout src          # fonte em cima, comandos embaixo
(gdb) layout asm          # assembly
(gdb) layout split        # fonte + assembly
(gdb) layout regs         # + registradores
(gdb) focus cmd           # setas voltam a navegar no histórico
(gdb) refresh             # redesenha (Ctrl-L)
(gdb) tui disable         # volta ao modo texto   (atalho: Ctrl-X A)
```

---

## 13. Scripts e comandos próprios

### Arquivos de comandos
```sh
arm-none-eabi-gdb -x gdb/curso.gdb build/f4/24-gdb.elf      # executa ao iniciar
(gdb) source gdb/curso.gdb                                    # executa depois
arm-none-eabi-gdb -batch -x roteiro.gdb build/f4/24-gdb.elf  # sem interação (CI, testes)
```
`make MCU=f4 gdb-batch` é um exemplo: conecta, imprime `cfg`, `estado` e os registradores, solta o alvo e sai.

### `define` e `document`
```
define recarregar
    monitor reset halt
    load
    monitor reset halt
end
document recarregar
Grava o ELF de novo e deixa o alvo parado no reset.
end
```
Dentro de `define`: `$arg0`, `$arg1`... e `$argc`; variáveis de conveniência `set $x = ...`; `if`/`else`/`end`, `while`/`end`; `printf`. Veja o comando `fault` em [`gdb/curso.gdb`](gdb/curso.gdb), que decodifica os bits do CFSR.

### `.gdbinit`
O GDB lê `~/.gdbinit` e, se permitido, o `.gdbinit` da pasta atual. Por segurança, o arquivo local só é carregado se você autorizar em `~/.gdbinit`:
```
set auto-load safe-path /home/voce/Studies/mcu
```

### Salvando breakpoints
```
(gdb) save breakpoints bps.gdb
(gdb) source bps.gdb             # na próxima sessão
```

### Python
```
(gdb) python print(int(gdb.parse_and_eval("cfg.limite")) * 2)
(gdb) python
> for i in range(5):
>     print(i, gdb.parse_and_eval(f"amostras[{i}]"))
> end
```
Projetos prontos usam isso: **gdb-dashboard** (painéis com registradores, pilha e memória) e as extensões **PyCortexMDebug**/**svd-tools** (leem os arquivos SVD da ST e mostram cada registrador de periférico pelo nome dos campos).

---

## 14. Mais recursos úteis

| Comando | |
|---|---|
| `monitor <cmd>` | qualquer comando do OpenOCD (módulo 23): `monitor reset halt`, `monitor mdw 0x...`, `monitor gpio_dump A` |
| `compare-sections` | confere se a Flash bate com o ELF |
| `dump binary memory ram.bin 0x20000000 0x20020000` | salva a RAM inteira num arquivo |
| `restore ram.bin binary 0x20000000` | carrega de volta |
| `info symbol 0x080004f3` | qual função contém esse endereço |
| `info line *0x080004f3` | qual linha |
| `semi` (curso) / `monitor arm semihosting enable` | liga o semihosting |
| `set print pretty on` | structs em várias linhas |
| `shell make MCU=f4` | roda um comando do sistema sem sair |
| `make` | o GDB tem um comando `make` próprio: recompila sem sair |

Ciclo de trabalho rápido sem sair do GDB:
```
(gdb) make MCU=f4
(gdb) recarregar
(gdb) continue
```

---

## 15. Interfaces gráficas

Todas usam o mesmo GDB + OpenOCD por baixo:
- **VS Code + Cortex-Debug**: `launch.json` com `"servertype": "openocd"`, `"configFiles": ["interface/stlink.cfg", "target/stm32f4x.cfg"]`, `"executable": "build/f4/24-gdb.elf"`, `"svdFile"` para ver periféricos;
- **gdbgui** (navegador), **Emacs** (`M-x gdb`), **Neovim** (nvim-dap), **Qt Creator**.

Saber os comandos de texto continua valendo: as interfaces gráficas mostram o mesmo e, quando algo falha, a mensagem que aparece vem do GDB.

---

## 16. Problemas comuns

| Sintoma | Causa / solução |
|---|---|
| `Remote connection closed` / `Connection refused` | OpenOCD não está rodando ou caiu (veja o terminal dele) |
| `<optimized out>` | compile com `OPT=-Og` ou `-O0`; ou declare a variável `volatile` temporariamente |
| linhas "pulando" ao dar `next` | otimização; ou o ELF não é o mesmo que está gravado (`load` de novo) |
| `Cannot insert hardware breakpoint` | passou do limite (4/6): `delete` alguns |
| `Cannot access memory at address` | endereço inválido, ou periférico sem clock (no TM4C gera falha) |
| breakpoint nunca para | a função foi "inlinada" (`-O2`) ou o código não passa por ali; `info breakpoints` mostra o endereço |
| `warning: ... no debugging symbols` | ELF sem `-g`, ou passou o `.bin` |
| `Software watchpoint` (muito lento) | expressão grande demais ou não alinhada; observe um campo menor |
| programa trava em `bkpt` sem GDB | semihosting ou `__asm("bkpt")` esquecido no código |

---

## Resumo (cola)

```
target extended-remote :3333   monitor reset halt   load   source gdb/curso.gdb
b func | b arq.c:N | b *ADDR | tbreak | b F if cond | dprintf F,"fmt",args | info b | d N
c | n | s | fin | until | adv N | si | ni | Ctrl-C | bt | frame N | up | down
p var | p/x | p *ptr@n | ptype | info locals | display | x/8xw ADDR | x/4i $pc
info reg | p/x $pc | p/x $xpsr & 0x1ff | set var X = V | call f(args)
watch X | rwatch | awatch | watch -l
layout src | layout split | tui disable
fault | gpio GPIOA | pilha | recarregar | reiniciar | semi
```

---

## Exercícios

1. Corrija os quatro bugs (`m`, `o`, `p`) e confirme no GDB com breakpoints e watchpoints.
2. Escreva um comando `define leds` que mostre o `ODR` do GPIOA em binário e diga se o LED está aceso.
3. Habilite handlers separados (`SHCSR`) e escreva `BusFault_Handler` e `UsageFault_Handler`; confira no GDB qual é chamado em cada cenário.
4. Use `commands` num breakpoint em `SysTick_Handler` com `ignore` para imprimir `millis()` a cada 1000 ticks sem parar o programa.
5. Faça um roteiro `-batch` que grave o programa, coloque um watchpoint em `cfg.limite`, mande continuar e imprima o backtrace quando ele disparar — um teste de regressão automático.
6. Instale o gdb-dashboard e o arquivo SVD do seu chip, e mostre os campos do `RCC->CFGR` pelo nome.
