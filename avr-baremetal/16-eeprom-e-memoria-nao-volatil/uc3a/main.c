/*
 * Módulo 16 - Memória não volátil no AT32UC3A: a "user page" da Flash
 *
 * O UC3A não tem EEPROM. Para guardar configurações usa-se a user page:
 * 512 bytes de Flash em 0x8080_0000, separados do programa (não são
 * apagados por um "chip erase" comum).
 *
 * Gravar Flash no UC3: limpar o "page buffer", escrever as words no
 * endereço da página (vão para o buffer), dar o comando de gravação.
 */
#include "system.h"

#define UP ((volatile uint32_t *)USERPAGE_BASE)

static void flashc_cmd(uint32_t cmd, uint32_t page)
{
    while (!(FLASHC->FSR & FLASHC_FSR_FRDY)) { }
    FLASHC->FCMD = FLASHC_KEY | (page << 8) | cmd;
    while (!(FLASHC->FSR & FLASHC_FSR_FRDY)) { }
}

static int userpage_write(const uint32_t *words, uint32_t n)
{
    uint32_t copia[FLASH_PAGE_SIZE / 4];
    for (uint32_t i = 0; i < FLASH_PAGE_SIZE / 4; i++) copia[i] = UP[i];   /* preserva o resto */
    for (uint32_t i = 0; i < n; i++) copia[i] = words[i];

    flashc_cmd(FLASHC_CMD_EUP, 0);                /* apaga a user page (tudo vira 0xFF) */
    flashc_cmd(FLASHC_CMD_CPB, 0);                /* limpa o page buffer */
    for (uint32_t i = 0; i < FLASH_PAGE_SIZE / 4; i++) UP[i] = copia[i];  /* enche o buffer */
    flashc_cmd(FLASHC_CMD_WUP, 0);                /* grava a user page */
    return (FLASHC->FSR & (FLASHC_FSR_LOCKE | FLASHC_FSR_PROGE)) ? -1 : 0;
}

int main(void)
{
    system_init();
    uart_init(115200);

    /* As primeiras words da user page podem ter configurações do bootloader
     * da EVK1100: usamos a word 64 em diante. */
    uint32_t boots = UP[64];
    if (boots == 0xFFFFFFFFUL) boots = 0;
    boots++;
    uint32_t dados[65];
    for (uint32_t i = 0; i < 64; i++) dados[i] = UP[i];
    dados[64] = boots;
    int r = userpage_write(dados, 65);

    kprintf("\n=== Modulo 16: user page ===\nboot numero %lu (gravacao %s)\n",
            (unsigned long)boots, r ? "ERRO" : "ok");
    for (;;) { }
}
