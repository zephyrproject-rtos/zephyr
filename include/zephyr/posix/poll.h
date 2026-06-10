/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Definitions for the poll() function.
 * @ingroup posix
 *
 * Provides the poll() function and associated poll event flags for
 * multiplexed I/O on multiple file descriptors.
 *
 * @posix_header{poll.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_POLL_H_
#define ZEPHYR_INCLUDE_POSIX_POLL_H_

#include <zephyr/net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief An unsigned integer type used for the number of file descriptors.
 */
typedef	unsigned int nfds_t;

/**
 * @brief File descriptor poll structure.
 */
#define pollfd zsock_pollfd

/**
 * @brief Data other than high-priority data may be read without blocking.
 */
#define POLLIN ZSOCK_POLLIN

/**
 * @brief High priority data may be read without blocking.
 */
#define POLLPRI ZSOCK_POLLPRI

/**
 * @brief Normal data may be written without blocking.
 */
#define POLLOUT ZSOCK_POLLOUT

/**
 * @brief An error has occurred (revents only).
 */
#define POLLERR ZSOCK_POLLERR

/**
 * @brief Device has been disconnected (revents only).
 */
#define POLLHUP ZSOCK_POLLHUP

/**
 * @brief Invalid fd member (revents only).
 */
#define POLLNVAL ZSOCK_POLLNVAL

/**
 * @brief Wait for events on a set of file descriptors.
 *
 * Examines each file descriptor in @p fds for the events specified in its
 * @c events field. Blocks until at least one descriptor is ready, the
 * timeout expires, or a signal is caught.
 *
 * @param fds     Array of @c struct pollfd descriptors to monitor.
 * @param nfds    Number of entries in @p fds.
 * @param timeout Timeout in milliseconds; -1 to block indefinitely, 0 to return immediately.
 *
 * @return Number of file descriptors with non-zero @c revents on success,
 *         0 on timeout, or -1 with errno set on failure.
 *
 * @posix_func{poll}
 */
int poll(struct pollfd *fds, int nfds, int timeout);

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_POLL_H_ */
