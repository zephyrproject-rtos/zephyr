/* string.h */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_string_h__
#define __INC_string_h__

#include <stddef.h>
#include <bits/restrict.h>

#ifdef __cplusplus
extern "C" {
#endif

extern char  *strcpy(char *_Restrict d, const char *_Restrict s);
extern char  *strncpy(char *_Restrict d, const char *_Restrict s, size_t n);
extern char  *strchr(const char *s, int c);
extern char  *strrchr(const char *s, int c);
extern size_t strlen(const char *s);
extern int    strcmp(const char *s1, const char *s2);
extern int    strncmp(const char *s1, const char *s2, size_t n);
extern char *strcat(char *_Restrict dest, const char *_Restrict src);
extern char  *strncat(char *_Restrict d, const char *_Restrict s, size_t n);
extern char *strstr(const char *s, const char *find);

extern int    memcmp(const void *m1, const void *m2, size_t n);
extern void  *memmove(void *d, const void *s, size_t n);
extern void  *memcpy(void *_Restrict d, const void *_Restrict s, size_t n);
extern void  *memset(void *buf, int c, size_t n);
extern void  *memchr(const void *s, unsigned char c, size_t n);

#ifdef __cplusplus
}
#endif

#endif  /* __INC_string_h__ */
