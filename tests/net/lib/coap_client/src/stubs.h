/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STUBS_H
#define STUBS_H

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <zephyr/net/coap_client.h>


/* Copy from zephyr/include/zephyr/net/socket.h */
/**
 * @name Options for poll()
 * @{
 */
/* ZSOCK_POLL* values are compatible with Linux */
/** zsock_poll: Poll for readability */
#define ZSOCK_POLLIN 1
/** zsock_poll: Poll for exceptional condition */
#define ZSOCK_POLLPRI 2
/** zsock_poll: Poll for writability */
#define ZSOCK_POLLOUT 4
/** zsock_poll: Poll results in error condition (output value only) */
#define ZSOCK_POLLERR 8
/** zsock_poll: Poll detected closed connection (output value only) */
#define ZSOCK_POLLHUP 0x10
/** zsock_poll: Invalid socket (output value only) */
#define ZSOCK_POLLNVAL 0x20
/** @} */

#define NUM_FD 2

void set_socket_events(int fd, short events);
void clear_socket_events(int fd, short events);

DECLARE_FAKE_VALUE_FUNC(uint32_t, z_impl_sys_rand32_get);
DECLARE_FAKE_VALUE_FUNC(ssize_t, z_impl_zsock_recvfrom, int, void *, size_t, int, struct sockaddr *,
			socklen_t *);
DECLARE_FAKE_VALUE_FUNC(ssize_t, z_impl_zsock_sendto, int, void*, size_t, int,
			const struct sockaddr *, socklen_t);

#define DO_FOREACH_FAKE(FUNC)                                                                      \
	do {                                                                                       \
		FUNC(z_impl_sys_rand32_get)                                                        \
		FUNC(z_impl_zsock_recvfrom)                                                        \
		FUNC(z_impl_zsock_sendto)                                                          \
	} while (0)

#endif /* STUBS_H */
