/*
 * Módulo 20 - Autoprogramação da Flash (AT32UC3A)
 *
 * No UC3 qualquer código pode gravar a Flash pelo controlador FLASHC.
 * O programa grava a última página da Flash (0x8007_FE00) e lê de volta.
 * (Enquanto a Flash grava, a CPU que executa da Flash fica parada.)
 */
#include "system.h"

#define ALVO_PAGINA  ((512UL * 1024UL) / FLASH_PAGE_SIZE - 1UL)     /* página 1023 */
#define ALVO_ADDR    (FLASH_BASE + ALVO_PAGINA * FLASH_PAGE_SIZE)

static void flashc_cmd(uint32_t cmd, uint32_t page)
{
    while (!(FLASHC->FSR & FLASHC_FSR_FRDY)) { }
    FLASHC->FCMD = FLASHC_KEY | (page << 8) | cmd;
    while (!(FLASHC->FSR & FLASHC_FSR_FRDY)) { }
}

int main(void)
{
    system_init();
    uart_init(115200);
    volatile uint32_t *pg = (volatile uint32_t *)ALVO_ADDR;

    kprintf("\n=== Modulo 20: autoprogramacao (FLASHC) ===\n");
    kprintf("antes : word 0 da pagina %lu = 0x%08lx\n", (unsigned long)ALVO_PAGINA, (unsigned long)pg[0]);

    uint32_t base = (pg[0] == 0xFFFFFFFFUL) ? 0 : pg[0] + 1UL;
    flashc_cmd(FLASHC_CMD_EP, ALVO_PAGINA);       /* apaga a página */
    flashc_cmd(FLASHC_CMD_CPB, 0);                /* limpa o page buffer */
    for (uint32_t i = 0; i < FLASH_PAGE_SIZE / 4; i++) pg[i] = base + i;   /* escreve no buffer */
    flashc_cmd(FLASHC_CMD_WP, ALVO_PAGINA);       /* grava */

    uint32_t erros = 0;
    for (uint32_t i = 0; i < FLASH_PAGE_SIZE / 4; i++) if (pg[i] != base + i) erros++;
    kprintf("depois: word 0 = 0x%08lx, %lu erros (FSR = 0x%08lx)\n",
            (unsigned long)pg[0], (unsigned long)erros, (unsigned long)FLASHC->FSR);
    for (;;) { }
}
