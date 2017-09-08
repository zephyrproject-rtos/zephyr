/****************************************************************************
**
** Copyright (C) 2015 Intel Corporation
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
** THE SOFTWARE.
**
****************************************************************************/

#define _BSD_SOURCE 1
#define _DEFAULT_SOURCE 1
#define _GNU_SOURCE 1

#ifndef WITHOUT_OPEN_MEMSTREAM

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__unix__) || defined(__APPLE__)
#  include <unistd.h>
#endif
#ifdef __APPLE__
typedef int RetType;
typedef int LenType;
#elif __GLIBC__
typedef ssize_t RetType;
typedef size_t LenType;
#else
#  error "Cannot implement open_memstream!"
#endif

#include "tinycbor/compilersupport_p.h"

struct Buffer
{
    char **ptr;
    size_t *len;
    size_t alloc;
};

static RetType write_to_buffer(void *cookie, const char *data, LenType len)
{
    struct Buffer *b = (struct Buffer *)cookie;
    char *ptr = *b->ptr;
    size_t newsize;

    errno = EFBIG;
    if (unlikely(add_check_overflow(*b->len, len, &newsize)))
        return -1;

    if (newsize > b->alloc) {
        // make room
        size_t newalloc = newsize + newsize / 2 + 1;    // give 50% more room
        ptr = realloc(ptr, newalloc);
        if (ptr == NULL)
            return -1;
        b->alloc = newalloc;
        *b->ptr = ptr;
    }

    memcpy(ptr + *b->len, data, len);
    *b->len = newsize;
    return len;
}

static int close_buffer(void *cookie)
{
    struct Buffer *b = (struct Buffer *)cookie;
    if (*b->ptr)
        (*b->ptr)[*b->len] = '\0';
    free(b);
    return 0;
}

FILE *open_memstream(char **bufptr, size_t *lenptr)
{
    struct Buffer *b = (struct Buffer *)malloc(sizeof(struct Buffer));
    if (b == NULL)
        return NULL;
    b->alloc = 0;
    b->len = lenptr;
    b->ptr = bufptr;
    *bufptr = NULL;
    *lenptr = 0;

#ifdef __APPLE__
    return funopen(b, NULL, write_to_buffer, NULL, close_buffer);
#elif __GLIBC__
    static const cookie_io_functions_t vtable = {
        NULL,
        write_to_buffer,
        NULL,
        close_buffer
    };
    return fopencookie(b, "w", vtable);
#endif
}

#endif
