#include <stddef.h>
#include <mini-printf.h>

extern int _write(int fd, char *buf, int size);

static int debug_puts(char *s, int len, void *buf)
{
    return _write(1, s, len);
}

int printf(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    int len = mini_vpprintf(debug_puts, NULL, fmt, va);
    va_end(va);
    return len;
}
