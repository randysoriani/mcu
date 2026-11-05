# Módulo 20 — RTC no módulo de Hibernação

O TM4C não tem um RTC com calendário BCD como o STM32. O relógio fica no **Hibernation Module (HIB)**: um **contador de segundos de 32 bits** (`RTCC`) mais sub-segundos (`RTCSS`, 1/32768 s), alimentado pelo cristal de **32,768 kHz** e pelo **VBAT**. Data e hora são calculadas por software a partir de um *epoch* — aqui, segundos desde 1970-01-01 (o "tempo Unix").

O TM4C129 tem também um modo calendário em hardware (`HIBCAL0/1`), mas o curso usa o contador, que é igual nos dois.

Código: [`src/main.c`](src/main.c)
- 1ª execução: liga o oscilador, acerta o RTC com a hora da **compilação** e grava um magic em `HIB->DATA[0]`
- resets seguintes: só lê (o HIB tem domínio de energia próprio)
- conversão epoch ↔ data (algoritmo de Howard Hinnant)
- alarme por comparação (`RTCM0`) todo minuto no segundo 30
- `r` na serial reacerta

```sh
make MCU=123 flash && picocom -b 115200 /dev/ttyACM0
```

---

## 1. Registradores (base `0x400F_C000`)

| Offset | Reg. | |
|---|---|---|
| 0x000 | `RTCC` | contador de segundos (r) |
| 0x004 | `RTCM0` | valor de comparação (alarme) |
| 0x00C | `RTCLD` | escrever = carrega o contador (acerta o relógio) |
| 0x010 | `CTL` | 0 **RTCEN**, 1 HIBREQ, 3 RTCWEN (acorda no alarme), 4 PINWEN, 6 **CLK32EN**, 8 VDD3ON, 31 **WRC** |
| 0x014 | `IM` | 0 RTCALT0, 2 LOWBAT, 3 EXTW, 4 WC |
| 0x018/0x01C | `RIS`/`MIS` | |
| 0x020 | `IC` | escreve 1 → limpa |
| 0x024 | `RTCT` | trim: corrige o cristal (± ppm) |
| 0x028 | `RTCSS` | [14:0] sub-segundos |
| 0x030–0x06C | `DATA[16]` | 64 bytes que sobrevivem a resets e à hibernação |

## 2. A regra do WRC
O HIB roda no domínio lento de 32 kHz. **Toda escrita** precisa esperar `CTL.WRC = 1` antes e depois:
```c
static void hib_write(volatile uint32_t *reg, uint32_t v)
{
    while (!(HIB->CTL & HIB_CTL_WRC)) { }
    *reg = v;
    while (!(HIB->CTL & HIB_CTL_WRC)) { }
}
```
Ignorar isso faz escritas sumirem silenciosamente.

## 3. Sequência
```c
periph_enable(&SYSCTL_RCGCHIB, &SYSCTL_PRHIB, 0);
if (!(HIB->CTL & CLK32EN)) {
    hib_write(&HIB->CTL, CLK32EN);          /* liga o oscilador */
    delay_ms(1500);                         /* estabilização do cristal */
}
hib_write(&HIB->CTL, HIB->CTL | RTCEN);
hib_write(&HIB->RTCLD, epoch);              /* acerta */
```

## 4. Epoch ↔ calendário
```c
dias = days_from_civil(2026, 11, 4);       /* dias desde 1970-01-01 */
epoch = dias*86400 + h*3600 + m*60 + s;

civil_from_days(epoch / 86400, &y, &m, &d);
r = epoch % 86400;  h = r/3600; min = (r/60)%60; s = r%60;
```
Vantagens do contador: diferenças de tempo são uma subtração; fusos e horário de verão ficam na camada de software. Desvantagem: o contador de 32 bits sem sinal estoura em 2106.

## 5. Alarme
`RTCM0 = instante desejado` → quando `RTCC == RTCM0`, `RIS.RTCALT0 = 1` (e acorda da hibernação se `RTCWEN`, módulo 21).

## 6. Na LaunchPad
- 123: cristal Y1 de 32,768 kHz; VBAT ligado ao 3,3 V (não há bateria: o RTC zera quando a placa é desligada).
- 129: cristal Y3; VBAT também ligado ao 3,3 V.
Para manter a hora sem USB, ligue uma bateria CR2032 ao VBAT (removendo a ligação ao 3,3 V conforme o manual da placa).

---

## Exercícios
1. Comandos pela serial para acertar a hora (`set 2026-11-04 21:40:00`).
2. Calibre o cristal com o `RTCT` comparando com o relógio do PC por 24 h.
3. No 129, use o modo calendário em hardware (`HIBCALCTL`, `HIBCAL0/1`) e compare.
