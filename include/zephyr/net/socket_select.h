/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file socket_select.h
 *
 * @brief BSD select support functions.
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKET_SELECT_H_
#define ZEPHYR_INCLUDE_NET_SOCKET_SELECT_H_

/**
 * @brief BSD Sockets compatible API
 * @defgroup bsd_sockets BSD Sockets compatible API
 * @ingroup networking
 * @{
 */

#include <time.h>

#include <zephyr/toolchain.h>
#include <zephyr/net/socket_types.h>
#include <zephyr/sys/fdtable.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Socket file descriptor set. */
typedef struct zvfs_fd_set zsock_fd_set;

/**
 * @brief Legacy function to poll multiple sockets for events
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/select.html
 * for normative description. This function is provided to ease porting of
 * existing code and not recommended for usage due to its inefficiency,
 * use zsock_poll() instead. In Zephyr this function works only with
 * sockets, not arbitrary file descriptors.
 * This function is also exposed as `select()`
 * if @kconfig{CONFIG_POSIX_DEVICE_IO} is defined (in which case
 * it may conflict with generic POSIX `select()` function).
 */
static inline int zsock_select(int nfds, zsock_fd_set *readfds, zsock_fd_set *writefds,
			       zsock_fd_set *exceptfds, struct zsock_timeval *timeout)
{
	struct timespec to = {
		.tv_sec = (timeout == NULL) ? 0 : timeout->tv_sec,
		.tv_nsec = (long)((timeout == NULL) ? 0 : timeout->tv_usec * NSEC_PER_USEC)};

	return zvfs_select(nfds, readfds, writefds, exceptfds, (timeout == NULL) ? NULL : &to,
			   NULL);
}

/** Number of file descriptors which can be added to zsock_fd_set */
#define ZSOCK_FD_SETSIZE ZVFS_FD_SETSIZE

/**
 * @brief Initialize (clear) fd_set
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/select.html
 * for normative description.
 * This function is also exposed as `FD_ZERO()`
 * if @kconfig{CONFIG_POSIX_DEVICE_IO} is defined.
 */
static inline void ZSOCK_FD_ZERO(zsock_fd_set *set)
{
	ZVFS_FD_ZERO(set);
}

/**
 * @brief Check whether socket is a member of fd_set
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/select.html
 * for normative description.
 * This function is also exposed as `FD_ISSET()`
 * if @kconfig{CONFIG_POSIX_DEVICE_IO} is defined.
 */
static inline int ZSOCK_FD_ISSET(int fd, zsock_fd_set *set)
{
	return ZVFS_FD_ISSET(fd, set);
}

/**
 * @brief Remove socket from fd_set
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/select.html
 * for normative description.
 * This function is also exposed as `FD_CLR()`
 * if @kconfig{CONFIG_POSIX_DEVICE_IO} is defined.
 */
static inline void ZSOCK_FD_CLR(int fd, zsock_fd_set *set)
{
	ZVFS_FD_CLR(fd, set);
}

/**
 * @brief Add socket to fd_set
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/select.html
 * for normative description.
 * This function is also exposed as `FD_SET()`
 * if @kconfig{CONFIG_POSIX_DEVICE_IO} is defined.
 */
static inline void ZSOCK_FD_SET(int fd, zsock_fd_set *set)
{
	ZVFS_FD_SET(fd, set);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_SELECT_H_ */
