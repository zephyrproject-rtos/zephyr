/* stdio.h */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_stdio_h__
#define __INC_stdio_h__

#include <stdarg.h>     /* Needed to get definition of va_list */
#include <bits/restrict.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__FILE_defined)
#define __FILE_defined
typedef int  FILE;
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

int printf(const char *_Restrict fmt, ...);
int snprintf(char *_Restrict s, size_t len, const char *_Restrict fmt, ...);
int sprintf(char *_Restrict s, const char *_Restrict fmt, ...);
int fprintf(FILE *_Restrict stream, const char *_Restrict format, ...);


int vprintf(const char *_Restrict fmt, va_list list);
int vsnprintf(char *_Restrict s, size_t len, const char *_Restrict fmt, va_list list);
int vsprintf(char *_Restrict s, const char *_Restrict fmt, va_list list);
int vfprintf(FILE *_Restrict stream, const char *_Restrict format, va_list ap);

int puts(const char *s);

int fputc(int c, FILE *stream);
int fputs(const char *_Restrict s, FILE *_Restrict stream);
size_t fwrite(const void *_Restrict ptr, size_t size, size_t nitems, FILE *_Restrict stream);

#ifdef __cplusplus
}
#endif

#endif /* __INC_stdio_h__ */
