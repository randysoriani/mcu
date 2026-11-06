/*
 * Módulo 16 - EEPROM interna (ATmega328P): 1 KB, cerca de 100 000 escritas por byte
 *
 *   - contador de boots no endereço 0..1 (sobrevive a desligar a placa)
 *   - variável com valor inicial gravado na EEPROM pelo programador (.eep)
 *   Comandos: 'z' zera o contador, 'd' mostra os 32 primeiros bytes
 */
#include "system.h"

/* Variável que mora na EEPROM: o valor inicial vai para o arquivo .eep */
__attribute__((section(".eeprom"))) uint8_t ee_config[8] = { 'A', 'V', 'R', '-', 'E', 'E', 0, 0 };

static uint8_t ee_read(uint16_t addr)
{
    while (EECR & (1U << EEPE)) { }               /* espera escrita anterior */
    EEAR = addr;
    EECR |= (1U << EERE);                         /* leitura: o dado fica em EEDR */
    return EEDR;
}

static void ee_write(uint16_t addr, uint8_t v)
{
    if (ee_read(addr) == v) return;               /* evita desgaste à toa */
    while (EECR & (1U << EEPE)) { }
    EEAR = addr;
    EEDR = v;
    uint8_t s = SREG;
    cli();                                        /* a sequência abaixo tem de levar <= 4 ciclos */
    EECR |= (1U << EEMPE);                        /* "master write enable" */
    EECR |= (1U << EEPE);                         /* dispara a escrita (~3,4 ms) */
    SREG = s;
}

int main(void)
{
    system_init();
    uart_init(115200);

    uint16_t boots = (uint16_t)(ee_read(0) | (ee_read(1) << 8));
    if (boots == 0xFFFF) boots = 0;               /* EEPROM apagada vale 0xFF */
    boots++;
    ee_write(0, (uint8_t)boots);
    ee_write(1, (uint8_t)(boots >> 8));

    kprintf("\n=== Modulo 16: EEPROM ===\nboot numero %u\n", boots);
    kprintf("ee_config (endereco %u da EEPROM): \"", (unsigned)(uintptr_t)ee_config);
    for (uint8_t i = 0; i < 6; i++) uart_putc((char)ee_read((uint16_t)(uintptr_t)ee_config + i));
    kprintf("\" (precisa gravar o .eep: veja o README)\n");

    for (;;) {
        int c = uart_getc();
        if (c == 'z') { ee_write(0, 0); ee_write(1, 0); kprintf("contador zerado\n"); }
        if (c == 'd') {
            for (uint16_t a = 0; a < 32; a++) kprintf("%02x%c", ee_read(a), (a % 16 == 15) ? '\n' : ' ');
        }
    }
}
