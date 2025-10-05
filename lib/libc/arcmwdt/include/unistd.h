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

int getentropy(void *buffer, size_t length);

#if _POSIX_C_SOURCE >= 2
#include <zephyr/posix/sys/confstr.h>
#endif

/* Note: usleep() was declared obsolescent as of POSIX.1-2001 and removed from POSIX.1-2008 */
int usleep(__useconds_t __useconds);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_LIBC_ARCMWDT_INCLUDE_UNISTD_H_ */
