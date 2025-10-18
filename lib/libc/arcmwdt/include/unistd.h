/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_ARCMWDT_INCLUDE_UNISTD_H_
#define ZEPHYR_LIB_LIBC_ARCMWDT_INCLUDE_UNISTD_H_

#include_next <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_XOPEN_SOURCE)
long gethostid(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_LIBC_ARCMWDT_INCLUDE_UNISTD_H_ */
