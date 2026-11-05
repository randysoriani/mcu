# Módulo 17 — I2S por software (Timer + uDMA → GPIO)

> **A família TM4C não tem periférico I2S.** Este módulo mostra como gerar um fluxo I2S válido com o que existe: um **timer** disparando o **uDMA**, que copia uma sequência pré-calculada para o registrador **DATA mascarado** de uma porta GPIO.
>
> **Estado: técnica didática não validada em hardware neste curso.** Compila para os dois chips; os pontos a conferir na bancada estão na seção 6.

Código: [`src/main.c`](src/main.c)

| | BCLK | WS (LRCLK) | SD (DIN) |
|---|---|---|---|
| 123 | PE1 | PE2 | PE3 |
| 129 | PK0 | PK1 | PK2 |

DAC sugerido: **MAX98357A** (amplificador I2S, liga direto num alto-falante; aceita 8–96 kHz) ou **PCM5102A** (pino SCK ao GND para o PLL interno usar o BCLK).

```sh
make MCU=123 flash
```

---

## 1. Protocolo I2S (Philips)

```
WS   ‾‾‾‾\______________________________/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾\___
BCLK _/‾\_/‾\_/‾\_/‾\ ... _/‾\_/‾\_/‾\_/‾\_/‾\ ...
SD    R0 |L15|L14|L13| ... | L0|R15|R14| ...
          ^ MSB do canal esquerdo 1 BCLK depois de WS cair
```
- WS = 0 → canal esquerdo, WS = 1 → direito; muda **um BCLK antes** do MSB.
- Dado muda na **descida** do BCLK e é lido na **subida**.
- 16 bits × 2 canais = 32 BCLK por quadro → BCLK = 32 × Fs.

## 2. A ideia

Cada bit I2S é gerado com **duas escritas** no GPIO:
1. `[BCLK=0, WS, SD=bit]` — dado estável antes da subida
2. `[BCLK=1, WS, SD=bit]` — DAC amostra

Um quadro = 64 escritas. Escrevendo em `GPIOx->DATA_BITS[máscara]`, só os 3 pinos de áudio mudam — atômico, sem afetar o resto da porta.

### Números
| | |
|---|---|
| Fs | **15 625 Hz** |
| Escritas por segundo | 64 × 15 625 = **1 000 000** |
| TIMER0 | 1 MHz: `TAILR = SYSCLK/1e6 − 1` (79 no 123, 119 no 129 — divisão exata) |
| Tom | 16 amostras por período → 976,6 Hz |
| Buffer | 16 quadros × 64 = **1024 bytes** = uma transferência uDMA (máximo) |

Fs foi escolhida para que o TIMER divida o SYSCLK exatamente nos dois chips; 48 kHz exigiria 3,07 M escritas/s — possível no limite do uDMA, mas o barramento ficaria saturado.

## 3. Pré-cálculo do padrão
```c
for (slot = 0; slot < 32; slot++) {
    word = slot < 16 ? L : R;
    bit  = (word >> (15 - slot % 16)) & 1;
    ws   = ((slot + 1) % 32) >= 16;          /* WS adiantado 1 bit */
    pattern[k++] = (ws << B_WS) | (bit << B_SD);                 /* BCLK = 0 */
    pattern[k++] = (ws << B_WS) | (bit << B_SD) | (1 << B_BCLK); /* BCLK = 1 */
}
```

## 4. uDMA em ping-pong

- canal **18**, encoding 0 = **TIMER0A**: cada timeout pede **uma** transferência (ARBSIZE = 1)
- origem: `pattern[]` (incrementa byte), destino: `&GPIOx->DATA_BITS[mask]` (fixo)
- estrutura **primária e alternativa** apontam para o mesmo buffer (o tom é periódico)
- quando uma termina, o uDMA passa para a outra sem parar; o `main` vê o `XFERMODE` da que terminou em 0 e a re-arma (tem ~1 ms para isso)
- no TM4C129 é preciso habilitar o pedido de DMA do timer: `TIMER0->DMAEV = TATODMAEN`

Para áudio variável (não um tom fixo), use dois buffers diferentes e preencha o que acabou de tocar com as próximas amostras.

## 5. Custo
- CPU: ~0% (só re-arma a cada 1 ms)
- Barramento: 1 M acessos/s ao GPIO e à SRAM pelo uDMA (≈ 1–2% dos ciclos a 80 MHz)
- Jitter: o uDMA arbitra com a CPU; atrasos de alguns ciclos no BCLK são tolerados pelos DACs com PLL (PCM5102A) — DACs que exigem MCLK limpo não são adequados.

## 6. O que conferir na bancada
1. Com o analisador lógico, confira BCLK = 500 kHz, WS = 15,625 kHz e o alinhamento WS→MSB.
2. Se o canal do timer não gerar pedidos no 123, verifique a tabela *μDMA Channel Assignments* do datasheet da sua revisão e o bit correspondente em `CHMAP2`.
3. Se o fluxo parar (o canal sai de `ENASET`), o `main` religa — mas isso indica re-arme atrasado.

## 7. Alternativas de verdade
- Microcontrolador com I2S/SAI (STM32, ver o curso irmão) ou um **codec com interface SPI/TDM**.
- Um DAC SPI (MCP4922) pelo SSI + uDMA, a taxa fixa por timer: mais simples que I2S emulado.
- O módulo 16 (PWM + RC) para áudio de baixa qualidade.

---

## Exercícios
1. Gere 8 kHz (BCLK de 256 kHz) e calcule o novo `TAILR` e o tamanho de buffer.
2. Toque uma melodia: dois buffers, preenchidos com amostras diferentes no laço principal.
3. Meça com o DWT quantos ciclos a CPU perde por causa do tráfego do uDMA.
