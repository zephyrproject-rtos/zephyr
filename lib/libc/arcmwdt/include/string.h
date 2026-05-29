/*
 * Copyright (c) 2025 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_LIBC_ARCMWDT_INCLUDE_STRING_H_
#define LIB_LIBC_ARCMWDT_INCLUDE_STRING_H_

#include_next <string.h>

#ifdef __cplusplus
extern "C" {
#endif

extern size_t strnlen(const char *s, size_t maxlen);

#ifdef __cplusplus
}
#endif

#endif /* LIB_LIBC_ARCMWDT_INCLUDE_STRING_H_ */
