/*
 * Copyright (c) 2026 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ZVFS functions for cooperation between POSIX and ISO C
 */

#ifndef ZEPHYR_INCLUDE_SYS_ZVFS_FD_FILE_H_
#define ZEPHYR_INCLUDE_SYS_ZVFS_FD_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/**
 * @brief Get pointer from file descriptor index.
 *
 * @param fd fdtable index
 * @param mode required by POSIX, currently ignored
 *
 * @return file descriptor pointer, NULL with errno set on error.
 */
FILE *zvfs_fdopen(int fd, const char *mode);

/**
 * @brief Get file descriptor index from pointer.
 *
 * @param file pointer returned by @ref zvfs_fdopen
 *
 * @return >=0 (index) on success, -1 with errno set on error.
 */
int zvfs_fileno(FILE *file);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ZVFS_FD_FILE_H_ */
