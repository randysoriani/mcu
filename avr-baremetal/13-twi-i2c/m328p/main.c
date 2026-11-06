/*
 * Módulo 13 - TWI / I2C (ATmega328P)
 *   SDA = A4 (PC4), SCL = A5 (PC5). Pull-ups de 4k7 para 5 V (ou 3,3 V,
 *   conforme o sensor). Varre o barramento e lê o registrador de
 *   identificação de alguns sensores comuns.
 *
 * O TWI do AVR é conduzido passo a passo: cada ação (START, enviar byte,
 * ler byte, STOP) termina com a flag TWINT e um CÓDIGO DE STATUS em TWSR,
 * que diz exatamente o que aconteceu no barramento.
 */
#include "system.h"

#define TW_START       0x08
#define TW_REP_START   0x10
#define TW_MT_SLA_ACK  0x18   /* endereço + escrita reconhecido */
#define TW_MT_DATA_ACK 0x28
#define TW_MR_SLA_ACK  0x40   /* endereço + leitura reconhecido */
#define TW_MR_DATA_ACK 0x50
#define TW_MR_DATA_NACK 0x58

static uint8_t tw_status(void) { return TWSR & 0xF8U; }

static int tw_wait(void)
{
    uint16_t t = 0;
    while (!(TWCR & (1U << TWINT))) { if (++t == 0) return -1; }   /* timeout */
    return 0;
}

static uint8_t tw_start(uint8_t addr_rw)
{
    TWCR = (1U << TWINT) | (1U << TWSTA) | (1U << TWEN);           /* gera START */
    if (tw_wait()) return 0;
    if (tw_status() != TW_START && tw_status() != TW_REP_START) return 0;
    TWDR = addr_rw;                                                 /* endereço + R/W */
    TWCR = (1U << TWINT) | (1U << TWEN);
    if (tw_wait()) return 0;
    return tw_status();
}

static void tw_stop(void)
{
    TWCR = (1U << TWINT) | (1U << TWSTO) | (1U << TWEN);
    while (TWCR & (1U << TWSTO)) { }                                /* o hardware limpa TWSTO */
}

static int tw_write(uint8_t b)
{
    TWDR = b;
    TWCR = (1U << TWINT) | (1U << TWEN);
    if (tw_wait()) return -1;
    return tw_status() == TW_MT_DATA_ACK ? 0 : -1;
}

static int tw_read(uint8_t ack)                                     /* ack = 0 no último byte */
{
    TWCR = (1U << TWINT) | (1U << TWEN) | (ack ? (1U << TWEA) : 0U);
    if (tw_wait()) return -1;
    return TWDR;
}

static int i2c_probe(uint8_t addr)
{
    uint8_t st = tw_start((uint8_t)(addr << 1));
    tw_stop();
    return st == TW_MT_SLA_ACK;
}

static int i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *val)
{
    int ok = tw_start((uint8_t)(addr << 1)) == TW_MT_SLA_ACK && tw_write(reg) == 0
          && tw_start((uint8_t)((addr << 1) | 1U)) == TW_MR_SLA_ACK;     /* repeated START */
    if (ok) { int v = tw_read(0); ok = v >= 0; if (ok) *val = (uint8_t)v; }
    tw_stop();
    return ok;
}

int main(void)
{
    system_init();
    uart_init(115200);

    TWSR = 0;                                    /* prescaler = 1 */
    TWBR = (uint8_t)((F_CPU / 100000UL - 16UL) / 2UL);   /* SCL = F_CPU / (16 + 2*TWBR) = 100 kHz */
    TWCR = (1U << TWEN);

    kprintf("\n=== Modulo 13: TWI/I2C (SDA = A4, SCL = A5) ===\n");
    for (;;) {
        kprintf("encontrados:");
        for (uint8_t a = 0x08; a <= 0x77; a++) if (i2c_probe(a)) kprintf(" 0x%02x", a);
        kprintf("\n");
        uint8_t v;
        if (i2c_read_reg(0x68, 0x75, &v)) kprintf("0x68: MPU-6050 WHO_AM_I = 0x%02x\n", v);
        if (i2c_read_reg(0x76, 0xD0, &v)) kprintf("0x76: BMP/BME280 id = 0x%02x\n", v);
        if (i2c_read_reg(0x77, 0xD0, &v)) kprintf("0x77: BMP/BME280 id = 0x%02x\n", v);
        delay_ms(3000);
    }
}
