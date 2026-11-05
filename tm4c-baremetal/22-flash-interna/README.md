# Módulo 22 — Flash interna como memória não volátil

Código: [`src/main.c`](src/main.c) — log de contador de boots no último bloco da Flash, com wear leveling simples (só apaga quando enche). `e` apaga, `w` grava +1.

| | TM4C123 | TM4C129 |
|---|---|---|
| Flash | 256 KB | 1 MB |
| Unidade de apagamento | **bloco de 1 KB** | **setor de 16 KB** |
| Área do exemplo | `0x0003_FC00` | `0x000F_C000` |
| Chave (WRKEY) | `0xA442` (BOOTCFG.KEY = 1, padrão) | valor de `FLPEKEY` |
| Ciclos de apagamento | 100 000 (típ.) | 100 000 (típ.) |
| EEPROM de verdade | 2 KB | 6 KB |

```sh
make MCU=123 flash && picocom -b 115200 /dev/ttyACM0
# aperte RESET várias vezes: o número do boot aumenta e sobrevive ao desligar
```

---

## 1. Registradores (base `0x400F_D000`)

| Offset | Reg. | |
|---|---|---|
| 0x000 | `FMA` | endereço (alinhado à word para gravar; qualquer endereço do bloco para apagar) |
| 0x004 | `FMD` | dado a gravar (32 bits) |
| 0x008 | **`FMC`** | [31:16] **WRKEY**, 0 WRITE, 1 ERASE, 2 MERASE (apaga tudo), 3 COMT |
| 0x00C | `FCRIS` | 0 ARIS (erro de acesso/proteção), 1 PRIS (concluído) |
| 0x014 | `FCMISC` | escreve 1 → limpa |
| 0x020 | `FMC2` | gravação em buffer (32 words de uma vez) |
| 0x100+ | `FWBn` | buffer de escrita para o FMC2 |
| 0x03C | `FLPEKEY` | (129) chave atual |
| 0xFC0 | `FLASHPP` | tamanho |

## 2. Apagar
```c
FLASH_FCMISC = 3;
FLASH_FMA = 0x3FC00;
FLASH_FMC = 0xA4420000 | FLASH_FMC_ERASE;     /* chave + comando numa única escrita */
while (FLASH_FMC & FLASH_FMC_ERASE) { }        /* o bit volta a 0 ao terminar */
if (FLASH_FCRIS & FLASH_FCRIS_ARIS) erro;
```

## 3. Gravar uma word
```c
FLASH_FMA = endereço;
FLASH_FMD = valor;
FLASH_FMC = 0xA4420000 | FLASH_FMC_WRITE;
while (FLASH_FMC & FLASH_FMC_WRITE) { }
```
Após apagar, todos os bits valem 1; gravar só transforma 1 em 0. Regravar a mesma word sem apagar só é permitido se apenas zerar bits (e mesmo assim a TI limita o número de regravações).

Sem a chave correta no mesmo acesso que o comando, a escrita no `FMC` é ignorada.

## 4. A CPU durante a operação
O chip tem um único banco: enquanto a Flash apaga (~ms no 123, mais no setor de 16 KB do 129), qualquer busca de instrução na Flash **para a CPU** até o fim. Interrupções também atrasam (a tabela de vetores está na Flash). Para continuar executando (ex.: manter um protocolo de comunicação), rode a rotina de apagar/gravar e os handlers críticos da RAM (`.ramfunc`, módulo 03).

## 5. Proteção
- `FMPREn`/`FMPPEn`: proteção de leitura/execução e de gravação por bloco de 2 KB (123) / 16 KB (129). Uma vez gravados com `COMT`, só um "unlock" de debug (que apaga tudo) desfaz.
- Gravar o próprio código que está rodando é possível, mas é exatamente como se faz um bootloader — e como se "tijola" uma placa.

## 6. Reservando a área no linker
Para garantir que o código nunca invada a área de dados:
```ld
FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 255K     /* 123: deixa o último 1 KB */
```

## 7. EEPROM
Para configurações pequenas, prefira a **EEPROM** interna (2 KB / 6 KB, 500 mil ciclos por palavra, gravação por word sem apagar bloco): `SYSCTL_RCGCEEPROM`, registradores `EEBLOCK`, `EEOFFSET`, `EERDWR` em `0x400A_F000`. É o próximo exercício natural deste módulo.

---

## Exercícios
1. Use o `FMC2` + `FWBn` para gravar 32 words de uma vez e compare o tempo.
2. Reserve a área no linker script e confira no `.map`.
3. Implemente o mesmo contador com a EEPROM interna.
