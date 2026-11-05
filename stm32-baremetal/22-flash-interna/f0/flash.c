/*
 * flash.c (F0) - apagar página e programar a Flash do STM32F072.
 *
 * - Flash organizada em PÁGINAS de 2 KB
 * - Programação em meias-palavras (16 bits)
 * - Após erase todos os bits valem 1; programar só transforma 1 em 0
 * - Enquanto a Flash está ocupada, a CPU que tenta buscar instruções nela
 *   fica parada (stall) - normal num chip de banco único
 */
#include "device.h"
#include "flash.h"

static void unlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = FLASH_KEY1;      /* sequência de chaves; errar trava até o reset */
        FLASH->KEYR = FLASH_KEY2;
    }
}

static void lock(void) { FLASH->CR |= FLASH_CR_LOCK; }

static int wait_done(void)
{
    while (FLASH->SR & FLASH_SR_BSY) { }
    uint32_t sr = FLASH->SR;
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR;   /* rc_w1 */
    return (sr & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) ? -1 : 0;
}

int flash_erase_data_area(void)
{
    int err = 0;
    unlock();
    for (uint32_t a = DATA_AREA_ADDR; a < DATA_AREA_ADDR + DATA_AREA_SIZE; a += FLASH_PAGE_SIZE) {
        FLASH->CR |= FLASH_CR_PER;     /* page erase */
        FLASH->AR  = a;                /* qualquer endereço dentro da página */
        FLASH->CR |= FLASH_CR_STRT;
        err |= wait_done();
        FLASH->CR &= ~FLASH_CR_PER;
    }
    lock();
    return err;
}

int flash_write_word(uint32_t addr, uint32_t value)
{
    int err;
    unlock();
    FLASH->CR |= FLASH_CR_PG;
    *(volatile uint16_t *)addr       = (uint16_t)value;          /* metade baixa */
    err  = wait_done();
    *(volatile uint16_t *)(addr + 2) = (uint16_t)(value >> 16);  /* metade alta  */
    err |= wait_done();
    FLASH->CR &= ~FLASH_CR_PG;
    lock();
    return (err || *(volatile uint32_t *)addr != value) ? -1 : 0;
}
