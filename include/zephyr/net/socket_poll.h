/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKET_POLL_H_
#define ZEPHYR_INCLUDE_NET_SOCKET_POLL_H_

#include <zephyr/sys/fdtable.h>

/* Setting for pollfd to avoid circular inclusion */

/**
 * @brief BSD Sockets compatible API
 * @defgroup bsd_sockets BSD Sockets compatible API
 * @ingroup networking
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __DOXYGEN__
/**
 * @brief Definition of the monitored socket/file descriptor.
 *
 * An array of these descriptors is passed as an argument to poll().
 */
struct zsock_pollfd {
	int fd;        /**< Socket descriptor */
	short events;  /**< Requested events */
	short revents; /**< Returned events */
};
#else
#define zsock_pollfd zvfs_pollfd
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_POLL_H_ */
