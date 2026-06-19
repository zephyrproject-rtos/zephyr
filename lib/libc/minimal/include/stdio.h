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

typedef long fpos_t;

#if !defined(EOF)
#define EOF  (-1)
#endif

#ifndef BUFSIZ
#define BUFSIZ 512
#endif

#ifndef FOPEN_MAX
#define FOPEN_MAX 32
#endif

#ifndef FILENAME_MAX
#define FILENAME_MAX 256
#endif

#ifndef L_tmpnam
#define L_tmpnam 20
#endif

#ifndef TMP_MAX
#define TMP_MAX 32
#endif

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

#define stdin  ((FILE *) 1)
#define stdout ((FILE *) 2)
#define stderr ((FILE *) 3)

#define SEEK_SET	0	/* Seek from beginning of file.  */
#define SEEK_CUR	1	/* Seek from current position.  */
#define SEEK_END	2	/* Seek from end of file.  */

void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);

int __printf_like(1, 2) printf(const char *ZRESTRICT format, ...);
int __printf_like(3, 4) snprintf(char *ZRESTRICT str, size_t len,
				 const char *ZRESTRICT format, ...);
int __printf_like(2, 3) sprintf(char *ZRESTRICT str,
				const char *ZRESTRICT format, ...);
int __printf_like(2, 3) fprintf(FILE *ZRESTRICT stream,
				const char *ZRESTRICT format, ...);
int scanf(const char *ZRESTRICT format, ...);
int fscanf(FILE *ZRESTRICT stream, const char *ZRESTRICT format, ...);
int sscanf(const char *ZRESTRICT str, const char *ZRESTRICT format, ...);

int __printf_like(1, 0) vprintf(const char *ZRESTRICT format, va_list list);
int __printf_like(3, 0) vsnprintf(char *ZRESTRICT str, size_t len,
				  const char *ZRESTRICT format,
				  va_list list);
int __printf_like(2, 0) vsprintf(char *ZRESTRICT str,
				 const char *ZRESTRICT format, va_list list);
int __printf_like(2, 0) vfprintf(FILE *ZRESTRICT stream,
				 const char *ZRESTRICT format,
				 va_list ap);
int vscanf(const char *ZRESTRICT format, va_list ap);
int vfscanf(FILE *ZRESTRICT stream, const char *ZRESTRICT format, va_list ap);
int vsscanf(const char *ZRESTRICT str, const char *ZRESTRICT format,
	    va_list ap);

void perror(const char *s);
int puts(const char *s);

int fgetc(FILE *stream);
char *fgets(char *ZRESTRICT s, int n, FILE *ZRESTRICT stream);
int fputc(int c, FILE *stream);
int fputs(const char *ZRESTRICT s, FILE *ZRESTRICT stream);
size_t fread(void *ZRESTRICT ptr, size_t size, size_t nitems,
	     FILE *ZRESTRICT stream);
size_t fwrite(const void *ZRESTRICT ptr, size_t size, size_t nitems,
	      FILE *ZRESTRICT stream);
int ungetc(int c, FILE *stream);
#define getc(stream) fgetc(stream)
#define getchar() getc(stdin)

char *gets(char *s);

int remove(const char *path);
int rename(const char *old, const char *new);
#define putc(c, stream) fputc(c, stream)
#define putchar(c) putc(c, stdout)

FILE *fopen(const char *name, const char *mode);
FILE *freopen(const char *ZRESTRICT filename, const char *ZRESTRICT mode,
	      FILE *ZRESTRICT stream);
FILE *tmpfile(void);
char *tmpnam(char *s);
int fclose(FILE *stream);
int fflush(FILE *stream);
int fgetpos(FILE *ZRESTRICT stream, fpos_t *ZRESTRICT pos);
int fseek(FILE *stream, long offset, int whence);
int fsetpos(FILE *stream, const fpos_t *pos);
long ftell(FILE *stream);
void rewind(FILE *stream);
void setbuf(FILE *ZRESTRICT stream, char *ZRESTRICT buf);
int setvbuf(FILE *ZRESTRICT stream, char *ZRESTRICT buf, int mode, size_t size);

#ifdef __cplusplus
}
#endif

#include <zephyr/posix/posix_stdio.h>

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDIO_H_ */
