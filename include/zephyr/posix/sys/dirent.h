/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Format of directory entries.
 * @ingroup posix
 *
 * Defines the DIR directory stream type and the dirent structure that describes a
 * single directory entry.
 *
 * @posix_header{dirent.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SYS_DIRENT_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_DIRENT_H_

#include <limits.h>

#if !defined(NAME_MAX) && defined(_XOPEN_SOURCE)
/**
 * @brief Maximum number of bytes in a filename (not including the terminating null).
 */
#define NAME_MAX _XOPEN_NAME_MAX
#endif

#if !defined(NAME_MAX) && defined(_POSIX_C_SOURCE)
/**
 * @brief Maximum number of bytes in a filename (not including the terminating null).
 */
#define NAME_MAX _POSIX_NAME_MAX
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque directory stream type.
 */
typedef void DIR;

/**
 * @brief Directory entry returned by readdir().
 */
struct dirent {
	/**
	 * @brief File serial number.
	 */
	unsigned int d_ino;
	/**
	 * @brief Filename (null-terminated).
	 */
	char d_name[NAME_MAX + 1];
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_DIRENT_H_ */
