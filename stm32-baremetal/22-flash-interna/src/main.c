/*
 * Módulo 22 - Flash interna como memória não volátil
 *
 * Guarda um contador de boots num "log" na área de dados:
 *   - cada boot grava o próximo valor na primeira word livre (0xFFFFFFFF)
 *   - só apaga quando a área enche  -> distribui o desgaste (wear leveling)
 *     (a Flash aguenta ~10 mil ciclos de apagamento por página/setor)
 *
 * Comandos pela serial: 'e' = apagar a área, 'w' = gravar +1 agora
 */
#include "system.h"
#include "flash.h"

#define NWORDS (DATA_AREA_SIZE / 4U)

static volatile const uint32_t *const area = (volatile const uint32_t *)DATA_AREA_ADDR;

/* Índice da primeira word livre, ou NWORDS se cheia */
static uint32_t first_free(void)
{
    uint32_t lo = 0, hi = NWORDS;      /* busca binária: o log é contíguo */
    while (lo < hi) {
        uint32_t mid = (lo + hi) / 2U;
        if (area[mid] == 0xFFFFFFFFU) hi = mid; else lo = mid + 1U;
    }
    return lo;
}

static uint32_t last_value(void)
{
    uint32_t i = first_free();
    return i ? area[i - 1U] : 0U;
}

static void append(uint32_t v)
{
    uint32_t i = first_free();
    if (i >= NWORDS) {
        kprintf("area cheia: apagando...\n");
        uint32_t t0 = millis();
        if (flash_erase_data_area()) kprintf("ERRO no erase\n");
        kprintf("erase levou %u ms\n", millis() - t0);
        i = 0;
    }
    uint32_t addr = DATA_AREA_ADDR + 4U * i;
    int r = flash_write_word(addr, v);
    kprintf("gravou %u em 0x%08X: %s\n", v, addr, r ? "ERRO" : "ok");
}

int main(void)
{
    system_init();
    uart_init(115200);
    kprintf("\n=== Modulo 22: Flash interna ===\n");
    kprintf("area de dados: 0x%08X, %u bytes (%u words)\n", DATA_AREA_ADDR, DATA_AREA_SIZE, NWORDS);

    uint32_t boots = last_value() + 1U;
    if (boots == 0) boots = 1;         /* 0xFFFFFFFF + 1 */
    kprintf("boot numero %u (word %u de %u usada)\n", boots, first_free(), NWORDS);
    append(boots);

    for (;;) {
        int c = uart_getc();
        if (c == 'e') {
            uint32_t t0 = millis();
            int r = flash_erase_data_area();
            kprintf("apagado (%s) em %u ms\n", r ? "ERRO" : "ok", millis() - t0);
        } else if (c == 'w') {
            append(last_value() + 1U);
        }
        delay_ms(10);
    }
}
