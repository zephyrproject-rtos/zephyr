/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#define STACK_SIZE 4096

K_THREAD_STACK_DEFINE(closed_poll_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(other_poll_stack, STACK_SIZE);

static struct k_thread closed_poll_thread;
static struct k_thread other_poll_thread;

static int udp_socket(void)
{
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_addr = NET_INADDR_LOOPBACK_INIT,
	};
	int sock;

	sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(sock >= 0, "socket() failed (%d)", errno);
	zassert_ok(zsock_bind(sock, (struct net_sockaddr *)&addr, sizeof(addr)));

	return sock;
}

static void poll_in(void *p1, void *p2, void *p3)
{
	struct zsock_pollfd pfd = {
		.fd = POINTER_TO_INT(p1),
		.events = ZSOCK_POLLIN,
	};

	ARG_UNUSED(p3);

	(void)zsock_poll(&pfd, 1, POINTER_TO_INT(p2));
}

/*
 * Replace a socket while another thread polls it, as the DNS resolver does
 * with CONFIG_DNS_RESOLVER_RANDOMIZE_SOURCE_PORT. The replacement gets the
 * same descriptor numbers and, from the host allocator, the same memory as the
 * closed socket. A second poll finishing first must not touch the closed
 * socket's poll registration through that memory, and the first poll must
 * still return cleanly.
 */
ZTEST(nsos, test_close_while_polled)
{
	int closed, other, replacement;

	closed = udp_socket();
	other = udp_socket();

	/* The poll on the socket to be closed is registered first */
	k_thread_create(&closed_poll_thread, closed_poll_stack, STACK_SIZE, poll_in,
			INT_TO_POINTER(closed), INT_TO_POINTER(-1), NULL,
			K_PRIO_PREEMPT(2), 0, K_NO_WAIT);
	k_sleep(K_MSEC(10));
	k_thread_create(&other_poll_thread, other_poll_stack, STACK_SIZE, poll_in,
			INT_TO_POINTER(other), INT_TO_POINTER(50), NULL,
			K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	k_sleep(K_MSEC(10));

	/*
	 * Let the other poll time out before either poller runs, so that the
	 * higher priority other poller completes its poll first.
	 */
	k_sched_lock();
	zassert_ok(zsock_close(closed));
	replacement = udp_socket();
	k_busy_wait(100 * USEC_PER_MSEC);
	k_sched_unlock();

	zassert_ok(k_thread_join(&other_poll_thread, K_SECONDS(1)));
	zassert_ok(k_thread_join(&closed_poll_thread, K_SECONDS(1)));

	zassert_ok(zsock_close(replacement));
	zassert_ok(zsock_close(other));
}

ZTEST_SUITE(nsos, NULL, NULL, NULL, NULL, NULL);
