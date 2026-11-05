/*
 * flash.h - escrita na Flash interna (f0/flash.c, f4/flash.c)
 *
 * Área de dados usada pelo módulo:
 *   F072: última página de 2 KB  -> 0x0801F800 .. 0x0801FFFF
 *   F446: setor 7 (128 KB)       -> 0x08060000 .. 0x0807FFFF
 * O programa deve ser menor que isso (o Makefile mostra o uso de FLASH).
 */
#ifndef FLASH_H
#define FLASH_H
#include <stdint.h>

#if defined(STM32F0)
#  define DATA_AREA_ADDR  0x0801F800UL
#  define DATA_AREA_SIZE  2048UL
#else
#  define DATA_AREA_ADDR  0x08060000UL
#  define DATA_AREA_SIZE  (128UL * 1024UL)
#endif

int flash_erase_data_area(void);                    /* 0 = ok */
int flash_write_word(uint32_t addr, uint32_t value); /* endereço alinhado a 4 */

#endif
