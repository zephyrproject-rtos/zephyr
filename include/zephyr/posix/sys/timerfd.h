/*
 * Copyright (c) 2025 Atym, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SYS_TIMERFD_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_TIMERFD_H_

#include <zephyr/zvfs/timerfd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TFD_NONBLOCK  ZVFS_TFD_NONBLOCK

#define TFD_IOC_SET_TICKS ZVFS_TFD_IOC_SET_TICKS

#define TFD_TIMER_ABSTIME 1

typedef zvfs_timerfd_t timerfd_t;

/**
 * @brief Create a file descriptor for ZVFS timer notification
 *
 * The returned file descriptor can be used with POSIX read calls or
 * with the @ref zvfs_timerfd_settime or @ref zvfs_timerfd_gettime functions.
 *
 * It also supports polling and by including an timerfd in a call to poll,
 * it is possible to signal and wake the polling thread by simply writing to
 * the timerfd.
 *
 * When using read() on a ZVFS timerfd, the size must always be at
 * least 8 bytes or the operation will fail with EINVAL.
 *
 * @return New ZVFS timerfd file descriptor on success, -1 on error
 */
int timerfd_create(int clockid, int flags);

/**
 * @brief Get the itimespec configuration ZVFS timerfd
 *
 * @param fd File descriptor
 * @param curr_value Pointer to struct itimerspec to store current value
 *
 * @return 0 on success, -1 on error
 */
int timerfd_gettime(int fd, struct itimerspec *curr_value);

/**
 * @brief Set the itimespec configuration ZVFS timerfd
 *
 * The current value is stored in old_value if it is not NULL.
 *
 * @param fd File descriptor
 * @param new_value Pointer to struct itimerspec to set new value
 * @param old_value Pointer to struct itimerspec to store old value
 *
 * @return 0 on success, -1 on error
 */
int timerfd_settime(int fd, int flags, const struct itimerspec *new_value,
    struct itimerspec *old_value);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_TIMERFD_H_ */
