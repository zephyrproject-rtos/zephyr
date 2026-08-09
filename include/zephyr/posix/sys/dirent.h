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
 *
 * The type is intentionally incomplete; directory streams are only ever
 * manipulated through pointers returned by opendir() and fdopendir().
 */
typedef void DIR;

/**
 * @brief Directory entry returned by readdir().
 */
struct dirent {
	unsigned int d_ino;        /**< File serial number. */
	char d_name[NAME_MAX + 1]; /**< Filename (null-terminated). */
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_DIRENT_H_ */
