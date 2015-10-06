/* stdio.h */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
