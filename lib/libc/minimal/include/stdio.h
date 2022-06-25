/* stdio.h */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDIO_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDIO_H_

#include <zephyr/toolchain.h>
#include <stdarg.h>     /* Needed to get definition of va_list */
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

int __printf_like(1, 2) printf(const char *ZRESTRICT format, ...);
int __printf_like(3, 4) snprintf(char *ZRESTRICT str, size_t len,
				 const char *ZRESTRICT format, ...);
int __printf_like(2, 3) sprintf(char *ZRESTRICT str,
				const char *ZRESTRICT format, ...);
int __printf_like(2, 3) fprintf(FILE *ZRESTRICT stream,
				const char *ZRESTRICT format, ...);


int __printf_like(1, 0) vprintf(const char *ZRESTRICT format, va_list list);
int __printf_like(3, 0) vsnprintf(char *ZRESTRICT str, size_t len,
				  const char *ZRESTRICT format,
				  va_list list);
int __printf_like(2, 0) vsprintf(char *ZRESTRICT str,
				 const char *ZRESTRICT format, va_list list);
int __printf_like(2, 0) vfprintf(FILE *ZRESTRICT stream,
				 const char *ZRESTRICT format,
				 va_list ap);

void perror(const char *s);
int puts(const char *s);

int fputc(int c, FILE *stream);
int fputs(const char *ZRESTRICT s, FILE *ZRESTRICT stream);
size_t fwrite(const void *ZRESTRICT ptr, size_t size, size_t nitems,
	      FILE *ZRESTRICT stream);
static inline int putc(int c, FILE *stream)
{
	return fputc(c, stream);
}
static inline int putchar(int c)
{
	return putc(c, stdout);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDIO_H_ */
