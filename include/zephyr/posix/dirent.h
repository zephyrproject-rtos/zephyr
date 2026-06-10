/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Format of directory entries.
 * @ingroup posix
 *
 * Provides the DIR directory stream type together with the functions used to open,
 * read, rewind, and close directories.
 *
 * @posix_header{dirent.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_DIRENT_H_
#define ZEPHYR_INCLUDE_POSIX_DIRENT_H_

#include <zephyr/posix/sys/dirent.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (_POSIX_C_SOURCE >= 200809L) || (_XOPEN_SOURCE >= 700)
/**
 * @brief Compare two directory entries alphabetically (for use with scandir()).
 *
 * @param d1 First directory entry.
 * @param d2 Second directory entry.
 *
 * @return Negative, zero, or positive value per strcmp() semantics.
 *
 * @posix_func{alphasort}
 */
int alphasort(const struct dirent **d1, const struct dirent **d2);
#endif

/**
 * @brief Close a directory stream.
 *
 * @param dirp Directory stream to close.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{closedir}
 */
int closedir(DIR *dirp);

#if (_POSIX_C_SOURCE >= 200809L) || (_XOPEN_SOURCE >= 700)
/**
 * @brief Extract the file descriptor used by a DIR stream.
 *
 * @param dirp Directory stream.
 *
 * @return File descriptor on success, or -1 with errno set on failure.
 *
 * @posix_func{dirfd}
 */
int dirfd(DIR *dirp);
#endif

/**
 * @brief Open a directory stream for a directory identified by a file descriptor.
 *
 * @param fd File descriptor for the directory.
 *
 * @return Directory stream on success, or NULL with errno set on failure.
 *
 * @posix_func{fdopendir}
 */
DIR *fdopendir(int fd);

/**
 * @brief Open a directory stream for a named directory.
 *
 * @param dirname Path to the directory.
 *
 * @return Directory stream on success, or NULL with errno set on failure.
 *
 * @posix_func{opendir}
 */
DIR *opendir(const char *dirname);

/**
 * @brief Read the next entry from a directory stream.
 *
 * @param dirp Directory stream.
 *
 * @return Pointer to a @c struct dirent on success, or NULL at end-of-directory or on error.
 *
 * @posix_func{readdir}
 */
struct dirent *readdir(DIR *dirp);

#if (_POSIX_C_SOURCE >= 199506L) || (_XOPEN_SOURCE >= 500)
/**
 * @brief Read a directory entry into a caller-supplied buffer (thread-safe).
 *
 * @param dirp   Directory stream.
 * @param entry  Caller-supplied buffer for the entry.
 * @param result Output: pointer to @p entry, or NULL at end-of-directory.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{readdir_r}
 */
int readdir_r(DIR *ZRESTRICT dirp, struct dirent *ZRESTRICT entry,
	      struct dirent **ZRESTRICT result);
#endif

/**
 * @brief Reset the position of a directory stream to the beginning of a directory.
 *
 * @param dirp Directory stream to rewind.
 *
 * @posix_func{rewinddir}
 */
void rewinddir(DIR *dirp);

#if (_POSIX_C_SOURCE >= 200809L) || (_XOPEN_SOURCE >= 700)
/**
 * @brief Scan a directory, optionally filtering and sorting the entries.
 *
 * @param dir      Path to the directory.
 * @param namelist Output: allocated array of directory entry pointers.
 * @param sel      Filter function, or NULL to include all entries.
 * @param compar   Comparison function for sorting, or NULL for no sorting.
 *
 * @return Number of entries in @p namelist on success, or -1 on failure.
 *
 * @posix_func{scandir}
 */
int scandir(const char *dir, struct dirent ***namelist, int (*sel)(const struct dirent *),
	    int (*compar)(const struct dirent **, const struct dirent **));
#endif

#if defined(_XOPEN_SOURCE)
/**
 * @brief Set the position of a directory stream.
 *
 * @param dirp Directory stream.
 * @param loc  Position value previously returned by telldir().
 *
 * @posix_func{seekdir}
 */
void seekdir(DIR *dirp, long loc);

/**
 * @brief Get the current position of a directory stream.
 *
 * @param dirp Directory stream.
 *
 * @return Current position value, or -1 with errno set on failure.
 *
 * @posix_func{telldir}
 */
long telldir(DIR *dirp);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_DIRENT_H_ */
