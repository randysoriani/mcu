/*
 * i2c.h - interface do driver I2C1 mestre (f0/i2c.c e f4/i2c.c)
 *
 * Pinos (iguais nas duas Nucleo): PB8 = SCL (D15), PB9 = SDA (D14)
 * O barramento precisa de resistores de pull-up (4k7 para 3V3). A maioria
 * dos módulos breakout já tem; os pull-ups internos (~40k) só servem para
 * testes curtos a 100 kHz.
 *
 * Retorno das funções: 0 = OK, -1 = NACK (ninguém respondeu), -2 = timeout
 */
#ifndef I2C_H
#define I2C_H
#include <stdint.h>
#include <stddef.h>

#define I2C_OK       0
#define I2C_NACK    -1
#define I2C_TIMEOUT -2

void i2c_init(void);                                   /* 100 kHz */
int  i2c_probe(uint8_t addr7);
int  i2c_write(uint8_t addr7, const uint8_t *data, size_t n);
int  i2c_read(uint8_t addr7, uint8_t *data, size_t n);
/* escreve 'wn' bytes, REPEATED START, lê 'rn' bytes (típico p/ ler registradores) */
int  i2c_write_read(uint8_t addr7, const uint8_t *w, size_t wn, uint8_t *r, size_t rn);

static inline int i2c_read_reg(uint8_t addr7, uint8_t reg, uint8_t *val, size_t n)
{
    return i2c_write_read(addr7, &reg, 1, val, n);
}

static inline int i2c_write_reg(uint8_t addr7, uint8_t reg, uint8_t val)
{
    uint8_t b[2] = { reg, val };
    return i2c_write(addr7, b, 2);
}

#endif
