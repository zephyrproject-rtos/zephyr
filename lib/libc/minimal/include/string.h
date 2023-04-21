/* string.h */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STRING_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STRING_H_

#include <stddef.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

extern char  *strcpy(char *ZRESTRICT d, const char *ZRESTRICT s);
extern char  *strerror(int errnum);
extern int   strerror_r(int errnum, char *strerrbuf, size_t buflen);
extern char  *strncpy(char *ZRESTRICT d, const char *ZRESTRICT s,
		      size_t n);
extern char  *strchr(const char *s, int c);
extern char  *strrchr(const char *s, int c);
extern char  *strchrnul(const char *s, int c);
extern size_t strlen(const char *s);
extern size_t strnlen(const char *s, size_t maxlen);
extern int    strcmp(const char *s1, const char *s2);
extern int    strncmp(const char *s1, const char *s2, size_t n);
extern char  *strtok_r(char *str, const char *sep, char **state);
extern char *strcat(char *ZRESTRICT dest,
		    const char *ZRESTRICT src);
extern char  *strncat(char *ZRESTRICT dest, const char *ZRESTRICT src,
		      size_t n);
extern char *strstr(const char *s, const char *find);

extern size_t strspn(const char *s, const char *accept);
extern size_t strcspn(const char *s, const char *reject);

extern int    memcmp(const void *m1, const void *m2, size_t n);
extern void  *memmove(void *d, const void *s, size_t n);
extern void  *memcpy(void *ZRESTRICT d, const void *ZRESTRICT s,
		     size_t n);
extern void  *memset(void *buf, int c, size_t n);
extern void  *memchr(const void *s, int c, size_t n);

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STRING_H_ */
