/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_PICOLIBC_INCLUDE_FCNTL_H_
#define ZEPHYR_LIB_LIBC_PICOLIBC_INCLUDE_FCNTL_H_

#include_next <fcntl.h>

#include <zephyr/sys/fdtable.h>

#if defined(F_GETFL) && F_GETFL != ZVFS_F_GETFL
#undef F_GETFL
#endif
#ifndef F_GETFL
#define F_GETFL ZVFS_F_GETFL
#endif

#if defined(F_SETFL) && F_SETFL != ZVFS_F_SETFL
#undef F_SETFL
#endif
#ifndef F_SETFL
#define F_SETFL ZVFS_F_SETFL
#endif

#if defined(O_NONBLOCK) && O_NONBLOCK != ZVFS_O_NONBLOCK
#undef O_NONBLOCK
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK ZVFS_O_NONBLOCK
#endif

#endif /* ZEPHYR_LIB_LIBC_PICOLIBC_INCLUDE_FCNTL_H_ */
