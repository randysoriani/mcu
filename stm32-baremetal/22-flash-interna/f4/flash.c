/*
 * flash.c (F4) - apagar setor e programar a Flash do STM32F446.
 *
 * - Flash organizada em SETORES de tamanhos diferentes:
 *     0-3: 16 KB | 4: 64 KB | 5-7: 128 KB  (512 KB no total)
 * - Largura de programação (PSIZE) depende da tensão: com 2,7-3,6 V use x32
 * - Apagar um setor de 128 KB leva ~1-2 s (CPU parada se rodar da Flash)
 * - Após apagar, invalide o cache de dados (ART), senão leituras podem
 *   devolver o conteúdo antigo
 */
#include "device.h"
#include "flash.h"

#define DATA_SECTOR 7U
#define FLASH_SR_ERRS (FLASH_SR_OPERR | FLASH_SR_WRPERR | FLASH_SR_PGAERR | \
                       FLASH_SR_PGPERR | FLASH_SR_PGSERR)

static void unlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
}

static void lock(void) { FLASH->CR |= FLASH_CR_LOCK; }

static int wait_done(void)
{
    while (FLASH->SR & FLASH_SR_BSY) { }
    uint32_t sr = FLASH->SR;
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_ERRS;          /* rc_w1 */
    return (sr & FLASH_SR_ERRS) ? -1 : 0;
}

static void flush_data_cache(void)
{
    FLASH->ACR &= ~FLASH_ACR_DCEN;
    FLASH->ACR |=  FLASH_ACR_DCRST;
    FLASH->ACR &= ~FLASH_ACR_DCRST;
    FLASH->ACR |=  FLASH_ACR_DCEN;
}

int flash_erase_data_area(void)
{
    unlock();
    (void)wait_done();                                 /* limpa erros antigos */
    FLASH->CR = FLASH_CR_SER                           /* sector erase       */
              | (DATA_SECTOR << FLASH_CR_SNB_Pos)
              | (2U << FLASH_CR_PSIZE_Pos);            /* x32                */
    FLASH->CR |= FLASH_CR_STRT;
    int err = wait_done();
    FLASH->CR = 0;
    lock();
    flush_data_cache();
    return err;
}

int flash_write_word(uint32_t addr, uint32_t value)
{
    unlock();
    FLASH->CR = FLASH_CR_PG | (2U << FLASH_CR_PSIZE_Pos);
    *(volatile uint32_t *)addr = value;
    int err = wait_done();
    FLASH->CR = 0;
    lock();
    return (err || *(volatile uint32_t *)addr != value) ? -1 : 0;
}
