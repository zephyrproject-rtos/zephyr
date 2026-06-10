/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Select types.
 * @ingroup posix
 *
 * Provides the select() and pselect() functions and the associated
 * fd_set type and macros for multiplexed I/O.
 *
 * @posix_header{sys_select.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SYS_SELECT_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_SELECT_H_

#include <zephyr/posix/posix_types.h>
#include <zephyr/sys/fdtable.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of file descriptors in an fd_set structure.
 */
#define FD_SETSIZE ZVFS_FD_SETSIZE

/**
 * @brief Set of file descriptors for select()/pselect().
 */
typedef struct zvfs_fd_set fd_set;

struct timeval;

/**
 * @brief Synchronous multiplexed I/O with signal mask and nanosecond timeout.
 *
 * @param nfds      Highest-numbered file descriptor in any set + 1.
 * @param readfds   Set of file descriptors to watch for readability, or NULL.
 * @param writefds  Set of file descriptors to watch for writability, or NULL.
 * @param exceptfds Set of file descriptors to watch for exceptions, or NULL.
 * @param timeout   Maximum wait time, or NULL to block indefinitely.
 * @param sigmask   Signal mask to apply during the wait, or NULL.
 *
 * @return Number of ready file descriptors on success, 0 on timeout,
 *         or -1 with errno set on failure.
 *
 * @posix_func{pselect}
 */
int pselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	    const struct timespec *timeout, const void *sigmask);

/**
 * @brief Synchronous multiplexed I/O (legacy interface, use pselect() for new code).
 *
 * @param nfds     Highest-numbered file descriptor in any set + 1.
 * @param readfds  Set of file descriptors to watch for readability, or NULL.
 * @param writefds Set of file descriptors to watch for writability, or NULL.
 * @param errorfds Set of file descriptors to watch for errors, or NULL.
 * @param timeout  Maximum wait time, or NULL to block indefinitely.
 *
 * @return Number of ready file descriptors on success, 0 on timeout,
 *         or -1 with errno set on failure.
 *
 * @posix_func{select}
 */
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds, struct timeval *timeout);

/**
 * @brief Remove a file descriptor from an fd_set.
 *
 * @param fd    File descriptor to clear.
 * @param fdset File descriptor set to modify.
 *
 * @posix_func{FD_CLR}
 */
void FD_CLR(int fd, fd_set *fdset);

/**
 * @brief Test whether a file descriptor is in an fd_set.
 *
 * @param fd    File descriptor to test.
 * @param fdset File descriptor set.
 *
 * @return Non-zero if @p fd is set, 0 otherwise.
 *
 * @posix_func{FD_ISSET}
 */
int FD_ISSET(int fd, fd_set *fdset);

/**
 * @brief Add a file descriptor to an fd_set.
 *
 * @param fd    File descriptor to add.
 * @param fdset File descriptor set to modify.
 *
 * @posix_func{FD_SET}
 */
void FD_SET(int fd, fd_set *fdset);

/**
 * @brief Clear all file descriptors from an fd_set.
 *
 * @param fdset File descriptor set to clear.
 *
 * @posix_func{FD_ZERO}
 */
void FD_ZERO(fd_set *fdset);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_SELECT_H_ */
