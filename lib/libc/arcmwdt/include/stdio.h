/*
 * Copyright (c) 2024 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_LIBC_ARCMWDT_INCLUDE_STDIO_H_
#define LIB_LIBC_ARCMWDT_INCLUDE_STDIO_H_

#include_next <stdio.h>

#ifdef fileno
#undef fileno
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int fileno(FILE *file);

#ifdef __cplusplus
}
#endif

#endif /* LIB_LIBC_ARCMWDT_INCLUDE_STDIO_H_ */
