/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_NEWLIB_INCLUDE_SYS_STAT_H_
#define ZEPHYR_LIB_LIBC_NEWLIB_INCLUDE_SYS_STAT_H_

#include_next <sys/stat.h>

#if defined(_XOPEN_SOURCE)

#include <zephyr/sys/fdtable.h>

#define S_IFSHM ZVFS_MODE_IFSHM

#define S_TYPEISMQ(buf)  (0)
#define S_TYPEISSEM(buf) (0)
#define S_TYPEISSHM(st)  (((st)->st_mode & S_IFMT) == S_IFSHM)
#endif

#endif /* ZEPHYR_LIB_LIBC_NEWLIB_INCLUDE_SYS_STAT_H_ */
