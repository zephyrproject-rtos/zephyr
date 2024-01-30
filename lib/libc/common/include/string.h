/*
 * Copyright (c) 2024, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_COMMON_INCLUDE_STRING_H_
#define ZEPHYR_LIB_LIBC_COMMON_INCLUDE_STRING_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if _POSIX_C_SOURCE >= 200809L || defined(_ZEPHYR_SOURCE)
size_t strnlen(const char *s, size_t maxlen);
#endif

#if _POSIX_C_SOURCE >= 200112L || defined(_ZEPHYR_SOURCE)
char *strtok_r(char *str, const char *sep, char **state);
#endif

#ifdef __cplusplus
}
#endif

#include_next <string.h>

#endif /* ZEPHYR_LIB_LIBC_COMMON_INCLUDE_STRING_H_ */
