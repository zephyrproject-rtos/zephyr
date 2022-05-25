/* stdlib.h */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDLIB_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDLIB_H_

#include <stddef.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

unsigned long strtoul(const char *nptr, char **endptr, int base);
long strtol(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
long long strtoll(const char *nptr, char **endptr, int base);
int atoi(const char *s);

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void *reallocarray(void *ptr, size_t nmemb, size_t size);

void *bsearch(const void *key, const void *array,
	      size_t count, size_t size,
	      int (*cmp)(const void *key, const void *element));

void qsort_r(void *base, size_t nmemb, size_t size,
	     int (*compar)(const void *, const void *, void *), void *arg);
void qsort(void *base, size_t nmemb, size_t size,
	   int (*compar)(const void *, const void *));

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
void _exit(int status);
static inline void exit(int status)
{
	_exit(status);
}
void abort(void);

#ifdef CONFIG_MINIMAL_LIBC_RAND
#define RAND_MAX INT_MAX
int rand_r(unsigned int *seed);
int rand(void);
void srand(unsigned int seed);
#endif /* CONFIG_MINIMAL_LIBC_RAND */

static inline int abs(int __n)
{
	return (__n < 0) ? -__n : __n;
}

static inline long labs(long __n)
{
	return (__n < 0L) ? -__n : __n;
}

static inline long long llabs(long long __n)
{
	return (__n < 0LL) ? -__n : __n;
}

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDLIB_H_ */
