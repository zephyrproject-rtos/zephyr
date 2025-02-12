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

#define ZSOCK_POLLIN  1
#define ZSOCK_POLLOUT 4

void set_socket_events(short events);
void clear_socket_events(void);

DECLARE_FAKE_VALUE_FUNC(uint32_t, z_impl_sys_rand32_get);
DECLARE_FAKE_VOID_FUNC(z_impl_sys_rand_get, void *, size_t);
DECLARE_FAKE_VALUE_FUNC(ssize_t, z_impl_zsock_recvfrom, int, void *, size_t, int, struct sockaddr *,
			socklen_t *);
DECLARE_FAKE_VALUE_FUNC(ssize_t, z_impl_zsock_sendto, int, void*, size_t, int,
			const struct sockaddr *, socklen_t);

#define DO_FOREACH_FAKE(FUNC)                                                                      \
	do {                                                                                       \
		FUNC(z_impl_sys_rand32_get)                                                        \
		FUNC(z_impl_sys_rand_get)                                                          \
		FUNC(z_impl_zsock_recvfrom)                                                        \
		FUNC(z_impl_zsock_sendto)                                                          \
	} while (0)

#endif /* STUBS_H */
