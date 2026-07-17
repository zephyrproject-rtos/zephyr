/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief POSIX ioctl declarations.
 * @ingroup posix
 *
 * Provides ioctl() for performing device-specific control operations on file
 * descriptors, along with the standard non-blocking and pending-read requests.
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SYS_IOCTL_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_IOCTL_H_

#include <zephyr/sys/fdtable.h>

/** Set non-blocking mode */

/**
 * @brief Set or clear non-blocking I/O mode on a file descriptor.
 */
#define FIONBIO ZFD_IOCTL_FIONBIO

/** Get the number of bytes available to read */

/**
 * @brief Get the number of bytes available to read without blocking.
 */
#define FIONREAD ZFD_IOCTL_FIONREAD

/** Get the number of bytes queued for TCP TX which have not yet been acknowledged */

/**
 * @brief Get the number of queued TCP transmit bytes not yet acknowledged.
 */
#define FIONWRITE ZFD_IOCTL_FIONWRITE

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Perform a device-specific control operation.
 *
 * @param fd      File descriptor.
 * @param request Device-dependent request code (e.g. @c FIONBIO, @c FIONREAD).
 * @param ...     Optional argument whose type depends on @p request.
 *
 * @return Request-specific non-negative value on success, or -1 with errno set on failure.
 *
 * @posix_func{ioctl}
 */
int ioctl(int fd, unsigned long request, ...);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_IOCTL_H_ */
