/*
 * Copyright (c) 2025 Atym, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_ZVFS_TIMERFD_H_
#define ZEPHYR_INCLUDE_ZEPHYR_ZVFS_TIMERFD_H_

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/timeutil.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZVFS_TFD_NONBLOCK 0x4000

#define ZVFS_TFD_IOC_SET_TICKS 1

typedef uint64_t zvfs_timerfd_t;

/**
 * @brief Create a file descriptor for ZVFS timer
 *
 * The returned file descriptor can be used with POSIX read calls or with
 * the @ref zvfs_timerfd_* functions.
 *
 * It also supports polling and by including an timerfd in a call to poll,
 * it is possible to signal and wake the polling thread when the timer
 * expires.
 *
 * When using read() on a ZVFS timerfd, the size must always be at least
 * 8 bytes or the operation will fail with EINVAL.
 *
 * @return New ZVFS timerfd file descriptor on success, -1 on error
 */
int zvfs_timerfd(unsigned int clockid, int flags);

/**
 * @brief Read from a ZVFS timerfd
 *
 * If call is successful, the value parameter will have the number of
 * times the timer has expired since the last read.
 *
 * @param fd File descriptor
 * @param value Pointer for storing the expired times value
 *
 * @return 0 on success, -1 on error
 */
int zvfs_timerfd_read(int fd, zvfs_timerfd_t *value);

/**
 * @brief Get the current value and period of a ZVFS timerfd.
 *
 * @param fd File descriptor
 * @param remaining Pointer for storing the remaining milliseconds
 * @param period Pointer for storing the period milliseconds
 *
 * @return 0 on success, -1 on error
 */
int zvfs_timerfd_gettime(int fd, uint32_t *remaining, uint32_t *period);

/**
 * @brief Set the value and period of a ZVFS timerfd.
 *
 * @param fd File descriptor
 * @param duration Duration to write
 * @param period Period to write
 *
 * @return 0 on success, -1 on error
 */
int zvfs_timerfd_settime(int fd, k_timeout_t duration, k_timeout_t period);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_ZVFS_TIMERFD_H_ */
