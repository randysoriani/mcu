# Módulo 16 — EEPROM e memória não volátil

> **Objetivo deste módulo:** guardar dados que **sobrevivem ao desligamento** — configurações, calibrações, contadores. O ATmega328P tem uma EEPROM dedicada de 1 KB, típica dos AVR de 8 bits. O AT32UC3A não tem EEPROM e usa a **user page** da Flash.

Código:
- [`m328p/main.c`](m328p/main.c) — contador de boots na EEPROM, variável com valor inicial gravado pelo `.eep`, comandos `z` (zerar) e `d` (dump)
- [`uc3a/main.c`](uc3a/main.c) — contador de boots na user page

```sh
make MCU=m328p flash && picocom -b 115200 /dev/ttyACM0     # aperte RESET: o número aumenta
```

---

## 1. Memórias que não apagam

| Memória | Apaga ao desligar? | Escrita | Vida útil |
|---|---|---|---|
| RAM | **sim** | byte a byte, instantânea | ilimitada |
| **EEPROM** | não | **byte a byte**, ~3,4 ms por byte | ~**100 000** escritas por byte |
| Flash | não | por **página** (apagar uma página inteira antes) | ~10 000 ciclos |

A Flash guarda o programa; a EEPROM foi feita para **dados** pequenos que mudam de vez em quando. Analogia: a Flash é um livro impresso (reescrever exige reimprimir a página toda); a EEPROM é um caderno onde se apaga e reescreve uma palavra.

---

## 2. ATmega328P — EEPROM de 1 KB

### 2.1 Registradores
| Registrador | Endereço | Função |
|---|---|---|
| **EEARH:EEARL** | 0x42:0x41 | endereço (0 a 1023) |
| **EEDR** | 0x40 | dado |
| **EECR** | 0x3F | EEPM1:0 (5:4): modo (apagar+escrever, só apagar, só escrever); EERIE (3): interrupção "pronto"; **EEMPE** (2): habilitação mestre; **EEPE** (1): inicia a escrita; **EERE** (0): inicia a leitura |

### 2.2 Leitura
```c
while (EECR & (1 << EEPE)) { }     /* não lê durante uma escrita */
EEAR = endereco;
EECR |= (1 << EERE);               /* o dado aparece em EEDR imediatamente (CPU para 4 ciclos) */
return EEDR;
```

### 2.3 Escrita: a sequência protegida
Para evitar que um programa descontrolado apague a EEPROM, a escrita exige dois passos em sequência rápida:
```c
while (EECR & (1 << EEPE)) { }
EEAR = endereco;
EEDR = valor;
cli();                             /* uma interrupção aqui estragaria o tempo */
EECR |= (1 << EEMPE);              /* 1) "estou autorizando": vale por 4 ciclos */
EECR |= (1 << EEPE);               /* 2) escreve (~3,4 ms; a CPU continua rodando) */
sei();
```
O compilador com `-Os` gera exatamente duas instruções `sbi`, dentro do prazo. Com `-O0` às vezes não: por isso a avr-libc tem `eeprom_write_byte` em assembly.

### 2.4 Cuidados
- **Desgaste**: 100 000 escritas acabam rápido se você grava num laço. O exemplo **só escreve se o valor mudou** (`ee_read(addr) == v`). Para contadores frequentes, distribua as escritas por vários endereços (*wear leveling*).
- **Valor apagado = 0xFF**: uma EEPROM nunca escrita lê 0xFF em todos os bytes; o exemplo trata 0xFFFF como "zero boots".
- **Queda de energia durante a escrita**: o byte fica corrompido. Ligue o **detector de brown-out** (fuse BODLEVEL, módulo 04) e use checksum nos dados importantes.
- **Fuse EESAVE**: com ele programado, a EEPROM **não** é apagada quando você grava um programa novo por ISP.

### 2.5 Variáveis na seção `.eeprom`
```c
__attribute__((section(".eeprom"))) uint8_t ee_config[8] = { 'A','V','R','-','E','E',0,0 };
```
O linker dá a `ee_config` um endereço na EEPROM (módulo 03), e o valor inicial vai para o arquivo **`build/m328p/*.eep`**:
```
:080000004156522D4545000058      <- 8 bytes no endereço 0: "AVR-EE" em hexadecimal
:00000001FF
```
O programa **não** pode ler `ee_config[i]` diretamente (é um endereço da EEPROM, não da RAM): lê com `ee_read((uint16_t)ee_config + i)`.

Para gravar o `.eep` é preciso um programador ISP (o bootloader do Arduino, optiboot, não grava EEPROM):
```sh
make MCU=m328p flash-eeprom PROG=usbasp
```
Sem isso, o programa mostra bytes 0xFF (ou o que já houver lá).

---

## 3. AT32UC3A — user page da Flash

O UC3A tem uma página extra de Flash, a **user page** (512 bytes em `0x8080_0000`), separada do programa e **não apagada** pelo "chip erase" comum. É lida como memória normal e gravada pelo controlador **FLASHC**.

### 3.1 FLASHC (base `0xFFFE_1400`)
| Offset | Registrador | Função |
|---|---|---|
| 0x00 | **FCR** | FWS (6): wait state (módulo 04); interrupções |
| 0x04 | **FCMD** | **KEY** (31:24) = 0xA5; PAGEN (23:8): página; **CMD** (5:0) |
| 0x08 | **FSR** | **FRDY** (0): pronto; LOCKE (2): região travada; PROGE (3): erro de comando |

Comandos usados: `EP` (2) apagar página, `WP` (1) gravar página, `CPB` (3) limpar o page buffer, `EUP` (14) apagar a user page, `WUP` (13) gravar a user page.

### 3.2 O page buffer
A Flash do UC3 é gravada assim:
1. `CPB`: limpa o **page buffer** (uma área de 512 bytes no FLASHC).
2. Escrever words **nos endereços da própria página**: as escritas não vão para a Flash, ficam no buffer.
3. `WUP` (ou `WP`): grava o buffer de uma vez.

Como a página é apagada inteira, o exemplo copia antes os 512 bytes para a RAM, muda o que precisa e regrava tudo.

> As primeiras words da user page podem conter a configuração do **bootloader** da EVK1100 (por exemplo, a condição que faz o bootloader rodar). O exemplo preserva essas words e usa a word 64 em diante. Apagar a configuração pode impedir a entrada no bootloader.

Vida útil: ~100 000 ciclos por página (datasheet). A mesma estratégia de "só gravar se mudou" vale aqui.

---

## Exercícios
1. Guarde uma estrutura de configuração com um byte de checksum e detecte dados corrompidos.
2. Faça um contador de boots com *wear leveling*: 64 posições em rodízio.
3. Use a interrupção `EE_READY_vect` para gravar um buffer inteiro sem esperar.
4. No UC3, grave uma string na user page e leia-a por um ponteiro comum.
