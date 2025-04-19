#include <stdio.h>

int ferror(FILE *stream)
{
    if (stream == NULL) {
        return 0;
    }

    return stream->err;
}
