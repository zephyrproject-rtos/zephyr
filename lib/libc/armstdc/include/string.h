/*
 * Copyright (c) 2023 Intel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_STRING_H_
#define ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_STRING_H_

#include <stddef.h>
#include <zephyr/toolchain.h>

#include_next <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#if _POSIX_C_SOURCE >= 200809L
extern size_t strnlen(const char *s, size_t maxlen);
#endif

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_STRING_H_ */
