/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 *
 * The STUN client over real UDP sockets, against the scripted server of
 * server.c.
 */
#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/stun.h>
#include <zephyr/net/stun_client.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#include "server.h"

/* More calls than there can be sockets: one left open by any of them leaves the
 * calls after it without. test_many_calls_exceed_the_socket_limit holds the
 * number to that.
 */
#define MANY_CALLS 64

/* The socket of the test that is running, closed after it whatever its outcome:
 * a failed test must not leave the ones after it without sockets.
 */
static int client_sock = -1;

static bool is_loopback(const struct net_sockaddr *sa, int family)
{
	static const uint8_t kV4[4] = {127, 0, 0, 1};

	if (sa->sa_family != family) {
		return false;
	}
	if (family == NET_AF_INET) {
		return memcmp(net_sin(sa)->sin_addr.s4_addr, kV4, sizeof(kV4)) == 0;
	}
#if defined(CONFIG_NET_IPV6)
	{
		static const uint8_t kV6[16] = {[15] = 1};

		return memcmp(net_sin6(sa)->sin6_addr.s6_addr, kV6, sizeof(kV6)) == 0;
	}
#else
	return false;
#endif
}

static void *suite_setup(void)
{
	server_start();
	return NULL;
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	server_reset();
}

static void after_each(void *fixture)
{
	ARG_UNUSED(fixture);
	if (client_sock >= 0) {
		(void)zsock_close(client_sock);
		client_sock = -1;
	}
}

ZTEST_SUITE(stun_client, NULL, suite_setup, before_each, after_each, NULL);

static int client_socket(int family)
{
	client_sock = udp_socket(family);
	return client_sock;
}

/* The port a socket is bound to. */
static uint16_t local_port(int sock)
{
	struct net_sockaddr local;
	net_socklen_t len = sizeof(local);

	memset(&local, 0, sizeof(local));
	zassert_ok(zsock_getsockname(sock, &local, &len), "getsockname: %d", errno);
	return port_of(&local);
}

/* One query to the server of a family; what comes back is this very socket. */
static void check_query(int family)
{
	struct net_sockaddr srv, mapped;
	int sock = client_socket(family);

	set_loopback(&srv, family, SERVER_PORT);
	memset(&mapped, 0, sizeof(mapped));
	zassert_ok(stun_client_query(sock, &srv, addr_len(family), 2000, &mapped));

	zassert_true(is_loopback(&mapped, family));
	zassert_true(port_of(&mapped) != 0);
	zassert_equal(port_of(&mapped), local_port(sock), "mapped port %u, socket port %u",
		      port_of(&mapped), local_port(sock));
	zassert_equal(atomic_get(&server.requests), 1);
	/* RFC 8489 §6.1: a request should say what sent it, unless told not to. */
	zassert_equal(atomic_get(&server.software_seen) != 0,
		      sizeof(CONFIG_STUN_CLIENT_SOFTWARE) > 1U, "SOFTWARE in the request");
}

ZTEST(stun_client, test_query_ipv4)
{
	if (!IS_ENABLED(CONFIG_NET_IPV4)) {
		ztest_test_skip();
	}
	check_query(NET_AF_INET);
}

ZTEST(stun_client, test_query_ipv6)
{
	if (!IS_ENABLED(CONFIG_NET_IPV6)) {
		ztest_test_skip();
	}
	check_query(NET_AF_INET6);
}

ZTEST(stun_client, test_query_timeout)
{
	struct net_sockaddr srv, mapped;
	int sock = client_socket(FAMILY);
	int64_t took, start;

	atomic_set(&server.mode, MODE_SILENT);
	set_loopback(&srv, FAMILY, SERVER_PORT);

	/* Shorter than the first retransmission interval, 500 ms: the call
	 * returns when the caller said, not when the schedule has a deadline.
	 */
	start = k_uptime_get();
	zassert_equal(stun_client_query(sock, &srv, addr_len(FAMILY), 300, &mapped), -ETIMEDOUT);
	took = k_uptime_get() - start;
	zassert_true(took >= 300 && took < 500, "gave up after %lld ms", took);
	zassert_equal(atomic_get(&server.requests), 1);

	/* Longer than it: the request is sent again in the meantime. */
	atomic_set(&server.requests, 0);
	start = k_uptime_get();
	zassert_equal(stun_client_query(sock, &srv, addr_len(FAMILY), 700, &mapped), -ETIMEDOUT);
	took = k_uptime_get() - start;
	zassert_true(took >= 700 && took < 900, "gave up after %lld ms", took);
	zassert_equal(atomic_get(&server.requests), 2);
}

ZTEST(stun_client, test_query_gives_up_when_the_schedule_ends)
{
	struct net_sockaddr srv, mapped;
	int64_t took, start;
	int sock;

	/* 39.5 seconds: nothing where they are real ones. */
	if (!IS_ENABLED(CONFIG_ARCH_POSIX)) {
		ztest_test_skip();
	}

	sock = client_socket(FAMILY);
	atomic_set(&server.mode, MODE_SILENT);
	set_loopback(&srv, FAMILY, SERVER_PORT);

	/* No time of the caller's: the schedule of RFC 8489 §6.2.1 runs out,
	 * seven transmissions and then 8 seconds of waiting.
	 */
	start = k_uptime_get();
	zassert_equal(stun_client_query(sock, &srv, addr_len(FAMILY), 0, &mapped), -ETIMEDOUT);
	took = k_uptime_get() - start;
	zassert_true(took >= 39500 && took < 40500, "gave up after %lld ms", took);
	zassert_equal(atomic_get(&server.requests), 7);
}

ZTEST(stun_client, test_query_retransmits)
{
	struct net_sockaddr srv, mapped;
	int sock = client_socket(FAMILY);
	int64_t took, start;

	atomic_set(&server.mode, MODE_ANSWER_SECOND);
	set_loopback(&srv, FAMILY, SERVER_PORT);

	start = k_uptime_get();
	zassert_ok(stun_client_query(sock, &srv, addr_len(FAMILY), 0, &mapped));
	took = k_uptime_get() - start;

	/* The same request again, 500 ms after the first (RFC 8489 §6.2.1). */
	zassert_equal(atomic_get(&server.requests), 2);
	zassert_mem_equal(server.txid[0], server.txid[1], STUN_TXID_SIZE);
	zassert_true(took >= 500 && took < 1000, "answered after %lld ms", took);
	zassert_true(is_loopback(&mapped, FAMILY));
	zassert_equal(port_of(&mapped), local_port(sock));
}

ZTEST(stun_client, test_query_error_response)
{
	struct net_sockaddr srv, mapped;
	int sock = client_socket(FAMILY);

	atomic_set(&server.mode, MODE_ERROR_500);
	set_loopback(&srv, FAMILY, SERVER_PORT);
	zassert_equal(stun_client_query(sock, &srv, addr_len(FAMILY), 2000, &mapped), -EPROTO);
	zassert_equal(atomic_get(&server.requests), 1, "an error response is not retried");
}

ZTEST(stun_client, test_query_ignores_what_is_not_the_answer)
{
	struct net_sockaddr srv, mapped;
	int sock = client_socket(FAMILY);

	atomic_set(&server.mode, MODE_NOISE_FIRST);
	set_loopback(&srv, FAMILY, SERVER_PORT);
	zassert_ok(stun_client_query(sock, &srv, addr_len(FAMILY), 2000, &mapped));

	/* Three datagrams came before the answer, two of them with an address
	 * that is a lie. What is returned is what the answer said.
	 */
	zassert_true(is_loopback(&mapped, FAMILY));
	zassert_equal(port_of(&mapped), local_port(sock), "mapped port %u", port_of(&mapped));
	zassert_equal(atomic_get(&server.requests), 1);
}

ZTEST(stun_client, test_truncated_datagram_is_not_the_answer)
{
	struct net_sockaddr srv, mapped;
	int sock = client_socket(FAMILY);
	int rc;

	/* RFC 8489 sections 5 and 6.3: a STUN message fills its datagram. The
	 * server sends CONFIG_STUN_CLIENT_RX_BUF_SIZE + 4 bytes, the first
	 * CONFIG_STUN_CLIENT_RX_BUF_SIZE of them a well-formed response with a
	 * lie in it. Whatever the receive buffer holds, the client has to know
	 * that the datagram was longer than what it got, and treat it like any
	 * other datagram that is not the answer: ignore it and go on waiting,
	 * so the query times out instead of returning the lie.
	 */
	atomic_set(&server.mode, MODE_OVERSIZED);
	set_loopback(&srv, FAMILY, SERVER_PORT);
	memset(&mapped, 0, sizeof(mapped));
	rc = stun_client_query(sock, &srv, addr_len(FAMILY), 1200, &mapped);
	zassert_equal(atomic_get(&server.sent_len), CONFIG_STUN_CLIENT_RX_BUF_SIZE + 4,
		      "the server could not build the oversized datagram");
	zassert_true(atomic_get(&server.requests) >= 1);
	zassert_equal(rc, -ETIMEDOUT,
		      "a truncated datagram was taken for the answer: rc %d, "
		      "mapped port %u",
		      rc, port_of(&mapped));
}

ZTEST(stun_client, test_two_queries_on_one_socket)
{
	struct net_sockaddr srv, first, second;
	int sock = client_socket(FAMILY);

	set_loopback(&srv, FAMILY, SERVER_PORT);
	zassert_ok(stun_client_query(sock, &srv, addr_len(FAMILY), 2000, &first));
	zassert_ok(stun_client_query(sock, &srv, addr_len(FAMILY), 2000, &second));

	zassert_equal(atomic_get(&server.requests), 2);
	zassert_true(memcmp(server.txid[0], server.txid[1], STUN_TXID_SIZE) != 0,
		     "every transaction has an id of its own");
	zassert_equal(port_of(&first), port_of(&second), "the same socket, the same address");
}

ZTEST(stun_client, test_many_calls_exceed_the_socket_limit)
{
	int socks[MANY_CALLS];
	int n = 0;

	/* What the two tests below rest on: fewer than MANY_CALLS sockets can be
	 * open at once.
	 */
	while (n < MANY_CALLS) {
		socks[n] = zsock_socket(FAMILY, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
		if (socks[n] < 0) {
			break;
		}
		n++;
	}
	zassert_true(n > 0 && n < MANY_CALLS, "%d sockets could be opened", n);
	while (n > 0) {
		zassert_ok(zsock_close(socks[--n]));
	}
}

ZTEST(stun_client, test_simple)
{
	struct net_sockaddr mapped;

	/* Port 0 is the default port; another one is where the request goes. */
	zassert_ok(stun_client_simple(HOST, 0, 2000, &mapped));
	zassert_true(is_loopback(&mapped, FAMILY));
	zassert_true(port_of(&mapped) != 0);
	zassert_equal(atomic_get(&server.last_port), STUN_CLIENT_DEFAULT_PORT);

	zassert_ok(stun_client_simple(HOST, OTHER_PORT, 2000, &mapped));
	zassert_true(is_loopback(&mapped, FAMILY));
	zassert_equal(atomic_get(&server.last_port), OTHER_PORT);

	/* It opens a socket of the family the name resolves to. */
	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		zassert_ok(stun_client_simple("127.0.0.1", SERVER_PORT, 2000, &mapped));
		zassert_true(is_loopback(&mapped, NET_AF_INET));
	}
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		zassert_ok(stun_client_simple("::1", SERVER_PORT, 2000, &mapped));
		zassert_true(is_loopback(&mapped, NET_AF_INET6));
	}

	/* And closes it. */
	for (int i = 0; i < MANY_CALLS; i++) {
		zassert_ok(stun_client_simple(HOST, SERVER_PORT, 2000, &mapped), "call %d", i);
	}
}

ZTEST(stun_client, test_simple_failures_do_not_leak)
{
	struct net_sockaddr mapped;

	atomic_set(&server.mode, MODE_SILENT);
	for (int i = 0; i < MANY_CALLS; i++) {
		/* A name nothing resolves, and a server that never answers. */
		zassert_equal(stun_client_simple("no-such-host.invalid", 0, 100, &mapped),
			      -EHOSTUNREACH, "call %d", i);
		zassert_equal(stun_client_simple(HOST, SERVER_PORT, 10, &mapped), -ETIMEDOUT,
			      "call %d", i);
	}

	atomic_set(&server.mode, MODE_ANSWER);
	zassert_ok(stun_client_simple(HOST, SERVER_PORT, 2000, &mapped));
}

ZTEST(stun_client, test_bad_arguments)
{
	struct net_sockaddr srv, mapped;
	int sock = client_socket(FAMILY);

	set_loopback(&srv, FAMILY, SERVER_PORT);

	zassert_equal(stun_client_query(sock, NULL, addr_len(FAMILY), 100, &mapped), -EINVAL);
	zassert_equal(stun_client_query(sock, &srv, addr_len(FAMILY), 100, NULL), -EINVAL);
	zassert_equal(stun_client_query(-1, &srv, addr_len(FAMILY), 100, &mapped), -EBADF);

	/* An address length too short for the family is refused before the
	 * address is read and before the socket is touched.
	 */
	zassert_equal(stun_client_query(sock, &srv, addr_len(FAMILY) - 1U, 100, &mapped), -EINVAL);
	zassert_equal(stun_client_query(-1, &srv, addr_len(FAMILY) - 1U, 100, &mapped), -EINVAL);
	zassert_equal(stun_client_query(-1, &srv, 0, 100, &mapped), -EINVAL);

	set_loopback(&srv, FAMILY, 0);
	zassert_equal(stun_client_query(sock, &srv, addr_len(FAMILY), 100, &mapped), -EINVAL,
		      "a server without a port");

	set_loopback(&srv, FAMILY, SERVER_PORT);
	srv.sa_family = NET_AF_UNSPEC;
	zassert_equal(stun_client_query(sock, &srv, addr_len(FAMILY), 100, &mapped), -EAFNOSUPPORT);

#if !defined(CONFIG_NET_IPV6)
	{
		struct net_sockaddr_in6 srv6 = {
			.sin6_family = NET_AF_INET6,
			.sin6_port = net_htons(SERVER_PORT),
		};

		zassert_equal(stun_client_query(sock, (struct net_sockaddr *)&srv6, sizeof(srv6),
						100, &mapped),
			      -EAFNOSUPPORT, "an IPv6 server without the IPv6 stack");
	}
#endif

	zassert_equal(stun_client_simple(NULL, SERVER_PORT, 100, &mapped), -EINVAL);
	zassert_equal(stun_client_simple(HOST, SERVER_PORT, 100, NULL), -EINVAL);

	zassert_equal(atomic_get(&server.requests), 0, "nothing was sent");
}
