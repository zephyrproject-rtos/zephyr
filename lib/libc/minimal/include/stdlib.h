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

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

unsigned long strtoul(const char *nptr, char **endptr, int base);
long strtol(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
long long strtoll(const char *nptr, char **endptr, int base);
int atoi(const char *s);

void *malloc(size_t size);
void *aligned_alloc(size_t alignment, size_t size); /* From C11 */

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
FUNC_NORETURN void _exit(int status);
FUNC_NORETURN static inline void exit(int status)
{
	_exit(status);
}
FUNC_NORETURN void abort(void);

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

char *getenv(const char *name);
#if _POSIX_C_SOURCE >= 200112L
int setenv(const char *name, const char *val, int overwrite);
int unsetenv(const char *name);
#endif

#ifdef _BSD_SOURCE
int getenv_r(const char *name, char *buf, size_t len);
#endif

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDLIB_H_ */
