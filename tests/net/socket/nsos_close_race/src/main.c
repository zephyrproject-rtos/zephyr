/*
 * Copyright (c) 2026 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Reproducer for a use-after-free in the native offloaded sockets (nsos)
 * driver when a socket is closed while another thread is blocked polling it.
 *
 * Background
 * ----------
 * zsock_poll() on an nsos socket goes through the generic zvfs poll path
 * (ZFD_IOCTL_POLL_PREPARE / _UPDATE). PREPARE links the socket's embedded
 * poll node (&sock->poll.node) into the driver-global "nsos_polls" dlist and
 * then blocks in k_poll(). No reference is held on the socket object across
 * the wait.
 *
 * nsos_close() frees the socket object with nsi_host_free(sock) but does NOT
 * remove &sock->poll.node from nsos_polls (it only walks the list to fire the
 * POLLHUP callback). As a result, after a concurrent close the global list
 * still contains a node that points into freed heap memory.
 *
 * The next poll of any nsos socket calls sys_dlist_append(&nsos_polls, ...),
 * which dereferences the (freed) list tail to write tail->next -- a write into
 * freed memory. Built with CONFIG_ASAN=y this aborts the run.
 *
 * The scenario below is made deterministic by priority:
 *   - a low-priority poller thread parks in zsock_poll(sock_a, FOREVER),
 *     linking sock_a's node into nsos_polls;
 *   - the (higher priority) test thread closes sock_a -- freeing it and
 *     leaving its node dangling in the list -- then, without ever blocking
 *     (so the poller cannot run in between), creates sock_b and polls it.
 *     That poll's sys_dlist_append() writes through the dangling tail.
 *
 * With the bug present, ASAN reports a heap-use-after-free at the append and
 * the test fails. Once nsos_close() unlinks the poll node before freeing the
 * socket, the list no longer references freed memory and the test passes.
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>

#define POLLER_STACK_SIZE 4096

static K_THREAD_STACK_DEFINE(poller_stack, POLLER_STACK_SIZE);
static struct k_thread poller_thread;

/* Signalled by the poller right before it enters the blocking poll. */
static K_SEM_DEFINE(poller_ready, 0, 1);

static int poller_fd;
static volatile int poller_poll_ret;

static void poller_entry(void *p1, void *p2, void *p3)
{
	struct zsock_pollfd fds = {
		.fd = poller_fd,
		.events = ZSOCK_POLLIN,
	};

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Tell the test thread we are about to block in poll(). */
	k_sem_give(&poller_ready);

	/* Blocks forever (no data ever arrives) until the socket is closed
	 * from the test thread, which delivers POLLHUP and wakes us.
	 */
	poller_poll_ret = zsock_poll(&fds, 1, SYS_FOREVER_MS);
}

ZTEST(nsos_close_race, test_close_while_polling)
{
	int sock_a;
	int sock_b;
	int ret;
	int prio = k_thread_priority_get(k_current_get());

	sock_a = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(sock_a >= 0, "socket(sock_a) failed: %d", errno);

	poller_fd = sock_a;

	/* Poller runs at a strictly lower priority than this thread so it can
	 * only execute while this thread is blocked. This guarantees the
	 * ordering the reproducer relies on.
	 */
	k_thread_create(&poller_thread, poller_stack, POLLER_STACK_SIZE,
			poller_entry, NULL, NULL, NULL,
			prio + 1, 0, K_NO_WAIT);

	/* Wait until the poller is about to poll, then sleep so it actually
	 * reaches and parks inside k_poll() with sock_a's node linked into
	 * the global nsos_polls list.
	 */
	ret = k_sem_take(&poller_ready, K_SECONDS(1));
	zassert_ok(ret, "poller did not start");
	k_msleep(100);

	/* Close sock_a while the poller is blocked on it. With the bug, this
	 * frees the socket object but leaves its poll node linked in
	 * nsos_polls, dangling into freed memory. The poller is woken (made
	 * ready) but, being lower priority, does not run yet.
	 */
	ret = zsock_close(sock_a);
	zassert_ok(ret, "close(sock_a) failed: %d", errno);

	/* Still on this thread, with no blocking call in between: create and
	 * poll another nsos socket. Its poll PREPARE does
	 * sys_dlist_append(&nsos_polls, ...), writing through the dangling
	 * tail left by the close above -> heap-use-after-free under ASAN.
	 */
	sock_b = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(sock_b >= 0, "socket(sock_b) failed: %d", errno);

	struct zsock_pollfd fds = {
		.fd = sock_b,
		.events = ZSOCK_POLLIN,
	};

	/* K_NO_WAIT poll: exercises PREPARE (the faulting append) and UPDATE
	 * without blocking, so the poller still cannot preempt us.
	 */
	ret = zsock_poll(&fds, 1, 0);
	zassert_true(ret >= 0, "poll(sock_b) failed: %d", errno);

	/* If we got here the bug is fixed. Let the woken poller run to
	 * completion and clean up.
	 */
	ret = k_thread_join(&poller_thread, K_SECONDS(5));
	zassert_ok(ret, "poller did not return after sock_a was closed");

	zassert_ok(zsock_close(sock_b), "close(sock_b) failed: %d", errno);
}

ZTEST_SUITE(nsos_close_race, NULL, NULL, NULL, NULL, NULL);
