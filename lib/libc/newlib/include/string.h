/*
 * Copyright (c) 2023 Intel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_NEWLIB_INCLUDE_STRING_H_
#define ZEPHYR_LIB_LIBC_NEWLIB_INCLUDE_STRING_H_

#include <stddef.h>

#include_next <string.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t strnlen(const char *, size_t);
char  *strtok_r(char *, const char *, char **);

/* define strerror_r as the posix version */
int __xpg_strerror_r (int, char *, size_t);
#define strerror_r __xpg_strerror_r

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_LIB_LIBC_NEWLIB_INCLUDE_STRING_H_ */
