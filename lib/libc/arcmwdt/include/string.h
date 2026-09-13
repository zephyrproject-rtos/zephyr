/*
 * Copyright (c) 2025 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_LIBC_ARCMWDT_INCLUDE_STRING_H_
#define LIB_LIBC_ARCMWDT_INCLUDE_STRING_H_

/* Enable declarations for the Annex K functions advertised by MWDT. */
#if defined(__STDC_LIB_EXT1__) && !defined(__STDC_WANT_LIB_EXT1__)
#define __STDC_WANT_LIB_EXT1__ 1
#endif

#include_next <string.h>

#ifdef __cplusplus
extern "C" {
#endif

extern size_t strnlen(const char *s, size_t maxlen);
extern char *strsignal(int signum);

#ifdef __cplusplus
}
#endif

#endif /* LIB_LIBC_ARCMWDT_INCLUDE_STRING_H_ */
