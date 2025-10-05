/*
 * Copyright (c) 2025 The Zephyr Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_POSIX_STRING_H_
#define ZEPHYR_INCLUDE_POSIX_POSIX_STRING_H_

#if defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__)

#include <stddef.h> /* NULL, size_t */

#ifdef __cplusplus
extern "C" {
#endif

/* TODO: additional POSIX signatures here */

char *strsignal(int signo);

#ifdef __cplusplus
}
#endif

#endif /* defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__) */

#endif /* ZEPHYR_INCLUDE_POSIX_POSIX_STRING_H_ */
