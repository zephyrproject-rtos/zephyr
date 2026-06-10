/*
 * Copyright (c) 2020 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linux-compatible eventfd declarations.
 * @ingroup posix_extensions
 *
 * An eventfd is a lightweight, kernel-managed counter that can be used for
 * event notification between threads. It integrates with poll() and select().
 *
 * @compat_api{This header provides non-POSIX compatibility interfaces.}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SYS_EVENTFD_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_EVENTFD_H_

#include <zephyr/zvfs/eventfd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Semaphore mode: each read decrements the counter by 1 instead of resetting it to 0.
 * @ingroup posix_extensions
 *
 * @compat_api{EFD_SEMAPHORE}
 */
#define EFD_SEMAPHORE ZVFS_EFD_SEMAPHORE

/**
 * @brief Non-blocking mode: reads and writes fail with @c EAGAIN instead of blocking.
 * @ingroup posix_extensions
 *
 * @compat_api{EFD_NONBLOCK}
 */
#define EFD_NONBLOCK  ZVFS_EFD_NONBLOCK

/**
 * @brief Counter value type for eventfd operations.
 * @ingroup posix_extensions
 *
 * @compat_api{eventfd_t}
 */
typedef zvfs_eventfd_t eventfd_t;

/**
 * @brief Create a file descriptor for event notification
 *
 * The returned file descriptor can be used with POSIX read/write calls or
 * with the eventfd_read/eventfd_write functions.
 *
 * It also supports polling and by including an eventfd in a call to poll,
 * it is possible to signal and wake the polling thread by simply writing to
 * the eventfd.
 *
 * When using read() and write() on an eventfd, the size must always be at
 * least 8 bytes or the operation will fail with @c EINVAL.
 *
 * @param initval Initial value of the eventfd counter.
 * @param flags   0, or a combination of @c EFD_SEMAPHORE and @c EFD_NONBLOCK.
 *
 * @return New eventfd file descriptor on success, -1 on error
 * @ingroup posix_extensions
 * @compat_api{eventfd}
 */
int eventfd(unsigned int initval, int flags);

/**
 * @brief Read from an eventfd
 *
 * In normal mode the current counter value is stored in @p value and the
 * counter is reset to 0. In @c EFD_SEMAPHORE mode, @p value receives 1 and
 * the counter is decremented by 1.
 *
 * @param fd File descriptor
 * @param value Pointer for storing the read value
 *
 * @return 0 on success, -1 on error
 * @ingroup posix_extensions
 * @compat_api{eventfd_read}
 */
int eventfd_read(int fd, eventfd_t *value);

/**
 * @brief Write to an eventfd
 *
 * @param fd File descriptor
 * @param value Value to write
 *
 * @return 0 on success, -1 on error
 * @ingroup posix_extensions
 * @compat_api{eventfd_write}
 */
int eventfd_write(int fd, eventfd_t value);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_EVENTFD_H_ */
