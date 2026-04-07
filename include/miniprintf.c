#include "miniprintf.h"
#include "uart.h"

/* Minimal variadic argument handling for Xtensa */
typedef struct {
    uint32_t gp[16];  /* general purpose regs saved area */
} va_list_compat;

/* Simple stack-based va_list for Xtensa call0 ABI */
typedef __builtin_va_list va_list;
#define va_start(ap, last)  __builtin_va_start(ap, last)
#define va_arg(ap, type)    __builtin_va_arg(ap, type)
#define va_end(ap)          __builtin_va_end(ap)

static void print_str(const char *s)
{
    uart_puts(s);
}

static void print_char(char c)
{
    uart_putc(c);
}

static void print_uint(uint32_t val, int base, int width, char pad)
{
    static const char digits[] = "0123456789abcdef";
    char buf[12];
    int i = 0;

    if (val == 0) {
        buf[i++] = '0';
    } else {
        while (val > 0) {
            buf[i++] = digits[val % base];
            val /= base;
        }
    }

    /* Pad to width */
    while (i < width)
        buf[i++] = pad;

    /* Print in reverse (correct order) */
    while (--i >= 0)
        uart_putc(buf[i]);
}

static void print_int(int32_t val, int width, char pad)
{
    if (val < 0) {
        uart_putc('-');
        val = -val;
        if (width > 0) width--;
    }
    print_uint((uint32_t)val, 10, width, pad);
}

static void print_ptr(void *ptr)
{
    uart_puts("0x");
    print_uint((uint32_t)(uintptr_t)ptr, 16, 8, '0');
}

int miniprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    int count = 0;
    char pad = ' ';
    int width = 0;

    while (*fmt) {
        if (*fmt != '%') {
            if (*fmt == '\n')
                uart_putc('\r');
            uart_putc(*fmt++);
            count++;
            continue;
        }

        fmt++; /* skip '%' */

        /* Reset formatting state */
        pad = ' ';
        width = 0;

        /* Check for '0' padding */
        if (*fmt == '0') {
            pad = '0';
            fmt++;
        }

        /* Parse width */
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        /* Parse format specifier */
        switch (*fmt) {
        case 'd':
        case 'i':
            print_int(va_arg(ap, int), width, pad);
            break;
        case 'u':
            print_uint(va_arg(ap, uint32_t), 10, width, pad);
            break;
        case 'x':
            print_uint(va_arg(ap, uint32_t), 16, width, pad);
            break;
        case 'p':
            print_ptr(va_arg(ap, void *));
            break;
        case 'c':
            uart_putc((char)va_arg(ap, int));
            break;
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (s)
                print_str(s);
            else
                print_str("(null)");
            break;
        }
        case '%':
            uart_putc('%');
            break;
        default:
            uart_putc('%');
            uart_putc(*fmt);
            break;
        }

        if (*fmt) {
            fmt++;
            count++;
        }
    }

    va_end(ap);
    return count;
}

/* Kernel print - alias for miniprintf */
void printk(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    miniprintf(fmt, ap);
    va_end(ap);
}
