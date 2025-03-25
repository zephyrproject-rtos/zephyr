#include <stdio.h>
#include <stdarg.h>

/*
 * Implementing vscanf() by calling vfscanf() on stdin.
 */
int vscanf(const char *format, va_list args) {
    return vfscanf(stdin, format, args);
}
