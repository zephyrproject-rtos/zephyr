#include <stdio.h>
#include <stdarg.h>

int fscanf(FILE *stream, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = vfscanf(stream, format, args);
    va_end(args);
    return result;
}
