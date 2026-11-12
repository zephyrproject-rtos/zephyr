/*
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/ztest_assert.h>

#include <zephyr/net/socket_service.h>

#include "../../socket_helpers.h"

#define BUF_AND_SIZE(buf) buf, sizeof(buf) - 1
#define STRLEN(buf) (sizeof(buf) - 1)

#define TEST_STR_SMALL "test"

#define MY_IPV6_ADDR "::1"

#define ANY_PORT 0
#define SERVER_PORT 4242
#define CLIENT_PORT 9898

#define TCP_TEARDOWN_TIMEOUT K_SECONDS(3)

K_SEM_DEFINE(wait_data, 0, UINT_MAX);
K_SEM_DEFINE(wait_data_tcp, 0, UINT_MAX);
#define WAIT_TIME 500

static void server_handler(struct net_socket_service_event *pev)
{
	ARG_UNUSED(pev);

	k_sem_give(&wait_data);
}

static void tcp_server_handler(struct net_socket_service_event *pev)
{
	ARG_UNUSED(pev);

	k_sem_give(&wait_data_tcp);

	k_yield();

	Z_SPIN_DELAY(100);
}

NET_SOCKET_SERVICE_SYNC_DEFINE(udp_service_sync, server_handler, 2);
NET_SOCKET_SERVICE_SYNC_DEFINE(tcp_service_small_sync, tcp_server_handler, 1);
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(tcp_service_sync, tcp_server_handler, 2);


void run_test_service(const struct net_socket_service_desc *udp_service,
		      const struct net_socket_service_desc *tcp_service_small,
		      const struct net_socket_service_desc *tcp_service)
{
	int ret;
	int c_sock_udp;
	int s_sock_udp;
	int c_sock_tcp;
	int s_sock_tcp;
	int new_sock;
	struct net_sockaddr_in6 c_addr;
	struct net_sockaddr_in6 s_addr;
	ssize_t len;
	char buf[10];
	struct zsock_pollfd sock[2] = {
		[0] = { .fd = -1 },
		[1] = { .fd = -1 },
	};

	prepare_sock_udp_v6(MY_IPV6_ADDR, CLIENT_PORT, &c_sock_udp, &c_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock_udp, &s_addr);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, CLIENT_PORT, &c_sock_tcp, &c_addr);
	prepare_sock_tcp_v6(MY_IPV6_ADDR, SERVER_PORT, &s_sock_tcp, &s_addr);

	sock[0].fd = s_sock_udp;
	sock[0].events = ZSOCK_POLLIN;

	ret = net_socket_service_register(udp_service, sock, ARRAY_SIZE(sock), NULL);
	zassert_equal(ret, 0, "Cannot register udp service (%d)", ret);

	sock[0].fd = s_sock_tcp;
	sock[0].events = ZSOCK_POLLIN;

	ret = net_socket_service_register(tcp_service_small, sock, ARRAY_SIZE(sock) + 1, NULL);
	zassert_equal(ret, -ENOMEM, "Could register tcp service (%d)", ret);

	ret = net_socket_service_register(tcp_service, sock, ARRAY_SIZE(sock), NULL);
	zassert_equal(ret, 0, "Cannot register tcp service (%d)", ret);

	ret = bind(s_sock_udp, (struct net_sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(ret, 0, "bind failed");

	ret = connect(c_sock_udp, (struct net_sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(ret, 0, "connect failed");

	/* Send pkt for s_sock_udp and poll with timeout of 10 */
	len = send(c_sock_udp, BUF_AND_SIZE(TEST_STR_SMALL), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid send len");

	if (k_sem_take(&wait_data, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting callback");
	}

	/* Recv pkt from s_sock_udp and ensure no poll events happen */
	len = recv(s_sock_udp, BUF_AND_SIZE(buf), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid recv len");

	ret = bind(s_sock_tcp, (struct net_sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(ret, 0, "bind failed (%d)", -errno);
	ret = listen(s_sock_tcp, 0);
	zassert_equal(ret, 0, "");

	ret = connect(c_sock_tcp, (const struct net_sockaddr *)&s_addr,
		      sizeof(s_addr));
	zassert_equal(ret, 0, "");

	/* Let the network stack run */
	k_msleep(10);

	len = send(c_sock_tcp, BUF_AND_SIZE(TEST_STR_SMALL), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid send len");

	if (k_sem_take(&wait_data_tcp, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting callback");
	}

	new_sock = accept(s_sock_tcp, NULL, NULL);
	zassert_true(new_sock >= 0, "");

	sock[1].fd = new_sock;
	sock[1].events = ZSOCK_POLLIN;

	ret = net_socket_service_register(tcp_service, sock, ARRAY_SIZE(sock), NULL);
	zassert_equal(ret, 0, "Cannot register tcp service (%d)", ret);

	if (k_sem_take(&wait_data_tcp, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting callback");
	}

	len = recv(new_sock, BUF_AND_SIZE(buf), 0);
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid recv len");

	ret = net_socket_service_unregister(tcp_service);
	zassert_equal(ret, 0, "Cannot unregister tcp service (%d)", ret);

	ret = net_socket_service_close(udp_service);
	zassert_equal(ret, 0, "Cannot unregister udp service (%d)", ret);

	ret = net_socket_service_unregister(tcp_service_small);
	zassert_equal(ret, 0, "Cannot unregister tcp service (%d)", ret);

	ret = close(new_sock);
	zassert_equal(ret, 0, "close failed");

	ret = close(c_sock_tcp);
	zassert_equal(ret, 0, "close failed");

	ret = close(s_sock_tcp);
	zassert_equal(ret, 0, "close failed");

	ret = close(c_sock_udp);
	zassert_equal(ret, 0, "close failed");

	/* Small sleep to allow socket service to action the close itself` */
	k_sleep(K_MSEC(10));

	ret = close(s_sock_udp);
	zassert_not_equal(ret, 0, "socket not automatically closed");

	/* Let the stack close the TCP sockets properly */
	k_msleep(100);
}

ZTEST(net_socket_service, test_service_sync)
{
	run_test_service(&udp_service_sync, &tcp_service_small_sync,
			 &tcp_service_sync);
}

/* Verify that a callback reconfiguring its own service (deferred close,
 * re-register, unregister) does not make the service thread reuse stale poll
 * state. Deferred close and event-mask changes are observable with real UDP
 * sockets: an unread loopback datagram keeps the socket level-triggered
 * readable, so a stale re-armed poll entry causes an extra callback and
 * discards the pending reconfiguration. Unregister and fd replacement leave
 * no socket-visible trace (the stale entry resolves silently inside the
 * service thread), so those two tests use a mock fd whose vtable counts
 * ZFD_IOCTL_POLL_PREPARE calls.
 */
#define RECONFIG_TIMEOUT        K_MSEC(500)
#define RECONFIG_CLOSE_FD_COUNT 2
#define RECONFIG_CLOSED_WAIT_MS 1000
#define RECONFIG_CLOSE_PORT     4243
#define RECONFIG_MULTI_PORT     4244
#define RECONFIG_EVENTS_PORT    4246

struct reconfig_mock_fd {
	struct k_sem readable;
	struct k_sem prepared;
	struct k_sem closed;
	struct k_sem wrong_events;
	short expected_events;
};

static void reconfig_mock_init(struct reconfig_mock_fd *mock)
{
	k_sem_init(&mock->readable, 0, 1);
	k_sem_init(&mock->prepared, 0, UINT_MAX);
	k_sem_init(&mock->closed, 0, 1);
	k_sem_init(&mock->wrong_events, 0, 1);
	mock->expected_events = -1;
}

static int reconfig_mock_close(void *obj)
{
	struct reconfig_mock_fd *mock = obj;

	k_sem_give(&mock->closed);

	return 0;
}

TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS)
static int reconfig_mock_ioctl(void *obj, unsigned int request, va_list args)
{
	struct reconfig_mock_fd *mock = obj;
	struct zsock_pollfd *pfd;
	struct k_poll_event **pev;

	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE: {
		struct k_poll_event *pev_end;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);

		if ((mock->expected_events >= 0) && (pfd->events != mock->expected_events)) {
			k_sem_give(&mock->wrong_events);
		}

		if (*pev == pev_end) {
			return -ENOMEM;
		}

		k_poll_event_init(*pev, K_POLL_TYPE_SEM_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY,
				  &mock->readable);
		(*pev)++;
		k_sem_give(&mock->prepared);

		return 0;
	}
	case ZFD_IOCTL_POLL_UPDATE:
		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);

		if ((*pev)->state == K_POLL_STATE_SEM_AVAILABLE) {
			pfd->revents = pfd->events;
		}

		(*pev)++;

		return 0;
	default:
		return -EOPNOTSUPP;
	}
}
TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS)

static const struct fd_op_vtable reconfig_mock_vtable = {
	.close = reconfig_mock_close,
	.ioctl = reconfig_mock_ioctl,
};

static int reconfig_mock_alloc(struct reconfig_mock_fd *mock)
{
	return zvfs_alloc_fd(mock, &reconfig_mock_vtable);
}

struct reconfig_state {
	struct k_sem cb_sem;
	int cb_count;
	int cb_result[2];
	short revents[2];
	int fds[2];
	struct zsock_pollfd replacement;
};

static void reconfig_state_init(struct reconfig_state *state)
{
	k_sem_init(&state->cb_sem, 0, UINT_MAX);
	state->cb_count = 0;
	state->cb_result[0] = 0;
	state->cb_result[1] = 0;
	state->revents[0] = 0;
	state->revents[1] = 0;
	state->fds[0] = -1;
	state->fds[1] = -1;
	state->replacement.fd = -1;
}

static void reconfig_wait_readable(int fd)
{
	struct zsock_pollfd pfd = {
		.fd = fd,
		.events = ZSOCK_POLLIN,
	};
	int ret;

	ret = zsock_poll(&pfd, 1, WAIT_TIME);
	zassert_equal(ret, 1, "Socket %d did not become readable (%d)", fd, ret);
}

static int reconfig_wait_fd_closed(int fd)
{
	for (int i = 0; i < (RECONFIG_CLOSED_WAIT_MS / 10); i++) {
		if ((zsock_fcntl(fd, F_GETFL, 0) < 0) && (errno == EBADF)) {
			return 0;
		}

		k_msleep(10);
	}

	return -ETIMEDOUT;
}

static void reconfig_drain_udp(int fd)
{
	char buf[32];

	while (zsock_recv(fd, buf, sizeof(buf), ZSOCK_MSG_DONTWAIT) >= 0) {
	}
}

static void reconfig_close_handler(struct net_socket_service_event *pev)
{
	struct reconfig_state *state = pev->user_data;

	state->cb_count++;
	if (state->cb_count == 1) {
		state->cb_result[0] = net_socket_service_close(pev->svc);
	} else {
		/* Only reachable if a stale poll entry was re-armed after the
		 * close request. Drain and unregister so the failure is
		 * reported cleanly instead of storming the service thread.
		 */
		for (int i = 0; i < ARRAY_SIZE(state->fds); i++) {
			if (state->fds[i] >= 0) {
				reconfig_drain_udp(state->fds[i]);
			}
		}

		(void)net_socket_service_unregister(pev->svc);
	}

	k_sem_give(&state->cb_sem);
}

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(reconfig_close_service, reconfig_close_handler, 1);
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(reconfig_close_multi_service, reconfig_close_handler,
				      RECONFIG_CLOSE_FD_COUNT);

static void reconfig_events_handler(struct net_socket_service_event *pev)
{
	struct reconfig_state *state = pev->user_data;

	state->cb_count++;
	if (state->cb_count == 1) {
		state->revents[0] = pev->event.revents;
		/* Keep the datagram unread: a stale POLLIN entry must stay
		 * ready so re-arming it is observable in the next callback.
		 */
		state->cb_result[0] =
			net_socket_service_register(pev->svc, &state->replacement, 1, state);
	} else {
		if (state->cb_count == 2) {
			state->revents[1] = pev->event.revents;
			/* A UDP socket is always ready for POLLOUT, so the
			 * service must be unregistered here to stop the
			 * callbacks.
			 */
			state->cb_result[1] = net_socket_service_unregister(pev->svc);
		} else {
			(void)net_socket_service_unregister(pev->svc);
		}

		reconfig_drain_udp(state->fds[0]);
	}

	k_sem_give(&state->cb_sem);
}

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(reconfig_events_service, reconfig_events_handler, 1);

static void reconfig_unregister_handler(struct net_socket_service_event *pev)
{
	struct reconfig_state *state = pev->user_data;

	state->cb_count++;
	state->cb_result[0] = net_socket_service_unregister(pev->svc);
	k_sem_give(&state->cb_sem);
}

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(reconfig_unregister_service, reconfig_unregister_handler, 1);

static void reconfig_replace_handler(struct net_socket_service_event *pev)
{
	struct reconfig_state *state = pev->user_data;

	state->cb_count++;
	if (state->cb_count == 1) {
		state->cb_result[0] =
			net_socket_service_register(pev->svc, &state->replacement, 1, state);
	} else if (state->cb_count == 2) {
		state->cb_result[1] = net_socket_service_close(pev->svc);
	} else {
		/* Only reachable if a stale poll entry discarded the close
		 * request; stop the callbacks so the test can fail cleanly.
		 */
		(void)net_socket_service_unregister(pev->svc);
	}

	k_sem_give(&state->cb_sem);
}

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(reconfig_replace_service, reconfig_replace_handler, 1);

static void reconfig_witness_handler(struct net_socket_service_event *pev)
{
	ARG_UNUSED(pev);
}

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(reconfig_witness_service, reconfig_witness_handler, 1);

ZTEST(net_socket_service, test_close_from_callback)
{
	struct zsock_pollfd pollfd = {.events = ZSOCK_POLLIN};
	struct net_sockaddr_in6 c_addr, s_addr;
	static struct reconfig_state state;
	int c_sock, s_sock, cb, closed, ret;
	ssize_t len;

	reconfig_state_init(&state);
	prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &c_sock, &c_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, RECONFIG_CLOSE_PORT, &s_sock, &s_addr);

	ret = bind(s_sock, (struct net_sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(ret, 0, "bind failed (%d)", -errno);

	len = sendto(c_sock, BUF_AND_SIZE(TEST_STR_SMALL), 0, (struct net_sockaddr *)&s_addr,
		     sizeof(s_addr));
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid send len");

	/* The unread datagram keeps the socket readable for the whole test. */
	reconfig_wait_readable(s_sock);

	state.fds[0] = s_sock;
	pollfd.fd = s_sock;
	ret = net_socket_service_register(&reconfig_close_service, &pollfd, 1, &state);
	zassert_equal(ret, 0, "Cannot register service (%d)", ret);

	cb = k_sem_take(&state.cb_sem, RECONFIG_TIMEOUT);
	closed = (cb == 0) ? reconfig_wait_fd_closed(s_sock) : -ETIMEDOUT;

	if (closed != 0) {
		(void)net_socket_service_unregister(&reconfig_close_service);
		(void)close(s_sock);
	}

	(void)close(c_sock);

	zassert_ok(cb, "Timeout while waiting callback");
	zassert_ok(closed, "Service did not close fd");
	zassert_equal(state.cb_count, 1, "Callback ran again after close request");
	zassert_ok(state.cb_result[0], "Cannot close service (%d)", state.cb_result[0]);
}

ZTEST(net_socket_service, test_close_multiple_from_callback)
{
	struct zsock_pollfd pollfd[RECONFIG_CLOSE_FD_COUNT] = {
		{.events = ZSOCK_POLLIN},
		{.events = ZSOCK_POLLIN},
	};
	struct net_sockaddr_in6 c_addr, s_addr[RECONFIG_CLOSE_FD_COUNT];
	int s_sock[RECONFIG_CLOSE_FD_COUNT], closed[RECONFIG_CLOSE_FD_COUNT];
	static struct reconfig_state state;
	int c_sock, cb, ret;
	ssize_t len;

	reconfig_state_init(&state);
	prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &c_sock, &c_addr);

	for (int i = 0; i < ARRAY_SIZE(s_sock); i++) {
		prepare_sock_udp_v6(MY_IPV6_ADDR, RECONFIG_MULTI_PORT + i, &s_sock[i], &s_addr[i]);

		ret = bind(s_sock[i], (struct net_sockaddr *)&s_addr[i], sizeof(s_addr[i]));
		zassert_equal(ret, 0, "bind %d failed (%d)", i, -errno);

		len = sendto(c_sock, BUF_AND_SIZE(TEST_STR_SMALL), 0,
			     (struct net_sockaddr *)&s_addr[i], sizeof(s_addr[i]));
		zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid send len");
	}

	/* Both sockets must be readable before registering so that both poll
	 * entries are ready in the same poll round.
	 */
	for (int i = 0; i < ARRAY_SIZE(s_sock); i++) {
		reconfig_wait_readable(s_sock[i]);
		state.fds[i] = s_sock[i];
		pollfd[i].fd = s_sock[i];
	}

	ret = net_socket_service_register(&reconfig_close_multi_service, pollfd, ARRAY_SIZE(pollfd),
					  &state);
	zassert_equal(ret, 0, "Cannot register service (%d)", ret);

	cb = k_sem_take(&state.cb_sem, RECONFIG_TIMEOUT);
	for (int i = 0; i < ARRAY_SIZE(s_sock); i++) {
		closed[i] = (cb == 0) ? reconfig_wait_fd_closed(s_sock[i]) : -ETIMEDOUT;
	}

	for (int i = 0; i < ARRAY_SIZE(s_sock); i++) {
		if (closed[i] != 0) {
			(void)net_socket_service_unregister(&reconfig_close_multi_service);
			(void)close(s_sock[i]);
		}
	}

	(void)close(c_sock);

	zassert_ok(cb, "Timeout while waiting callback");
	for (int i = 0; i < ARRAY_SIZE(s_sock); i++) {
		zassert_ok(closed[i], "Service fd %d was not closed", i);
	}
	zassert_equal(state.cb_count, 1, "Second ready fd invoked callback after close");
	zassert_ok(state.cb_result[0], "Cannot close service (%d)", state.cb_result[0]);
}

ZTEST(net_socket_service, test_reconfigure_events_from_callback)
{
	struct zsock_pollfd pollfd = {.events = ZSOCK_POLLIN};
	struct net_sockaddr_in6 c_addr, s_addr;
	static struct reconfig_state state;
	int c_sock, s_sock, cb1, cb2, ret;
	ssize_t len;

	reconfig_state_init(&state);
	prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &c_sock, &c_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR, RECONFIG_EVENTS_PORT, &s_sock, &s_addr);

	ret = bind(s_sock, (struct net_sockaddr *)&s_addr, sizeof(s_addr));
	zassert_equal(ret, 0, "bind failed (%d)", -errno);

	len = sendto(c_sock, BUF_AND_SIZE(TEST_STR_SMALL), 0, (struct net_sockaddr *)&s_addr,
		     sizeof(s_addr));
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid send len");

	reconfig_wait_readable(s_sock);

	state.fds[0] = s_sock;
	state.replacement.fd = s_sock;
	state.replacement.events = ZSOCK_POLLOUT;
	pollfd.fd = s_sock;
	ret = net_socket_service_register(&reconfig_events_service, &pollfd, 1, &state);
	zassert_equal(ret, 0, "Cannot register service (%d)", ret);

	cb1 = k_sem_take(&state.cb_sem, RECONFIG_TIMEOUT);
	cb2 = (cb1 == 0) ? k_sem_take(&state.cb_sem, RECONFIG_TIMEOUT) : -ETIMEDOUT;

	(void)net_socket_service_unregister(&reconfig_events_service);
	(void)close(c_sock);
	(void)close(s_sock);

	zassert_ok(cb1, "Timeout while waiting reconfigure callback");
	zassert_ok(cb2, "Timeout while waiting reconfigured callback");
	zassert_equal(state.cb_count, 2, "Callback ran again after unregister");
	zassert_ok(state.cb_result[0], "Cannot update service (%d)", state.cb_result[0]);
	zassert_ok(state.cb_result[1], "Cannot unregister service (%d)", state.cb_result[1]);
	zassert_true((state.revents[0] & ZSOCK_POLLIN) != 0,
		     "First callback did not report POLLIN (0x%x)", state.revents[0]);
	zassert_true((state.revents[1] & ZSOCK_POLLOUT) != 0,
		     "Reconfigured mask was not applied (0x%x)", state.revents[1]);
	zassert_false((state.revents[1] & ZSOCK_POLLIN) != 0,
		      "Stale POLLIN after reconfigure (0x%x)", state.revents[1]);
}

ZTEST(net_socket_service, test_unregister_from_callback)
{
	struct zsock_pollfd pollfd = {.events = ZSOCK_POLLIN};
	struct zsock_pollfd witness_pollfd = {.events = ZSOCK_POLLIN};
	static struct reconfig_mock_fd mock, witness;
	static struct reconfig_state state;
	int fd, witness_fd, cb, repolled, sync, ret;
	int close_ret, witness_close_ret, witness_closed;

	reconfig_mock_init(&mock);
	reconfig_mock_init(&witness);
	reconfig_state_init(&state);
	fd = reconfig_mock_alloc(&mock);
	zassert_true(fd >= 0, "Cannot allocate fd (%d)", fd);
	witness_fd = reconfig_mock_alloc(&witness);
	if (witness_fd < 0) {
		(void)zvfs_close(fd);
	}
	zassert_true(witness_fd >= 0, "Cannot allocate witness fd (%d)", witness_fd);

	/* The witness fd is prepared on every poll round and acts as a sync
	 * point telling that the service thread has entered another round.
	 */
	witness_pollfd.fd = witness_fd;
	ret = net_socket_service_register(&reconfig_witness_service, &witness_pollfd, 1, NULL);
	if (ret != 0) {
		(void)zvfs_close(fd);
		(void)zvfs_close(witness_fd);
	}
	zassert_equal(ret, 0, "Cannot register witness service (%d)", ret);

	ret = k_sem_take(&witness.prepared, RECONFIG_TIMEOUT);
	if (ret == 0) {
		pollfd.fd = fd;
		ret = net_socket_service_register(&reconfig_unregister_service, &pollfd, 1, &state);
		zassert_equal(ret, 0, "Cannot register service (%d)", ret);

		ret = k_sem_take(&mock.prepared, RECONFIG_TIMEOUT);
		if (ret == 0) {
			ret = k_sem_take(&witness.prepared, RECONFIG_TIMEOUT);
		}
	}

	if (ret != 0) {
		(void)net_socket_service_unregister(&reconfig_unregister_service);
		(void)net_socket_service_unregister(&reconfig_witness_service);
		(void)zvfs_close(fd);
		(void)zvfs_close(witness_fd);
	}
	zassert_equal(ret, 0, "Registered fds were not polled (%d)", ret);

	k_sem_give(&mock.readable);
	cb = k_sem_take(&state.cb_sem, RECONFIG_TIMEOUT);
	/* Wait for the poll round following the callback: a stale re-armed
	 * entry would have prepared the unregistered fd in that same round.
	 */
	sync = (cb == 0) ? k_sem_take(&witness.prepared, RECONFIG_TIMEOUT) : -ETIMEDOUT;
	repolled = k_sem_count_get(&mock.prepared);

	if (cb != 0) {
		(void)net_socket_service_unregister(&reconfig_unregister_service);
	}

	close_ret = zvfs_close(fd);
	witness_close_ret = net_socket_service_close(&reconfig_witness_service);
	witness_closed = k_sem_take(&witness.closed, RECONFIG_TIMEOUT);
	if (witness_closed != 0) {
		(void)net_socket_service_unregister(&reconfig_witness_service);
		(void)zvfs_close(witness_fd);
	}

	zassert_ok(cb, "Timeout while waiting callback");
	zassert_ok(sync, "Service thread did not run another poll round");
	zassert_equal(repolled, 0, "Unregistered fd was polled again");
	zassert_equal(state.cb_count, 1, "Callback ran again after unregister");
	zassert_ok(state.cb_result[0], "Cannot unregister service (%d)", state.cb_result[0]);
	zassert_equal(close_ret, 0, "close failed");
	zassert_ok(witness_close_ret, "Cannot close witness service (%d)", witness_close_ret);
	zassert_ok(witness_closed, "Witness fd was not closed");
}

ZTEST(net_socket_service, test_reconfigure_from_callback)
{
	struct zsock_pollfd pollfd = {.events = ZSOCK_POLLIN};
	static struct reconfig_mock_fd mock, replacement;
	static struct reconfig_state state;
	int fd, replacement_fd, cb1, cb2, closed, repolled, ret;

	reconfig_mock_init(&mock);
	reconfig_mock_init(&replacement);
	reconfig_state_init(&state);
	replacement.expected_events = ZSOCK_POLLOUT;

	fd = reconfig_mock_alloc(&mock);
	zassert_true(fd >= 0, "Cannot allocate fd (%d)", fd);
	replacement_fd = reconfig_mock_alloc(&replacement);
	if (replacement_fd < 0) {
		(void)zvfs_close(fd);
	}
	zassert_true(replacement_fd >= 0, "Cannot allocate replacement fd (%d)", replacement_fd);

	pollfd.fd = fd;
	state.replacement.fd = replacement_fd;
	state.replacement.events = ZSOCK_POLLOUT;
	ret = net_socket_service_register(&reconfig_replace_service, &pollfd, 1, &state);
	if (ret != 0) {
		(void)zvfs_close(fd);
		(void)zvfs_close(replacement_fd);
	}
	zassert_equal(ret, 0, "Cannot register service (%d)", ret);

	ret = k_sem_take(&mock.prepared, RECONFIG_TIMEOUT);
	if (ret != 0) {
		(void)net_socket_service_unregister(&reconfig_replace_service);
		(void)zvfs_close(fd);
		(void)zvfs_close(replacement_fd);
	}
	zassert_ok(ret, "Registered fd was not polled");

	k_sem_give(&replacement.readable);
	k_sem_give(&mock.readable);
	cb1 = k_sem_take(&state.cb_sem, RECONFIG_TIMEOUT);
	cb2 = (cb1 == 0) ? k_sem_take(&state.cb_sem, RECONFIG_TIMEOUT) : -ETIMEDOUT;
	closed = (cb2 == 0) ? k_sem_take(&replacement.closed, RECONFIG_TIMEOUT) : -ETIMEDOUT;
	/* The replaced fd must not be polled again after the initial prepare
	 * consumed above: a stale re-armed entry would prepare it once more.
	 */
	repolled = k_sem_count_get(&mock.prepared);

	if (closed != 0) {
		(void)net_socket_service_unregister(&reconfig_replace_service);
		(void)zvfs_close(replacement_fd);
	}

	/* The replaced fd is not closed by the service. */
	ret = zvfs_close(fd);

	zassert_ok(cb1, "Timeout while waiting reconfigure callback");
	zassert_ok(cb2, "Timeout while waiting replacement callback");
	zassert_ok(state.cb_result[0], "Cannot update service (%d)", state.cb_result[0]);
	zassert_ok(state.cb_result[1], "Cannot close service (%d)", state.cb_result[1]);
	zassert_ok(closed, "Replacement fd was not closed");
	zassert_equal(state.cb_count, 2, "Callback ran again after close request");
	zassert_equal(repolled, 0, "Replaced fd was polled again");
	zassert_equal(k_sem_count_get(&replacement.wrong_events), 0,
		      "Replacement fd was polled with stale events");
	zassert_equal(ret, 0, "close failed");
}

ZTEST_SUITE(net_socket_service, NULL, NULL, NULL, NULL, NULL);

/* Each socket registered with net_socket_service_register_single_fd() carries
 * its own user_data, which must be handed back to the callback when that
 * socket is triggered.
 */
#define SINGLE_FD_COUNT 3
#define SINGLE_FD_PORT  4250

struct single_fd_ctx {
	int fd;
};

static K_SEM_DEFINE(single_fd_sem, 0, 1);
static struct single_fd_ctx *single_fd_received_ctx;
static int single_fd_received_fd;

static void single_fd_handler(struct net_socket_service_event *pev)
{
	single_fd_received_ctx = pev->user_data;
	single_fd_received_fd = pev->event.fd;

	/* Consume the datagram so the socket does not trigger again. */
	reconfig_drain_udp(pev->event.fd);

	k_sem_give(&single_fd_sem);
}

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(single_fd_service, single_fd_handler, SINGLE_FD_COUNT);

ZTEST(net_socket_service, test_register_single_fd_user_data)
{
	struct single_fd_ctx ctx[SINGLE_FD_COUNT];
	struct net_sockaddr_in6 c_addr, s_addr[SINGLE_FD_COUNT];
	struct zsock_pollfd pfd = {.events = ZSOCK_POLLIN};
	int c_sock, ret;
	ssize_t len;

	prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &c_sock, &c_addr);

	for (int i = 0; i < SINGLE_FD_COUNT; i++) {
		prepare_sock_udp_v6(MY_IPV6_ADDR, SINGLE_FD_PORT + i, &ctx[i].fd, &s_addr[i]);

		ret = bind(ctx[i].fd, (struct net_sockaddr *)&s_addr[i], sizeof(s_addr[i]));
		zassert_equal(ret, 0, "bind %d failed (%d)", i, -errno);

		pfd.fd = ctx[i].fd;
		ret = net_socket_service_register_single_fd(&single_fd_service, &pfd, &ctx[i]);
		zassert_ok(ret, "Cannot register socket %d (%d)", i, ret);
	}

	/* Trigger the sockets in reverse order of registration so that a
	 * callback returning the user_data of the wrong slot is detected.
	 */
	for (int i = SINGLE_FD_COUNT - 1; i >= 0; i--) {
		single_fd_received_ctx = NULL;

		len = sendto(c_sock, BUF_AND_SIZE(TEST_STR_SMALL), 0,
			     (struct net_sockaddr *)&s_addr[i], sizeof(s_addr[i]));
		zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid send len");

		ret = k_sem_take(&single_fd_sem, K_MSEC(WAIT_TIME));
		zassert_ok(ret, "Timeout while waiting callback for socket %d", i);
		zassert_equal(single_fd_received_fd, ctx[i].fd, "Callback for wrong socket");
		zassert_equal_ptr(single_fd_received_ctx, &ctx[i],
				  "Callback for socket %d returned the wrong ctx", i);
	}

	ret = net_socket_service_unregister(&single_fd_service);
	zassert_ok(ret, "Cannot unregister service (%d)", ret);

	for (int i = 0; i < SINGLE_FD_COUNT; i++) {
		ret = close(ctx[i].fd);
		zassert_equal(ret, 0, "close failed");
	}

	ret = close(c_sock);
	zassert_equal(ret, 0, "close failed");
}

ZTEST(net_socket_service, test_unregister_single_fd)
{
	struct single_fd_ctx ctx[SINGLE_FD_COUNT];
	struct net_sockaddr_in6 c_addr, s_addr[SINGLE_FD_COUNT];
	struct zsock_pollfd pfd = {.events = ZSOCK_POLLIN};
	int c_sock, ret;
	ssize_t len;

	prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &c_sock, &c_addr);

	for (int i = 0; i < SINGLE_FD_COUNT; i++) {
		prepare_sock_udp_v6(MY_IPV6_ADDR, SINGLE_FD_PORT + i, &ctx[i].fd, &s_addr[i]);

		ret = bind(ctx[i].fd, (struct net_sockaddr *)&s_addr[i], sizeof(s_addr[i]));
		zassert_equal(ret, 0, "bind %d failed (%d)", i, -errno);

		pfd.fd = ctx[i].fd;
		ret = net_socket_service_register_single_fd(&single_fd_service, &pfd, &ctx[i]);
		zassert_ok(ret, "Cannot register socket %d (%d)", i, ret);
	}

	ret = net_socket_service_unregister_single_fd(&single_fd_service, ctx[0].fd);
	zassert_ok(ret, "Cannot unregister socket (%d)", ret);

	/* Unregistering the same socket again fails. */
	ret = net_socket_service_unregister_single_fd(&single_fd_service, ctx[0].fd);
	zassert_equal(ret, -ENOENT, "Socket unregistered twice (%d)", ret);

	/* Data on the unregistered socket must not invoke the callback. */
	len = sendto(c_sock, BUF_AND_SIZE(TEST_STR_SMALL), 0, (struct net_sockaddr *)&s_addr[0],
		     sizeof(s_addr[0]));
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid send len");

	ret = k_sem_take(&single_fd_sem, K_MSEC(WAIT_TIME));
	zassert_equal(ret, -EAGAIN, "Callback for unregistered socket");

	/* The other sockets are still serviced with their own ctx. */
	for (int i = 1; i < SINGLE_FD_COUNT; i++) {
		single_fd_received_ctx = NULL;

		len = sendto(c_sock, BUF_AND_SIZE(TEST_STR_SMALL), 0,
			     (struct net_sockaddr *)&s_addr[i], sizeof(s_addr[i]));
		zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid send len");

		ret = k_sem_take(&single_fd_sem, K_MSEC(WAIT_TIME));
		zassert_ok(ret, "Timeout while waiting callback for socket %d", i);
		zassert_equal(single_fd_received_fd, ctx[i].fd, "Callback for wrong socket");
		zassert_equal_ptr(single_fd_received_ctx, &ctx[i],
				  "Callback for socket %d returned the wrong ctx", i);
	}

	ret = net_socket_service_unregister(&single_fd_service);
	zassert_ok(ret, "Cannot unregister service (%d)", ret);

	for (int i = 0; i < SINGLE_FD_COUNT; i++) {
		ret = close(ctx[i].fd);
		zassert_equal(ret, 0, "close failed");
	}

	ret = close(c_sock);
	zassert_equal(ret, 0, "close failed");
}

ZTEST(net_socket_service, test_register_single_fd_full)
{
	struct single_fd_ctx ctx[SINGLE_FD_COUNT + 1];
	struct net_sockaddr_in6 addr;
	struct zsock_pollfd pfd = {.events = ZSOCK_POLLIN};
	int ret;

	for (int i = 0; i < ARRAY_SIZE(ctx); i++) {
		prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &ctx[i].fd, &addr);
	}

	for (int i = 0; i < SINGLE_FD_COUNT; i++) {
		pfd.fd = ctx[i].fd;
		ret = net_socket_service_register_single_fd(&single_fd_service, &pfd, &ctx[i]);
		zassert_ok(ret, "Cannot register socket %d (%d)", i, ret);
	}

	/* All slots are in use. */
	pfd.fd = ctx[SINGLE_FD_COUNT].fd;
	ret = net_socket_service_register_single_fd(&single_fd_service, &pfd,
						    &ctx[SINGLE_FD_COUNT]);
	zassert_equal(ret, -ENOMEM, "Registered more sockets than slots (%d)", ret);

	/* Freeing a slot makes room for another socket. */
	ret = net_socket_service_unregister_single_fd(&single_fd_service, ctx[0].fd);
	zassert_ok(ret, "Cannot unregister socket (%d)", ret);

	ret = net_socket_service_register_single_fd(&single_fd_service, &pfd,
						    &ctx[SINGLE_FD_COUNT]);
	zassert_ok(ret, "Freed slot was not reused (%d)", ret);

	ret = net_socket_service_unregister(&single_fd_service);
	zassert_ok(ret, "Cannot unregister service (%d)", ret);

	for (int i = 0; i < ARRAY_SIZE(ctx); i++) {
		ret = close(ctx[i].fd);
		zassert_equal(ret, 0, "close failed");
	}
}

ZTEST(net_socket_service, test_register_single_fd_invalid)
{
	static struct net_socket_service_desc unknown_service;
	struct single_fd_ctx ctx;
	struct net_sockaddr_in6 addr;
	struct zsock_pollfd pfd = {.events = ZSOCK_POLLIN};
	int ret;

	prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &ctx.fd, &addr);

	ret = net_socket_service_register_single_fd(&single_fd_service, NULL, &ctx);
	zassert_equal(ret, -EINVAL, "NULL pollfd accepted (%d)", ret);

	pfd.fd = -1;
	ret = net_socket_service_register_single_fd(&single_fd_service, &pfd, &ctx);
	zassert_equal(ret, -EINVAL, "Negative fd accepted (%d)", ret);

	pfd.fd = ctx.fd;
	ret = net_socket_service_register_single_fd(&unknown_service, &pfd, &ctx);
	zassert_equal(ret, -ENOENT, "Unknown service accepted (%d)", ret);

	ret = net_socket_service_unregister_single_fd(&unknown_service, ctx.fd);
	zassert_equal(ret, -ENOENT, "Unknown service accepted (%d)", ret);

	ret = net_socket_service_unregister_single_fd(&single_fd_service, ctx.fd);
	zassert_equal(ret, -ENOENT, "Unregistered a socket never registered (%d)", ret);

	ret = net_socket_service_register_single_fd(&single_fd_service, &pfd, &ctx);
	zassert_ok(ret, "Cannot register socket (%d)", ret);

	/* The same socket cannot be registered twice. */
	ret = net_socket_service_register_single_fd(&single_fd_service, &pfd, &ctx);
	zassert_equal(ret, -EEXIST, "Socket registered twice (%d)", ret);

	ret = net_socket_service_unregister(&single_fd_service);
	zassert_ok(ret, "Cannot unregister service (%d)", ret);

	ret = close(ctx.fd);
	zassert_equal(ret, 0, "close failed");
}

ZTEST(net_socket_service, test_register_single_fd_reuses_freed_slot)
{
	const struct net_socket_service_desc *svc = &single_fd_service;
	struct single_fd_ctx ctx[SINGLE_FD_COUNT], new_ctx;
	struct net_sockaddr_in6 c_addr, addr, new_addr;
	struct zsock_pollfd pfd = {.events = ZSOCK_POLLIN};
	int c_sock, ret;
	ssize_t len;

	/* Fill all three slots: socket i goes to slot i. */
	for (int i = 0; i < ARRAY_SIZE(ctx); i++) {
		prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &ctx[i].fd, &addr);

		pfd.fd = ctx[i].fd;
		ret = net_socket_service_register_single_fd(svc, &pfd, &ctx[i]);
		zassert_ok(ret, "Cannot register socket %d (%d)", i, ret);
		zassert_equal(svc->pev[i].event.fd, ctx[i].fd, "Socket %d not in slot %d", i, i);
	}

	/* Unregister only the second socket, freeing the second slot. */
	ret = net_socket_service_unregister_single_fd(svc, ctx[1].fd);
	zassert_ok(ret, "Cannot unregister socket (%d)", ret);
	zassert_equal(svc->pev[1].event.fd, -1, "Second slot not freed");

	/* The second socket is kept open, so the new socket gets a different fd. */
	prepare_sock_udp_v6(MY_IPV6_ADDR, SINGLE_FD_PORT, &new_ctx.fd, &new_addr);

	ret = bind(new_ctx.fd, (struct net_sockaddr *)&new_addr, sizeof(new_addr));
	zassert_equal(ret, 0, "bind failed (%d)", -errno);

	pfd.fd = new_ctx.fd;
	ret = net_socket_service_register_single_fd(svc, &pfd, &new_ctx);
	zassert_ok(ret, "Cannot register new socket (%d)", ret);

	/* The new socket takes the freed second slot; the others are untouched. */
	zassert_equal(svc->pev[1].event.fd, new_ctx.fd, "New socket not in the second slot");
	zassert_equal_ptr(svc->pev[1].user_data, &new_ctx, "Wrong user_data in second slot");
	zassert_equal(svc->pev[0].event.fd, ctx[0].fd, "First slot changed");
	zassert_equal_ptr(svc->pev[0].user_data, &ctx[0], "First slot user_data changed");
	zassert_equal(svc->pev[2].event.fd, ctx[2].fd, "Third slot changed");
	zassert_equal_ptr(svc->pev[2].user_data, &ctx[2], "Third slot user_data changed");

	/* Triggering the new socket returns its own ctx. */
	single_fd_received_ctx = NULL;
	prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &c_sock, &c_addr);

	len = sendto(c_sock, BUF_AND_SIZE(TEST_STR_SMALL), 0, (struct net_sockaddr *)&new_addr,
		     sizeof(new_addr));
	zassert_equal(len, STRLEN(TEST_STR_SMALL), "invalid send len");

	ret = k_sem_take(&single_fd_sem, K_MSEC(WAIT_TIME));
	zassert_ok(ret, "Timeout while waiting callback");
	zassert_equal(single_fd_received_fd, new_ctx.fd, "Callback for wrong socket");
	zassert_equal_ptr(single_fd_received_ctx, &new_ctx, "Callback returned the wrong ctx");

	ret = net_socket_service_unregister(svc);
	zassert_ok(ret, "Cannot unregister service (%d)", ret);

	for (int i = 0; i < ARRAY_SIZE(ctx); i++) {
		ret = close(ctx[i].fd);
		zassert_equal(ret, 0, "close failed");
	}

	ret = close(new_ctx.fd);
	zassert_equal(ret, 0, "close failed");

	ret = close(c_sock);
	zassert_equal(ret, 0, "close failed");
}
