/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SYS_DIRENT_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_DIRENT_H_

#include <limits.h>

#if !defined(NAME_MAX) && defined(_XOPEN_SOURCE)
#define NAME_MAX _XOPEN_NAME_MAX
#endif

#if !defined(NAME_MAX) && defined(_POSIX_C_SOURCE)
#define NAME_MAX _POSIX_NAME_MAX
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void DIR;

struct dirent {
	unsigned int d_ino;
	char d_name[NAME_MAX + 1];
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_DIRENT_H_ */
