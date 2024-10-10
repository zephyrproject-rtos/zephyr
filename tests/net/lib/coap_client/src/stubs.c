/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <stubs.h>

LOG_MODULE_DECLARE(coap_client_test);

DEFINE_FAKE_VALUE_FUNC(uint32_t, z_impl_sys_rand32_get);
DEFINE_FAKE_VOID_FUNC(z_impl_sys_rand_get, void *, size_t);
DEFINE_FAKE_VALUE_FUNC(ssize_t, z_impl_zsock_recvfrom, int, void *, size_t, int, struct sockaddr *,
		       socklen_t *);
DEFINE_FAKE_VALUE_FUNC(ssize_t, z_impl_zsock_sendto, int, void *, size_t, int,
		       const struct sockaddr *, socklen_t);

struct zsock_pollfd {
	int fd;
	short events;
	short revents;
};

static short my_events;

void set_socket_events(short events)
{
	my_events |= events;
}

void clear_socket_events(void)
{
	my_events = 0;
}

int z_impl_zsock_socket(int family, int type, int proto)
{
	return 0;
}

int z_impl_zsock_poll(struct zsock_pollfd *fds, int nfds, int poll_timeout)
{
	LOG_INF("Polling, events %d", my_events);
	k_sleep(K_MSEC(1));
	fds->revents = my_events;

	if (my_events) {
		return 1;
	}

	return 0;
}
