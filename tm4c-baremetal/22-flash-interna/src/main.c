/*
 * Módulo 22 - Flash interna como memória não volátil
 *
 *   123: blocos de apagamento de 1 KB  -> área = último bloco  0x0003FC00
 *   129: setores de 16 KB              -> área = último setor  0x000FC000
 * Programação word a word (32 bits) com a "chave" WRKEY no FMC:
 *   123: 0xA442 (BOOTCFG.KEY = 1, padrão)   129: valor de FLPEKEY
 *
 * Log de contador de boots com wear leveling (só apaga quando enche).
 * Comandos: 'e' = apagar, 'w' = gravar +1
 * (Para dados pequenos o TM4C também tem uma EEPROM de verdade: 2 KB/6 KB.)
 */
#include "system.h"

#define DATA_AREA_ADDR  (FLASH_SIZE - FLASH_ERASE_SIZE)
#define DATA_AREA_SIZE  FLASH_ERASE_SIZE
#define NWORDS (DATA_AREA_SIZE / 4U)

static volatile const uint32_t *const area = (volatile const uint32_t *)DATA_AREA_ADDR;

static uint32_t wrkey(void)
{
#if defined(TM4C123)
    return 0xA4420000U;
#else
    return (FLASH_FLPEKEY & 0xFFFFU) << 16;
#endif
}

static int flash_erase(uint32_t addr)
{
    FLASH_FCMISC = 0x3U;                       /* limpa flags antigas */
    FLASH_FMA = addr;
    FLASH_FMC = wrkey() | FLASH_FMC_ERASE;
    while (FLASH_FMC & FLASH_FMC_ERASE) { }    /* o bit volta a 0 no fim */
    return (FLASH_FCRIS & FLASH_FCRIS_ARIS) ? -1 : 0;
}

static int flash_write_word(uint32_t addr, uint32_t v)
{
    FLASH_FCMISC = 0x3U;
    FLASH_FMA = addr;
    FLASH_FMD = v;
    FLASH_FMC = wrkey() | FLASH_FMC_WRITE;
    while (FLASH_FMC & FLASH_FMC_WRITE) { }
    return ((FLASH_FCRIS & FLASH_FCRIS_ARIS) || *(volatile uint32_t *)addr != v) ? -1 : 0;
}

static uint32_t first_free(void)
{
    uint32_t lo = 0, hi = NWORDS;
    while (lo < hi) {
        uint32_t mid = (lo + hi) / 2U;
        if (area[mid] == 0xFFFFFFFFU) hi = mid; else lo = mid + 1U;
    }
    return lo;
}

static uint32_t last_value(void) { uint32_t i = first_free(); return i ? area[i - 1U] : 0U; }

static void append(uint32_t v)
{
    uint32_t i = first_free();
    if (i >= NWORDS) {
        kprintf("area cheia: apagando...\n");
        if (flash_erase(DATA_AREA_ADDR)) kprintf("ERRO no erase\n");
        i = 0;
    }
    uint32_t addr = DATA_AREA_ADDR + 4U * i;
    kprintf("gravou %u em 0x%08X: %s\n", v, addr, flash_write_word(addr, v) ? "ERRO" : "ok");
}

int main(void)
{
    system_init();
    uart_init(115200);
    kprintf("\n=== Modulo 22: Flash interna ===\n");
    kprintf("area de dados: 0x%08X, %u bytes (%u words), chave 0x%04X\n",
            DATA_AREA_ADDR, DATA_AREA_SIZE, NWORDS, wrkey() >> 16);

    uint32_t boots = last_value() + 1U;
    if (boots == 0) boots = 1;
    kprintf("boot numero %u (word %u de %u usada)\n", boots, first_free(), NWORDS);
    append(boots);

    for (;;) {
        int c = uart_getc();
        if (c == 'e') {
            uint32_t t0 = millis();
            int r = flash_erase(DATA_AREA_ADDR);
            kprintf("apagado (%s) em %u ms\n", r ? "ERRO" : "ok", millis() - t0);
        } else if (c == 'w') {
            append(last_value() + 1U);
        }
        delay_ms(10);
    }
}
