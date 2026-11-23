/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

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

static void socket_address(int sock, struct net_sockaddr_in *addr)
{
	net_socklen_t len = sizeof(*addr);

	zassert_ok(zsock_getsockname(sock, (struct net_sockaddr *)addr, &len));
	zassert_equal(len, sizeof(*addr));
}

/* A connected TCP pair over the loopback interface, plus its listener. */
static void tcp_pair(int *listener, int *client, int *server)
{
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_addr = NET_INADDR_LOOPBACK_INIT,
	};

	*listener = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	zassert_true(*listener >= 0, "socket() failed (%d)", errno);
	zassert_ok(zsock_bind(*listener, (struct net_sockaddr *)&addr, sizeof(addr)));
	zassert_ok(zsock_listen(*listener, 1));
	socket_address(*listener, &addr);

	*client = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	zassert_true(*client >= 0, "socket() failed (%d)", errno);
	zassert_ok(zsock_connect(*client, (struct net_sockaddr *)&addr, sizeof(addr)));
	*server = zsock_accept(*listener, NULL, NULL);
	zassert_true(*server >= 0, "accept() failed (%d)", errno);
}

/* recvmsg() gathers into several buffers and reports the peer address. */
ZTEST(nsos, test_recvmsg)
{
	char first[] = "scatter";
	char second[] = "gather";
	struct net_iovec tx_iov[] = {
		{.iov_base = first, .iov_len = sizeof(first) - 1U},
		{.iov_base = second, .iov_len = sizeof(second) - 1U},
	};
	char head[4];
	char tail[16];
	struct net_iovec rx_iov[] = {
		{.iov_base = head, .iov_len = sizeof(head)},
		{.iov_base = tail, .iov_len = sizeof(tail)},
	};
	struct net_sockaddr_in to, from, sender;
	struct net_msghdr msg;
	ssize_t ret;
	int tx, rx;

	tx = udp_socket();
	rx = udp_socket();
	socket_address(rx, &to);
	socket_address(tx, &sender);

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = &to;
	msg.msg_namelen = sizeof(to);
	msg.msg_iov = tx_iov;
	msg.msg_iovlen = ARRAY_SIZE(tx_iov);
	ret = zsock_sendmsg(tx, &msg, 0);
	zassert_equal(ret, 13, "sendmsg() failed (%d)", errno);

	memset(&msg, 0, sizeof(msg));
	memset(&from, 0, sizeof(from));
	msg.msg_name = &from;
	msg.msg_namelen = sizeof(from);
	msg.msg_iov = rx_iov;
	msg.msg_iovlen = ARRAY_SIZE(rx_iov);
	ret = zsock_recvmsg(rx, &msg, 0);
	zassert_equal(ret, 13, "recvmsg() failed (%d)", errno);

	zexpect_mem_equal(head, "scat", sizeof(head));
	zexpect_mem_equal(tail, "tergather", 9);
	zexpect_equal(msg.msg_namelen, sizeof(from));
	zexpect_equal(from.sin_family, NET_AF_INET);
	zexpect_equal(from.sin_port, sender.sin_port);
	zexpect_equal(from.sin_addr.s_addr, sender.sin_addr.s_addr);
	zexpect_equal(msg.msg_controllen, 0);
	zexpect_equal(msg.msg_flags, 0);

	zassert_ok(zsock_close(tx));
	zassert_ok(zsock_close(rx));
}

/* A datagram longer than the buffers is cut and reported as truncated. */
ZTEST(nsos, test_recvmsg_truncated)
{
	static const char data[] = "0123456789";
	char buf[4];
	struct net_iovec iov = {.iov_base = buf, .iov_len = sizeof(buf)};
	struct net_sockaddr_in to;
	struct net_msghdr msg;
	ssize_t ret;
	int tx, rx;

	tx = udp_socket();
	rx = udp_socket();
	socket_address(rx, &to);
	ret = zsock_sendto(tx, data, sizeof(data) - 1U, 0, (struct net_sockaddr *)&to, sizeof(to));
	zassert_equal(ret, (ssize_t)(sizeof(data) - 1U), "sendto() failed (%d)", errno);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	ret = zsock_recvmsg(rx, &msg, 0);
	zassert_equal(ret, (ssize_t)sizeof(buf), "recvmsg() failed (%d)", errno);
	zexpect_mem_equal(buf, "0123", sizeof(buf));
	zexpect_equal(msg.msg_flags & ZSOCK_MSG_TRUNC, ZSOCK_MSG_TRUNC);

	zassert_ok(zsock_close(tx));
	zassert_ok(zsock_close(rx));
}

/* On a connected stream socket the host reports no peer address. */
ZTEST(nsos, test_recvmsg_connected)
{
	struct net_sockaddr_in from;
	char buf[4];
	struct net_iovec iov = {.iov_base = buf, .iov_len = sizeof(buf)};
	struct net_msghdr msg;
	ssize_t ret;
	int listener, client, server;

	tcp_pair(&listener, &client, &server);

	ret = zsock_send(client, "x", 1, 0);
	zassert_equal(ret, 1, "send() failed (%d)", errno);

	memset(&msg, 0, sizeof(msg));
	memset(&from, 0xff, sizeof(from));
	msg.msg_name = &from;
	msg.msg_namelen = sizeof(from);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	ret = zsock_recvmsg(server, &msg, 0);
	zassert_equal(ret, 1, "recvmsg() failed (%d)", errno);
	zexpect_equal(msg.msg_namelen, 0);

	zassert_ok(zsock_close(server));
	zassert_ok(zsock_close(client));
	zassert_ok(zsock_close(listener));
}

/* recvfrom() on a connected stream socket reports no peer address either. */
ZTEST(nsos, test_recvfrom_connected)
{
	struct net_sockaddr_in from;
	net_socklen_t fromlen = sizeof(from);
	char buf[4];
	ssize_t ret;
	int listener, client, server;

	tcp_pair(&listener, &client, &server);

	ret = zsock_send(client, "x", 1, 0);
	zassert_equal(ret, 1, "send() failed (%d)", errno);

	memset(&from, 0xff, sizeof(from));
	ret = zsock_recvfrom(server, buf, sizeof(buf), 0, (struct net_sockaddr *)&from, &fromlen);
	zassert_equal(ret, 1, "recvfrom() failed (%d)", errno);
	zexpect_equal(fromlen, 0);

	zassert_ok(zsock_close(server));
	zassert_ok(zsock_close(client));
	zassert_ok(zsock_close(listener));
}

ZTEST(nsos, test_so_broadcast)
{
	int optval;
	net_socklen_t optlen = sizeof(optval);
	int sock;

	sock = udp_socket();

	zassert_ok(zsock_getsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_BROADCAST, &optval,
				    &optlen), "getsockopt() failed (%d)", errno);
	zassert_equal(optval, 0);

	optval = 1;
	zassert_ok(zsock_setsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_BROADCAST, &optval,
				    sizeof(optval)), "setsockopt() failed (%d)", errno);

	optval = 0;
	zassert_ok(zsock_getsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_BROADCAST, &optval,
				    &optlen), "getsockopt() failed (%d)", errno);
	zassert_not_equal(optval, 0);

	zassert_ok(zsock_close(sock));
}

ZTEST_SUITE(nsos, NULL, NULL, NULL, NULL, NULL);
