/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief File control options.
 * @ingroup posix
 *
 * Defines the file-control commands, file access mode flags, and file status flags
 * used with open() and fcntl().
 *
 * @posix_header{fcntl.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_FCNTL_H_
#define ZEPHYR_INCLUDE_POSIX_FCNTL_H_

#include <zephyr/sys/fdtable.h>

#define O_APPEND   ZVFS_O_APPEND /**< Write operations append to the end of the file. */

#define O_CREAT    ZVFS_O_CREAT  /**< Create file if it does not exist. */

#define O_EXCL     ZVFS_O_EXCL   /**< Exclusive creation: fail if the file already exists. */

#define O_NONBLOCK ZVFS_O_NONBLOCK /**< Non-blocking I/O mode. */

#define O_TRUNC    ZVFS_O_TRUNC  /**< Truncate the file to zero length on open. */

#define O_ACCMODE (ZVFS_O_RDONLY | ZVFS_O_RDWR | ZVFS_O_WRONLY) /**< File access mode mask. */

#define O_RDONLY ZVFS_O_RDONLY   /**< Open for reading only. */

#define O_RDWR   ZVFS_O_RDWR     /**< Open for reading and writing. */

#define O_WRONLY ZVFS_O_WRONLY   /**< Open for writing only. */

#define F_DUPFD ZVFS_F_DUPFD     /**< Duplicate a file descriptor to the lowest available number. */

#define F_GETFL ZVFS_F_GETFL     /**< Get file status flags and file access modes. */

#define F_SETFL ZVFS_F_SETFL     /**< Set file status flags. */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Open or create a file.
 *
 * @param name  Pathname of the file.
 * @param flags Access mode and creation flags (@c O_RDONLY, @c O_WRONLY, @c O_RDWR,
 *              @c O_CREAT, ...).
 * @param ...   Permission bits of type @c mode_t (required when @c O_CREAT is given).
 *
 * @return New file descriptor on success, or -1 with errno set on failure.
 *
 * @posix_api{POSIX_DEVICE_IO,open}
 */
int open(const char *name, int flags, ...);

/**
 * @brief Perform file control operations on an open file descriptor.
 *
 * @param fildes File descriptor.
 * @param cmd    Control command (@c F_DUPFD, @c F_GETFL, @c F_SETFL, ...).
 * @param ...    Optional argument whose type depends on @p cmd.
 *
 * @return Command-specific value on success, or -1 with errno set on failure.
 *
 * @posix_api{POSIX_FD_MGMT,fcntl}
 */
int fcntl(int fildes, int cmd, ...);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_FCNTL_H_ */
