/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_STRING_H_
#define ZEPHYR_INCLUDE_STRING_H_

#include <zephyr/types.h>

void    *memchr(const void *, int, size_t);
int      memcmp(const void *, const void *, size_t);
void    *memcpy(void *restrict, const void *restrict, size_t);
void    *memmove(void *, const void *, size_t);
void    *memset(void *, int, size_t);
char    *strcat(char *restrict, const char *restrict);
char    *strchr(const char *, int);
int      strcmp(const char *, const char *);
char    *strcpy(char *restrict, const char *restrict);
size_t   strcspn(const char *, const char *);
char    *strerror(int);
size_t   strlen(const char *);
char    *strncat(char *restrict, const char *restrict, size_t);
int      strncmp(const char *, const char *, size_t);
char    *strncpy(char *restrict, const char *restrict, size_t);
size_t   strnlen(const char *, size_t);
char    *strrchr(const char *, int);
size_t   strspn(const char *, const char *);
char    *strstr(const char *, const char *);

#endif /* ZEPHYR_INCLUDE_STRING_H_ */
