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

/* Is the address the loopback address of the family? The port is not compared. */
static bool is_loopback(const struct net_sockaddr *sa, int family)
{
	static const uint8_t loopback4[NET_IPV4_ADDR_SIZE] = {127, 0, 0, 1};

	if (sa->sa_family != family) {
		return false;
	}
	if (family == NET_AF_INET) {
		return memcmp(net_sin(sa)->sin_addr.s4_addr, loopback4, sizeof(loopback4)) == 0;
	}
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		/* The address ::1: every byte zero but the last. */
		static const uint8_t loopback6[NET_IPV6_ADDR_SIZE] = {[15] = 1};

		return memcmp(net_sin6(sa)->sin6_addr.s6_addr, loopback6, sizeof(loopback6)) == 0;
	}
	return false;
}

/* Start the scripted server before the first test. */
static void *suite_setup(void)
{
	server_start();
	return NULL;
}

/* Put the server back to answering every request, with its counters at zero. */
static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	server_reset();
}

/* Close the socket the test opened with client_socket(), if any. */
static void after_each(void *fixture)
{
	ARG_UNUSED(fixture);
	if (client_sock >= 0) {
		(void)zsock_close(client_sock);
		client_sock = -1;
	}
}

ZTEST_SUITE(stun_client, NULL, suite_setup, before_each, after_each, NULL);

/* Open a UDP socket of the family and keep it in client_sock for after_each()
 * to close.
 */
static int client_socket(int family)
{
	client_sock = udp_socket(family);
	return client_sock;
}

/* The port a socket is bound to. */
static uint16_t local_port(int sock)
{
	struct net_sockaddr_storage local = {0};
	net_socklen_t len = sizeof(local);

	memset(&local, 0, sizeof(local));
	zassert_ok(zsock_getsockname(sock, net_sad(&local), &len), "getsockname: %d", errno);
	return port_of(net_sad(&local));
}

/* One query to the server of a family; what comes back is this very socket. */
static void check_query(int family)
{
	struct net_sockaddr_storage srv = {0};
	struct net_sockaddr_storage mapped = {0};
	int sock = client_socket(family);

	set_loopback(net_sad(&srv), family, SERVER_PORT);
	memset(&mapped, 0, sizeof(mapped));
	zassert_ok(
		stun_client_query(sock, net_sad(&srv), addr_len(family), 2000, &mapped));

	zassert_true(is_loopback(net_sad(&mapped), family));
	zassert_true(port_of(net_sad(&mapped)) != 0);
	zassert_equal(port_of(net_sad(&mapped)), local_port(sock), "mapped port %u, socket port %u",
		      port_of(net_sad(&mapped)), local_port(sock));
	zassert_equal(atomic_get(&server.requests), 1);
	/* RFC 8489 section 6.1: a request should say what sent it, unless told not to. */
	zassert_equal(atomic_get(&server.software_seen) != 0,
		      sizeof(CONFIG_STUN_CLIENT_SOFTWARE) > 1U, "SOFTWARE in the request");
}

/* A query over IPv4 returns the loopback address and the port of the querying
 * socket.
 */
ZTEST(stun_client, test_query_ipv4)
{
	if (!IS_ENABLED(CONFIG_NET_IPV4)) {
		ztest_test_skip();
	}
	check_query(NET_AF_INET);
}

/* A query over IPv6 returns the loopback address and the port of the querying
 * socket.
 */
ZTEST(stun_client, test_query_ipv6)
{
	if (!IS_ENABLED(CONFIG_NET_IPV6)) {
		ztest_test_skip();
	}
	check_query(NET_AF_INET6);
}

/* A silent server: the query returns -ETIMEDOUT after the time of the caller,
 * having sent the requests the schedule has in that time.
 */
ZTEST(stun_client, test_query_timeout)
{
	struct net_sockaddr_storage srv = {0};
	struct net_sockaddr_storage mapped = {0};
	int sock = client_socket(FAMILY);
	int64_t took, start;

	atomic_set(&server.mode, MODE_SILENT);
	set_loopback(net_sad(&srv), FAMILY, SERVER_PORT);

	/* Shorter than the first retransmission interval, 500 ms: the call
	 * returns when the caller said, not when the schedule has a deadline.
	 */
	start = k_uptime_get();
	zassert_equal(
		stun_client_query(sock, net_sad(&srv), addr_len(FAMILY), 300, &mapped),
		-ETIMEDOUT);
	took = k_uptime_get() - start;
	zassert_true(took >= 300 && took < 500, "gave up after %lld ms", took);
	zassert_equal(atomic_get(&server.requests), 1);

	/* Longer than it: the request is sent again in the meantime. */
	atomic_set(&server.requests, 0);
	start = k_uptime_get();
	zassert_equal(
		stun_client_query(sock, net_sad(&srv), addr_len(FAMILY), 700, &mapped),
		-ETIMEDOUT);
	took = k_uptime_get() - start;
	zassert_true(took >= 700 && took < 900, "gave up after %lld ms", took);
	zassert_equal(atomic_get(&server.requests), 2);
}

/* A silent server and timeout 0: the query sends seven requests and returns
 * -ETIMEDOUT after 39.5 seconds.
 */
ZTEST(stun_client, test_query_gives_up_when_the_schedule_ends)
{
	struct net_sockaddr_storage srv = {0};
	struct net_sockaddr_storage mapped = {0};
	int64_t took, start;
	int sock;

	/* 39.5 seconds: nothing where they are real ones. */
	if (!IS_ENABLED(CONFIG_ARCH_POSIX)) {
		ztest_test_skip();
	}

	sock = client_socket(FAMILY);
	atomic_set(&server.mode, MODE_SILENT);
	set_loopback(net_sad(&srv), FAMILY, SERVER_PORT);

	/* No time of the caller's: the schedule of RFC 8489 section 6.2.1 runs out,
	 * seven transmissions and then 8 seconds of waiting.
	 */
	start = k_uptime_get();
	zassert_equal(stun_client_query(sock, net_sad(&srv), addr_len(FAMILY), 0, &mapped),
		      -ETIMEDOUT);
	took = k_uptime_get() - start;
	zassert_true(took >= 39500 && took < 40500, "gave up after %lld ms", took);
	zassert_equal(atomic_get(&server.requests), 7);
}

/* The server answers the second request only: the query sends the same request
 * again after 500 ms and succeeds.
 */
ZTEST(stun_client, test_query_retransmits)
{
	struct net_sockaddr_storage srv = {0};
	struct net_sockaddr_storage mapped = {0};
	int sock = client_socket(FAMILY);
	int64_t took, start;

	atomic_set(&server.mode, MODE_ANSWER_SECOND);
	set_loopback(net_sad(&srv), FAMILY, SERVER_PORT);

	start = k_uptime_get();
	zassert_ok(stun_client_query(sock, net_sad(&srv), addr_len(FAMILY), 0, &mapped));
	took = k_uptime_get() - start;

	/* The same request again, 500 ms after the first (RFC 8489 section 6.2.1). */
	zassert_equal(atomic_get(&server.requests), 2);
	zassert_mem_equal(server.txid[0], server.txid[1], STUN_TXID_SIZE);
	zassert_true(took >= 500 && took < 1000, "answered after %lld ms", took);
	zassert_true(is_loopback(net_sad(&mapped), FAMILY));
	zassert_equal(port_of(net_sad(&mapped)), local_port(sock));
}

/* An error response ends the query with -EPROTO after one request. */
ZTEST(stun_client, test_query_error_response)
{
	struct net_sockaddr_storage srv = {0};
	struct net_sockaddr_storage mapped = {0};
	int sock = client_socket(FAMILY);

	atomic_set(&server.mode, MODE_ERROR_500);
	set_loopback(net_sad(&srv), FAMILY, SERVER_PORT);
	zassert_equal(
		stun_client_query(sock, net_sad(&srv), addr_len(FAMILY), 2000, &mapped),
		-EPROTO);
	zassert_equal(atomic_get(&server.requests), 1, "an error response is not retried");
}

/* Datagrams that are not the answer arrive first: the query returns the address
 * of the answer.
 */
ZTEST(stun_client, test_query_ignores_what_is_not_the_answer)
{
	struct net_sockaddr_storage srv = {0};
	struct net_sockaddr_storage mapped = {0};
	int sock = client_socket(FAMILY);

	atomic_set(&server.mode, MODE_NOISE_FIRST);
	set_loopback(net_sad(&srv), FAMILY, SERVER_PORT);
	zassert_ok(
		stun_client_query(sock, net_sad(&srv), addr_len(FAMILY), 2000, &mapped));

	/* Three datagrams came before the answer, two of them with an address
	 * that is a lie. What is returned is what the answer said.
	 */
	zassert_true(is_loopback(net_sad(&mapped), FAMILY));
	zassert_equal(port_of(net_sad(&mapped)), local_port(sock), "mapped port %u",
		      port_of(net_sad(&mapped)));
	zassert_equal(atomic_get(&server.requests), 1);
}

/* A datagram longer than the receive buffer is ignored, and the query times
 * out.
 */
ZTEST(stun_client, test_truncated_datagram_is_not_the_answer)
{
	struct net_sockaddr_storage srv = {0};
	struct net_sockaddr_storage mapped = {0};
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
	set_loopback(net_sad(&srv), FAMILY, SERVER_PORT);
	memset(&mapped, 0, sizeof(mapped));
	rc = stun_client_query(sock, net_sad(&srv), addr_len(FAMILY), 1200, &mapped);
	zassert_equal(atomic_get(&server.sent_len), CONFIG_STUN_CLIENT_RX_BUF_SIZE + 4,
		      "the server could not build the oversized datagram");
	zassert_true(atomic_get(&server.requests) >= 1);
	zassert_equal(rc, -ETIMEDOUT,
		      "a truncated datagram was taken for the answer: rc %d, "
		      "mapped port %u",
		      rc, port_of(net_sad(&mapped)));
}

/* Two queries on one socket: two transaction ids, one mapped port. */
ZTEST(stun_client, test_two_queries_on_one_socket)
{
	struct net_sockaddr_storage srv = {0};
	struct net_sockaddr_storage first = {0};
	struct net_sockaddr_storage second = {0};
	int sock = client_socket(FAMILY);

	set_loopback(net_sad(&srv), FAMILY, SERVER_PORT);
	zassert_ok(stun_client_query(sock, net_sad(&srv), addr_len(FAMILY), 2000, &first));
	zassert_ok(stun_client_query(sock, net_sad(&srv), addr_len(FAMILY), 2000, &second));

	zassert_equal(atomic_get(&server.requests), 2);
	zassert_true(memcmp(server.txid[0], server.txid[1], STUN_TXID_SIZE) != 0,
		     "every transaction has an id of its own");
	zassert_equal(port_of(net_sad(&first)), port_of(net_sad(&second)),
		      "the same socket, the same address");
}

/* Sockets are opened until the stack refuses one: fewer than MANY_CALLS can be
 * open at once.
 */
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
		n--;
		zassert_ok(zsock_close(socks[n]));
	}
}

/* stun_client_simple() takes port 0 for the default port, opens a socket of the
 * family of the address and closes it after every call.
 */
ZTEST(stun_client, test_simple)
{
	struct net_sockaddr_storage mapped = {0};

	/* Port 0 is the default port; another one is where the request goes. */
	zassert_ok(stun_client_simple(HOST, 0, 2000, &mapped));
	zassert_true(is_loopback(net_sad(&mapped), FAMILY));
	zassert_true(port_of(net_sad(&mapped)) != 0);
	zassert_equal(atomic_get(&server.last_port), STUN_CLIENT_DEFAULT_PORT);

	zassert_ok(stun_client_simple(HOST, OTHER_PORT, 2000, &mapped));
	zassert_true(is_loopback(net_sad(&mapped), FAMILY));
	zassert_equal(atomic_get(&server.last_port), OTHER_PORT);

	/* It opens a socket of the family the name resolves to. */
	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		zassert_ok(stun_client_simple("127.0.0.1", SERVER_PORT, 2000, &mapped));
		zassert_true(is_loopback(net_sad(&mapped), NET_AF_INET));
	}
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		zassert_ok(stun_client_simple("::1", SERVER_PORT, 2000, &mapped));
		zassert_true(is_loopback(net_sad(&mapped), NET_AF_INET6));
	}

	/* And closes it. */
	for (int i = 0; i < MANY_CALLS; i++) {
		zassert_ok(stun_client_simple(HOST, SERVER_PORT, 2000, &mapped), "call %d",
			   i);
	}
}

/* stun_client_simple() with datagrams that are not the answer arriving first:
 * it returns the address of the answer.
 */
ZTEST(stun_client, test_simple_ignores_what_is_not_the_answer)
{
	struct net_sockaddr_storage mapped = {0};

	atomic_set(&server.mode, MODE_NOISE_FIRST);
	zassert_ok(stun_client_simple(HOST, SERVER_PORT, 2000, &mapped));

	/* Three datagrams came before the answer, one of them a response to the
	 * request sent from another port of the server, with an address that is
	 * a lie. What is returned is what the answer said.
	 */
	zassert_true(is_loopback(net_sad(&mapped), FAMILY));
	zassert_equal(port_of(net_sad(&mapped)), atomic_get(&server.last_client_port),
		      "mapped port %u", port_of(net_sad(&mapped)));
	zassert_equal(atomic_get(&server.requests), 1);
}

/* MANY_CALLS calls that do not resolve and MANY_CALLS that time out, then one
 * that succeeds: no failed call kept a socket.
 */
ZTEST(stun_client, test_simple_failures_do_not_leak)
{
	struct net_sockaddr_storage mapped = {0};

	atomic_set(&server.mode, MODE_SILENT);
	for (int i = 0; i < MANY_CALLS; i++) {
		/* A name nothing resolves, and a server that never answers. */
		zassert_equal(stun_client_simple("no-such-host.invalid", 0, 100, &mapped),
			      -EHOSTUNREACH, "call %d", i);
		zassert_equal(stun_client_simple(HOST, SERVER_PORT, 10, &mapped),
			      -ETIMEDOUT, "call %d", i);
	}

	atomic_set(&server.mode, MODE_ANSWER);
	zassert_ok(stun_client_simple(HOST, SERVER_PORT, 2000, &mapped));
}

/* Call stun_client_simple() for DUAL_HOST twice, with the mode of the server
 * limited to IPv4 and then to IPv6, and expect the address the other family
 * answered with. Returns the time the two calls took.
 */
static int64_t simple_with_each_family_failing(uint32_t timeout_ms)
{
	static const int families[] = {NET_AF_INET, NET_AF_INET6};
	struct net_sockaddr_storage mapped = {0};
	int64_t start = k_uptime_get();

	ARRAY_FOR_EACH(families, i) {
		int family = families[i];
		int other = (family == NET_AF_INET) ? NET_AF_INET6 : NET_AF_INET;
		bool from_other = false;
		int rc = 0;

		atomic_set(&server.mode_family, family);
		rc = stun_client_simple(DUAL_HOST, SERVER_PORT, timeout_ms, &mapped);
		zassert_ok(rc, "family %d fails", family);

		from_other = is_loopback(net_sad(&mapped), other);
		zassert_true(from_other, "family %d fails", family);
	}
	return k_uptime_get() - start;
}

/* A name with an IPv4 and an IPv6 address, one of which is silent:
 * stun_client_simple() waits for it for half of the time the caller said and
 * asks the other.
 */
ZTEST(stun_client, test_simple_tries_the_next_address_after_a_timeout)
{
	int64_t took = 0;

	if (!IS_ENABLED(CONFIG_DNS_RESOLVER)) {
		ztest_test_skip();
	}

	atomic_set(&server.mode, MODE_SILENT);
	took = simple_with_each_family_failing(600);

	/* Whichever address the resolver puts first, it is the silent one in
	 * one of the two calls: one request and 300 ms there, half of the time,
	 * then the request that is answered. The other call needs one request.
	 */
	zassert_equal(atomic_get(&server.requests), 3);
	zassert_true(took >= 300 && took < 600, "the calls took %lld ms", took);
}

/* The same with an address that answers with an error: the other is asked at
 * once.
 */
ZTEST(stun_client, test_simple_tries_the_next_address_after_an_error)
{
	if (!IS_ENABLED(CONFIG_DNS_RESOLVER)) {
		ztest_test_skip();
	}

	atomic_set(&server.mode, MODE_ERROR_500);
	(void)simple_with_each_family_failing(300);
	zassert_equal(atomic_get(&server.requests), 3);
}

/* A name with two addresses that are both silent: each gets a request and half
 * of the time the caller said, the call fails and keeps no socket.
 */
ZTEST(stun_client, test_simple_fails_when_no_address_answers)
{
	struct net_sockaddr_storage mapped = {0};
	int64_t start = 0;
	int64_t took = 0;

	if (!IS_ENABLED(CONFIG_DNS_RESOLVER)) {
		ztest_test_skip();
	}

	atomic_set(&server.mode, MODE_SILENT);
	start = k_uptime_get();
	zassert_equal(stun_client_simple(DUAL_HOST, SERVER_PORT, 200, &mapped),
		      -ETIMEDOUT);
	took = k_uptime_get() - start;
	zassert_equal(atomic_get(&server.requests), 2);
	zassert_true(took >= 200 && took < 400, "gave up after %lld ms", took);

	for (int i = 0; i < MANY_CALLS; i++) {
		zassert_equal(stun_client_simple(DUAL_HOST, SERVER_PORT, 10, &mapped),
			      -ETIMEDOUT, "call %d", i);
	}

	atomic_set(&server.mode, MODE_ANSWER);
	zassert_ok(stun_client_simple(DUAL_HOST, SERVER_PORT, 2000, &mapped));
}

/* The same with no time given: each of the two addresses gets the whole
 * schedule, seven transmissions in 39.5 seconds.
 */
ZTEST(stun_client, test_simple_gives_each_address_the_whole_schedule)
{
	struct net_sockaddr_storage mapped = {0};
	int64_t start = 0;
	int64_t took = 0;

	/* 79 seconds: nothing where they are real ones. */
	if (!IS_ENABLED(CONFIG_DNS_RESOLVER) || !IS_ENABLED(CONFIG_ARCH_POSIX)) {
		ztest_test_skip();
	}

	atomic_set(&server.mode, MODE_SILENT);
	start = k_uptime_get();
	zassert_equal(stun_client_simple(DUAL_HOST, SERVER_PORT, 0, &mapped), -ETIMEDOUT);
	took = k_uptime_get() - start;
	zassert_equal(atomic_get(&server.requests), 14);
	zassert_true(took >= 79000 && took < 80000, "gave up after %lld ms", took);
}

/* Bad arguments are refused with their errno, and nothing is sent. */
ZTEST(stun_client, test_bad_arguments)
{
	struct net_sockaddr_storage srv = {0};
	struct net_sockaddr_storage mapped = {0};
	int sock = client_socket(FAMILY);

	set_loopback(net_sad(&srv), FAMILY, SERVER_PORT);

	zassert_equal(stun_client_query(sock, NULL, addr_len(FAMILY), 100, &mapped),
		      -EINVAL);
	zassert_equal(stun_client_query(sock, net_sad(&srv), addr_len(FAMILY), 100, NULL), -EINVAL);
	zassert_equal(stun_client_query(-1, net_sad(&srv), addr_len(FAMILY), 100, &mapped),
		      -EBADF);

	/* An address length too short for the family is refused before the
	 * address is read and before the socket is touched.
	 */
	zassert_equal(stun_client_query(sock, net_sad(&srv), addr_len(FAMILY) - 1U, 100,
					&mapped),
		      -EINVAL);
	zassert_equal(
		stun_client_query(-1, net_sad(&srv), addr_len(FAMILY) - 1U, 100, &mapped),
		-EINVAL);
	zassert_equal(stun_client_query(-1, net_sad(&srv), 0, 100, &mapped), -EINVAL);

	set_loopback(net_sad(&srv), FAMILY, 0);
	zassert_equal(
		stun_client_query(sock, net_sad(&srv), addr_len(FAMILY), 100, &mapped),
		-EINVAL, "a server without a port");

	memset(&srv, 0, sizeof(srv));
	srv.ss_family = FAMILY;
	zassert_ok(net_port_set(net_sad(&srv), SERVER_PORT));
	zassert_equal(
		stun_client_query(sock, net_sad(&srv), addr_len(FAMILY), 100, &mapped),
		-EINVAL, "a server without an address");

	set_loopback(net_sad(&srv), FAMILY, SERVER_PORT);
	srv.ss_family = NET_AF_UNSPEC;
	zassert_equal(
		stun_client_query(sock, net_sad(&srv), addr_len(FAMILY), 100, &mapped),
		-EAFNOSUPPORT);

	if (!IS_ENABLED(CONFIG_NET_IPV6)) {
		struct net_sockaddr_in6 srv6 = {
			.sin6_family = NET_AF_INET6,
			.sin6_port = net_htons(SERVER_PORT),
		};

		zassert_equal(stun_client_query(sock, (struct net_sockaddr *)&srv6, sizeof(srv6),
						100, &mapped),
			      -EAFNOSUPPORT, "an IPv6 server without the IPv6 stack");
	}

	zassert_equal(stun_client_simple(NULL, SERVER_PORT, 100, &mapped), -EINVAL);
	zassert_equal(stun_client_simple(HOST, SERVER_PORT, 100, NULL), -EINVAL);
	zassert_equal(stun_client_simple(IS_ENABLED(CONFIG_NET_IPV4) ? "0.0.0.0" : "::",
					 SERVER_PORT, 100, &mapped),
		      -EINVAL, "a server without an address");

	zassert_equal(atomic_get(&server.requests), 0, "nothing was sent");
}
