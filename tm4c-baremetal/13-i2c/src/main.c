/*
 * Módulo 13 - I2C
 *
 * 1. Varre o barramento (como o i2cdetect do Linux) e mostra os endereços
 *    que responderam com ACK.
 * 2. Para alguns sensores comuns, lê o registrador de identificação:
 *      0x68/0x69  MPU-6050/9250 -> WHO_AM_I (0x75)
 *      0x76/0x77  BMP280/BME280 -> chip id  (0xD0): 0x58 = BMP280, 0x60 = BME280
 *      0x1E       HMC5883L      -> ID A     (0x0A) = 'H'
 *      0x50..0x57 EEPROM 24Cxx  -> lê 8 bytes do endereço 0
 */
#include "system.h"
#include "i2c.h"

static void scan(void)
{
    kprintf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
    for (uint32_t base = 0; base < 0x80; base += 16) {
        kprintf("%02x: ", base);
        for (uint32_t a = base; a < base + 16; a++) {
            if (a < 0x08 || a > 0x77) { kprintf("   "); continue; }   /* reservados */
            int r = i2c_probe((uint8_t)a);
            if (r == I2C_OK)            kprintf("%02x ", a);
            else if (r == I2C_TIMEOUT)  kprintf("TO ");
            else                        kprintf("-- ");
        }
        kprintf("\n");
    }
}

static void identifica(void)
{
    uint8_t v[8];
    for (uint8_t a = 0x68; a <= 0x69; a++)
        if (i2c_read_reg(a, 0x75, v, 1) == I2C_OK)
            kprintf("0x%02X: MPU WHO_AM_I = 0x%02X\n", a, v[0]);
    for (uint8_t a = 0x76; a <= 0x77; a++)
        if (i2c_read_reg(a, 0xD0, v, 1) == I2C_OK)
            kprintf("0x%02X: BMx280 id = 0x%02X %s\n", a, v[0],
                    v[0] == 0x60 ? "(BME280)" : v[0] == 0x58 ? "(BMP280)" : "");
    if (i2c_read_reg(0x1E, 0x0A, v, 3) == I2C_OK)
        kprintf("0x1E: HMC5883L id = %c%c%c\n", v[0], v[1], v[2]);
    for (uint8_t a = 0x50; a <= 0x57; a++) {
        uint8_t addr0 = 0;
        if (i2c_write_read(a, &addr0, 1, v, 8) == I2C_OK) {
            kprintf("0x%02X: EEPROM[0..7] =", a);
            for (int i = 0; i < 8; i++) kprintf(" %02X", v[i]);
            kprintf("\n");
            break;
        }
    }
}

int main(void)
{
    system_init();
    uart_init(115200);
    i2c_init();

    kprintf("\n=== Modulo 13: I2C (SCL = PB2, SDA = PB3) ===\n");
    for (;;) {
        scan();
        identifica();
        kprintf("\n(nova varredura em 5 s)\n");
        delay_ms(5000);
    }
}
