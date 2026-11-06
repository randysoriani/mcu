/*
 * Módulo 13 - TWI / I2C (AT32UC3A)
 *   SDA = PA29, SCL = PA30 (função A). Pull-ups externos de 4k7 para 3,3 V.
 *
 * O TWI do UC3 (o mesmo dos ARM SAM7) é "orientado a transação": você
 * programa endereço do escravo, direção e endereço interno (registrador)
 * no MMR/IADR, dá START e só alimenta/consome os bytes.
 */
#include "system.h"

#define TWI_SR_TXCOMP (1UL << 0)
#define TWI_SR_RXRDY  (1UL << 1)
#define TWI_SR_NACK   (1UL << 8)

static int twi_wait(uint32_t flag)
{
    for (uint32_t t = 0; t < 200000UL; t++) {
        uint32_t sr = TWI->SR;
        if (sr & TWI_SR_NACK) return -1;
        if (sr & flag) return 0;
    }
    return -2;
}

/* Lê n bytes a partir do registrador 'reg' (IADRSZ = 1 byte de endereço interno) */
static int twi_read(uint8_t addr, int reg, uint8_t *buf, uint32_t n)
{
    TWI->MMR  = ((uint32_t)addr << 16) | (1UL << 12) | (reg >= 0 ? (1UL << 8) : 0UL);   /* DADR, MREAD, IADRSZ */
    TWI->IADR = reg >= 0 ? (uint32_t)reg : 0UL;
    TWI->CR   = (n == 1) ? 3UL : 1UL;          /* 1 byte: START + STOP juntos */
    for (uint32_t i = 0; i < n; i++) {
        if (i == n - 1 && n > 1) TWI->CR = 2UL;   /* STOP antes do último byte */
        int e = twi_wait(TWI_SR_RXRDY);
        if (e) { twi_wait(TWI_SR_TXCOMP); return e; }
        buf[i] = (uint8_t)TWI->RHR;
    }
    return twi_wait(TWI_SR_TXCOMP);
}

int main(void)
{
    system_init();
    uart_init(115200);

    gpio_periph(PA(29), 0);                       /* TWI SDA */
    gpio_periph(PA(30), 0);                       /* TWI SCL */
    TWI->CR = (1UL << 7);                         /* SWRST */
    (void)TWI->RHR;
    /* T_low = T_high = (CLDIV * 2^CKDIV + 4) / PBA -> CLDIV = 48e6/200e3 - 4 = 236 */
    TWI->CWGR = (236UL << 0) | (236UL << 8) | (0UL << 16);
    TWI->CR = (1UL << 2) | (1UL << 5);            /* MSEN (mestre), SVDIS */

    kprintf("\n=== Modulo 13: TWI/I2C (SDA = PA29, SCL = PA30) ===\n");
    for (;;) {
        uint8_t v;
        kprintf("encontrados:");
        for (uint8_t a = 0x08; a <= 0x77; a++) if (twi_read(a, -1, &v, 1) == 0) kprintf(" 0x%02x", a);
        kprintf("\n");
        if (twi_read(0x68, 0x75, &v, 1) == 0) kprintf("0x68: MPU-6050 WHO_AM_I = 0x%02x\n", v);
        if (twi_read(0x76, 0xD0, &v, 1) == 0) kprintf("0x76: BMP/BME280 id = 0x%02x\n", v);
        delay_ms(3000);
    }
}
