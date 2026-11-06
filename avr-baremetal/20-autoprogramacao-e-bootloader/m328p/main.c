/*
 * Módulo 20 - Autoprogramação da Flash (ATmega328P)
 *
 * No ATmega a instrução SPM (que grava a Flash) SÓ funciona quando
 * executada da SEÇÃO DE BOOT (o fim da Flash, tamanho escolhido pelos
 * fuses BOOTSZ). Aqui a rotina de gravação é colocada na seção
 * ".bootloader", que o linker script deste módulo põe em 0x7000
 * (seção de boot de 2 KB: fuse alto com BOOTSZ = 00).
 *
 * >>> Exige gravação por um programador ISP (USBasp etc.): isto APAGA o
 *     bootloader do Arduino. Veja o README para os fuses e como restaurar. <<<
 *
 * O programa grava 128 bytes na página 0x6F80 (última página antes da
 * seção de boot) e lê de volta com LPM.
 */
#include "system.h"

#define PAGE_SIZE 128U
#define ALVO      0x6F80U

#define BOOT __attribute__((section(".bootloader"), noinline))

/* Executa uma instrução SPM com o comando 'cmd' no endereço Z */
BOOT static void spm_cmd(uint16_t z, uint8_t cmd)
{
    __asm__ __volatile__ (
        "sts %0, %1 \n\t"
        "spm        \n\t"
        :: "i"(0x57), "r"(cmd), "z"(z) : "memory");
    while (SPMCSR & (1U << SPMEN)) { }            /* espera terminar */
}

BOOT static void spm_fill(uint16_t z, uint16_t word)
{
    __asm__ __volatile__ (
        "movw r0, %2 \n\t"                        /* o dado vai em r1:r0 */
        "sts %0, %1  \n\t"
        "spm         \n\t"
        "clr r1      \n\t"                        /* r1 volta a ser o "zero" do GCC */
        :: "i"(0x57), "r"((uint8_t)(1U << SPMEN)), "r"(word), "z"(z) : "r0", "memory");
}

BOOT void flash_write_page(uint16_t addr, const uint8_t *data)
{
    uint8_t s = SREG;
    cli();
    while (EECR & (1U << EEPE)) { }               /* não pode gravar com a EEPROM ocupada */
    spm_cmd(addr, (1U << PGERS) | (1U << SPMEN)); /* 1) apaga a página */
    spm_cmd(addr, (1U << RWWSRE) | (1U << SPMEN));/*    reabilita a leitura da área de aplicação */
    for (uint16_t i = 0; i < PAGE_SIZE; i += 2)   /* 2) enche o buffer temporário */
        spm_fill(addr + i, (uint16_t)(data[i] | (data[i + 1] << 8)));
    spm_cmd(addr, (1U << PGWRT) | (1U << SPMEN)); /* 3) grava o buffer na página */
    spm_cmd(addr, (1U << RWWSRE) | (1U << SPMEN));
    SREG = s;
}

static uint8_t flash_read(uint16_t addr)
{
    uint8_t r;
    __asm__ __volatile__ ("lpm %0, Z" : "=r"(r) : "z"(addr));
    return r;
}

int main(void)
{
    system_init();
    uart_init(115200);
    kprintf("\n=== Modulo 20: autoprogramacao (SPM) ===\n");
    kprintf("antes : primeiro byte em 0x%04x = 0x%02x\n", ALVO, flash_read(ALVO));

    static uint8_t buf[PAGE_SIZE];
    uint8_t base = (uint8_t)(flash_read(ALVO) + 1U);
    for (uint16_t i = 0; i < PAGE_SIZE; i++) buf[i] = (uint8_t)(base + i);
    flash_write_page(ALVO, buf);

    uint16_t erros = 0;
    for (uint16_t i = 0; i < PAGE_SIZE; i++) if (flash_read(ALVO + i) != buf[i]) erros++;
    kprintf("depois: primeiro byte = 0x%02x, %u erros na pagina\n", flash_read(ALVO), erros);
    kprintf("(aperte RESET: o valor continua aumentando, pois esta na Flash)\n");
    for (;;) { }
}
