/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/zvfs/eventfd.h>
#include <stubs.h>

LOG_MODULE_DECLARE(coap_client_test, LOG_LEVEL_DBG);

DEFINE_FAKE_VALUE_FUNC(uint32_t, z_impl_sys_rand32_get);
DEFINE_FAKE_VALUE_FUNC(ssize_t, z_impl_zsock_recvfrom, int, void *, size_t, int,
		       struct net_sockaddr *, net_socklen_t *);
DEFINE_FAKE_VALUE_FUNC(ssize_t, z_impl_zsock_sendto, int, void *, size_t, int,
		       const struct net_sockaddr *, net_socklen_t);

struct zvfs_pollfd {
	int fd;
	short events;
	short revents;
};

static short my_events[NUM_FD];

void set_socket_events(int fd, short events)
{
	__ASSERT_NO_MSG(fd < NUM_FD);
	my_events[fd] |= events;
}

void clear_socket_events(int fd, short events)
{
	my_events[fd] &= ~events;
}

int z_impl_zvfs_poll(struct zvfs_pollfd *fds, int nfds, int poll_timeout)
{
	int events = 0;
	k_sleep(K_MSEC(1));
	for (int i = 0; i < nfds; i++) {
		fds[i].revents =
			my_events[fds[i].fd] & (fds[i].events | ZSOCK_POLLERR | ZSOCK_POLLHUP);
		if (fds[i].revents) {
			events++;
		}
	}
	if (events == 0) {
		k_sleep(K_MSEC(poll_timeout));
	}

	return events;
}

/* Mock of the cancel-wakeup eventfd. It is hooked into the same my_events[]
 * table as the sockets so that a write makes the poll() stub report POLLIN on
 * its fd (NUM_FD - 1), exercising the real wakeup/ack path in
 * coap_client_cancel_requests().
 */
static zvfs_eventfd_t eventfd_value;

int zvfs_eventfd(unsigned int initval, int flags)
{
	ARG_UNUSED(flags);
	eventfd_value = initval;
	return NUM_FD - 1;
}

int zvfs_eventfd_read(int fd, zvfs_eventfd_t *value)
{
	*value = eventfd_value;
	eventfd_value = 0;
	clear_socket_events(fd, ZSOCK_POLLIN);
	return 0;
}

int zvfs_eventfd_write(int fd, zvfs_eventfd_t value)
{
	eventfd_value += value;
	set_socket_events(fd, ZSOCK_POLLIN);
	return 0;
}
