/*
 * printf.c - printf minimalista, portátil (sem newlib, sem malloc).
 *
 * Suporta: %d %i %u %x %X %c %s %p %% , flag '0', flag '-' e largura.
 *          Ex.: "%5d", "%08X", "%-10s"
 * Não suporta float (no F0 isso puxaria ~10 KB de soft-float).
 * Para float, imprima parte inteira e fracionária separadamente.
 */
#include <stdarg.h>
#include <stdint.h>
#include "system.h"

static void out_pad(char c, int n, int *count)
{
    while (n-- > 0) { uart_putc(c); (*count)++; }
}

static void out_str(const char *s, int width, int left, char pad, int *count)
{
    int len = 0;
    for (const char *p = s; *p; p++) len++;
    if (!left) out_pad(pad, width - len, count);
    while (*s) { uart_putc(*s++); (*count)++; }
    if (left) out_pad(' ', width - len, count);
}

static void out_num(uint32_t v, unsigned base, int neg, int upper,
                    int width, int left, char pad, int *count)
{
    char buf[12];
    const char *dig = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    int i = 0;
    do { buf[i++] = dig[v % base]; v /= base; } while (v);

    int len = i + (neg ? 1 : 0);
    if (neg && pad == '0') { uart_putc('-'); (*count)++; neg = 0; }
    if (!left) out_pad(pad, width - len, count);
    if (neg) { uart_putc('-'); (*count)++; }
    while (i) { uart_putc(buf[--i]); (*count)++; }
    if (left) out_pad(' ', width - len, count);
}

int kprintf(const char *fmt, ...)
{
    va_list ap;
    int count = 0;
    va_start(ap, fmt);

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            if (*fmt == '\n') { uart_putc('\r'); count++; }  /* terminais gostam de CRLF */
            uart_putc(*fmt);
            count++;
            continue;
        }
        fmt++;
        int left = 0, width = 0;
        char pad = ' ';
        if (*fmt == '-') { left = 1; fmt++; }
        if (*fmt == '0') { pad = '0'; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');
        if (*fmt == 'l') fmt++;              /* %lu, %ld: int == long aqui */

        switch (*fmt) {
        case 'd': case 'i': {
            int32_t v = va_arg(ap, int32_t);
            out_num(v < 0 ? (uint32_t)(-(v + 1)) + 1U : (uint32_t)v, 10, v < 0, 0,
                    width, left, pad, &count);
            break;
        }
        case 'u': out_num(va_arg(ap, uint32_t), 10, 0, 0, width, left, pad, &count); break;
        case 'x': out_num(va_arg(ap, uint32_t), 16, 0, 0, width, left, pad, &count); break;
        case 'X': out_num(va_arg(ap, uint32_t), 16, 0, 1, width, left, pad, &count); break;
        case 'p':
            out_str("0x", 0, 0, ' ', &count);
            out_num((uint32_t)va_arg(ap, void *), 16, 0, 0, 8, 0, '0', &count);
            break;
        case 'c': uart_putc((char)va_arg(ap, int)); count++; break;
        case 's': {
            const char *s = va_arg(ap, const char *);
            out_str(s ? s : "(null)", width, left, ' ', &count);
            break;
        }
        case '%': uart_putc('%'); count++; break;
        case '\0': goto end;
        default:  uart_putc('%'); uart_putc(*fmt); count += 2; break;
        }
    }
end:
    va_end(ap);
    return count;
}
