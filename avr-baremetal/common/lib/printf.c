/*
 * printf.c - kprintf() mínimo e portátil (AVR de 8 e de 32 bits).
 *
 *   %d %i %u %x %X %c %s %%   com int    (16 bits no AVR8, 32 bits no AVR32)
 *   %ld %lu %lx %lX           com long   (32 bits nos dois)
 *   largura e zeros: %5d, %04x, %-8s
 * Sem float (no AVR de 8 bits a biblioteca de float ocupa vários KB).
 */
#include <stdarg.h>
#include <stdint.h>
#include "system.h"

static void out_pad(char c, int n, int *count) { while (n-- > 0) { uart_putc(c); (*count)++; } }

static void out_num(unsigned long v, unsigned base, int neg, int upper,
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
            if (*fmt == '\n') { uart_putc('\r'); count++; }
            uart_putc(*fmt); count++;
            continue;
        }
        fmt++;
        int left = 0, width = 0, lng = 0;
        char pad = ' ';
        if (*fmt == '-') { left = 1; fmt++; }
        if (*fmt == '0') { pad = '0'; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');
        if (*fmt == 'l') { lng = 1; fmt++; }
        switch (*fmt) {
        case 'd': case 'i': {
            long v = lng ? va_arg(ap, long) : (long)va_arg(ap, int);
            out_num(v < 0 ? (unsigned long)(-(v + 1)) + 1UL : (unsigned long)v, 10, v < 0, 0, width, left, pad, &count);
            break;
        }
        case 'u': case 'x': case 'X': {
            unsigned long v = lng ? va_arg(ap, unsigned long) : (unsigned long)va_arg(ap, unsigned);
            out_num(v, *fmt == 'u' ? 10 : 16, 0, *fmt == 'X', width, left, pad, &count);
            break;
        }
        case 'c': uart_putc((char)va_arg(ap, int)); count++; break;
        case 's': {
            const char *s = va_arg(ap, const char *);
            int len = 0;
            for (const char *p = s; *p; p++) len++;
            if (!left) out_pad(' ', width - len, &count);
            while (*s) { uart_putc(*s++); count++; }
            if (left) out_pad(' ', width - len, &count);
            break;
        }
        case '%': uart_putc('%'); count++; break;
        case '\0': goto fim;
        default: uart_putc('%'); uart_putc(*fmt); count += 2; break;
        }
    }
fim:
    va_end(ap);
    return count;
}
