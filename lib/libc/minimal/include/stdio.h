/* stdio.h */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __INC_stdio_h__
#define __INC_stdio_h__

#include <stdarg.h>     /* Needed to get definition of va_list */

#if !defined(__size_t_defined)
#define __size_t_defined
typedef unsigned int  size_t;
#endif

#if !defined(__FILE_defined)
#define __FILE_defined
typedef int  FILE;
#endif

#if !defined(NULL)
#define NULL (void *) 0
#endif

#if !defined(EOF)
#define EOF  (-1)
#endif

#define stdin  ((FILE *) 1)
#define stdout ((FILE *) 2)
#define stderr ((FILE *) 3)

/*
 * NOTE: This libc implementation does not define the routines
 * declared below.
 */

int printf(const char *restrict fmt, ...);
int snprintf(char *restrict s, size_t len, const char *restrict fmt, ...);
int sprintf(char *restrict s, const char *restrict fmt, ...);
int fprintf(FILE *restrict stream, const char *restrict format, ...);


int vprintf(const char *restrict fmt, va_list list);
int vsnprintf(char *restrict s, size_t len, const char *restrict fmt, va_list list);
int vsprintf(char *restrict s, const char *restrict fmt, va_list list);
int vfprintf(FILE *restrict stream, const char *restrict format, va_list ap);

int puts(const char *s);

int fputc(int c, FILE *stream);
int fputs(const char *restrict s, FILE *restrict stream);
size_t fwrite(const void *restrict ptr, size_t size, size_t nitems, FILE *restrict stream);

#endif /* __INC_stdio_h__ */
