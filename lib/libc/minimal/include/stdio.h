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

#define SEEK_SET	0	/* Seek from beginning of file.  */
#define SEEK_CUR	1	/* Seek from current position.  */
#define SEEK_END	2	/* Seek from end of file.  */

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
/**
 * @brief Open file with given name and mode.
 *
 * @param filename Name of file
 * @param mode String declaring more details, as defined in the C standard
 *
 * @return FILE* pointer to be used with other C functions on success,
 * NULL with errno set on failure
 */
FILE *fopen(const char *ZRESTRICT filename, const char *ZRESTRICT mode);
/**
 * @brief Close stream.
 *
 * @param stream Pointer to stream to close; invalid after this function returns
 *
 * @return 0 on success; EOF with errno set on failure
 */
int fclose(FILE *stream);
int fputc(int c, FILE *stream);
/**
 * @brief Place file position indicator.
 *
 * @param stream Stream to work on
 * @param offset Offset from position specified by @a whence
 * @param whence Reference for @a offset : SEEK_SET (beginning), SEEK_CUR (current position)
 * or SEEK_END (end)
 *
 * @return 0 on success; EOF with errno set on failure
 */
int fseek(FILE *stream, long offset, int whence);
/**
 * @brief Change name of file or directory.
 *
 * @param old Prior name
 * @param newp New name
 *
 * @return 0 on success; -1 with errno set on failure
 */
int rename(const char *old, const char *newp);
int remove(const char *path);
#define putc(c, stream) fputc(c, stream)
#define putchar(c) putc(c, stdout)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDIO_H_ */
