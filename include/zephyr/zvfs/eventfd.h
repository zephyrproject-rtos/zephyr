/*
 * Copyright (c) 2020 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_ZVFS_EVENTFD_H_
#define ZEPHYR_INCLUDE_ZEPHYR_ZVFS_EVENTFD_H_

#include <stdint.h>

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZVFS_EFD_SEMAPHORE 2
#define ZVFS_EFD_NONBLOCK 0x4000

typedef uint64_t zvfs_eventfd_t;

/**
 * @brief Create a file descriptor for ZVFS event notification
 *
 * The returned file descriptor can be used with POSIX read/write calls or
 * with the @ref zvfs_eventfd_read or @ref zvfs_eventfd_write functions.
 *
 * It also supports polling and by including an eventfd in a call to poll,
 * it is possible to signal and wake the polling thread by simply writing to
 * the eventfd.
 *
 * When using read() and write() on a ZVFS eventfd, the size must always be at
 * least 8 bytes or the operation will fail with EINVAL.
 *
 * @return New ZVFS eventfd file descriptor on success, -1 on error
 */
int zvfs_eventfd(unsigned int initval, int flags);

/**
 * @brief Read from a ZVFS eventfd
 *
 * If call is successful, the value parameter will have the value 1
 *
 * @param fd File descriptor
 * @param value Pointer for storing the read value
 *
 * @return 0 on success, -1 on error
 */
int zvfs_eventfd_read(int fd, zvfs_eventfd_t *value);

/**
 * @brief Write to a ZVFS eventfd
 *
 * @param fd File descriptor
 * @param value Value to write
 *
 * @return 0 on success, -1 on error
 */
int zvfs_eventfd_write(int fd, zvfs_eventfd_t value);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_ZVFS_EVENTFD_H_ */
