/*
 * Copyright (c) 2025 The Zephyr Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_POSIX_STDLIB_H_
#define ZEPHYR_INCLUDE_POSIX_POSIX_STDLIB_H_

#include <stddef.h> /* NULL, size_t */

#ifdef __cplusplus
extern "C" {
#endif

/* TODO: additional POSIX signatures here */

#if defined(_BSD_SOURCE) || defined(__DOXYGEN__)
int getenv_r(const char *name, char *buf, size_t len);
#endif

#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
int putenv(char *string);
#endif

#if defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__)
int setenv(const char *envname, const char *envval, int overwrite);
int unsetenv(const char *name);
#endif /* defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__) */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_POSIX_STDLIB_H_ */
