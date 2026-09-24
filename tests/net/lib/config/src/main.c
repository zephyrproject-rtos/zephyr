/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: Copyright 2026 inovex GmbH
 */

/*
 * Tests for selecting the net_config SNTP server at runtime.
 *
 * A minimal SNTP server runs on loopback and answers whatever reaches it.
 * The Kconfig server is a name that cannot be resolved, so which server a
 * query went to is visible in whether the clock was set.
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_config.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/clock.h>
#include <zephyr/ztest.h>

#include "sntp_pkt.h"

#define MODE_SERVER 4
#define VERSION     4

/* The time the test server serves, and a different one the tests write to
 * the clock beforehand, so "the clock was set" and "the clock was left
 * alone" tell each other apart.
 */
#define SERVED_TIME    1000000000
#define UNTOUCHED_TIME 500000000

#define SNTP_SERVER_STACK_SIZE 2048
#define SNTP_SERVER_PRIORITY   K_PRIO_PREEMPT(8)

static int sntp_server_fd = -1;
static struct k_thread sntp_server_thread_data;
static K_THREAD_STACK_DEFINE(sntp_server_stack, SNTP_SERVER_STACK_SIZE);

/* Cleared by the teardown so the server thread can return on its own. It
 * is aborting a thread parked in a receive that leaves the run without an
 * event to end on.
 */
static volatile bool sntp_server_running;

/* Answer every request that arrives, echoing the client's transmit
 * timestamp back as the originate timestamp so the client accepts it.
 */
static void sntp_server_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (sntp_server_running) {
		struct net_sockaddr_in client;
		net_socklen_t client_len = sizeof(client);
		struct sntp_pkt pkt;
		int ret;

		ret = zsock_recvfrom(sntp_server_fd, &pkt, sizeof(pkt), 0,
				     (struct net_sockaddr *)&client, &client_len);
		if (ret != (int)sizeof(pkt)) {
			continue;
		}

		pkt.orig_tm_s = pkt.tx_tm_s;
		pkt.orig_tm_f = pkt.tx_tm_f;
		pkt.li = 0;
		pkt.vn = VERSION;
		pkt.mode = MODE_SERVER;
		pkt.stratum = 2;
		pkt.poll = 4;
		pkt.precision = -6;
		pkt.rx_tm_s = net_htonl(OFFSET_1970_JAN_1 + SERVED_TIME);
		pkt.rx_tm_f = 0;
		pkt.tx_tm_s = net_htonl(OFFSET_1970_JAN_1 + SERVED_TIME);
		pkt.tx_tm_f = 0;

		(void)zsock_sendto(sntp_server_fd, &pkt, sizeof(pkt), 0,
				   (struct net_sockaddr *)&client, client_len);
	}
}

static void set_clock(time_t seconds)
{
	struct timespec tspec = {.tv_sec = seconds, .tv_nsec = 0};

	zassert_ok(sys_clock_settime(SYS_CLOCK_REALTIME, &tspec),
		   "could not prime the realtime clock");
}

static time_t get_clock(void)
{
	struct timespec tspec;

	zassert_ok(sys_clock_gettime(SYS_CLOCK_REALTIME, &tspec),
		   "could not read the realtime clock");

	return tspec.tv_sec;
}

/* The Kconfig server does not resolve, so a query that reached a server at
 * all can only have used the runtime setting.
 */
ZTEST(net_config_sntp, test_runtime_server_overrides_kconfig)
{
	zassert_true(sizeof(CONFIG_NET_CONFIG_SNTP_INIT_SERVER) > 1,
		     "the test needs a Kconfig server to be overridden");

	set_clock(UNTOUCHED_TIME);

	zassert_ok(net_config_sntp_set_server("127.0.0.1"), "could not set the server");
	zassert_ok(net_config_init_clock_via_sntp(), "the SNTP query failed");

	zassert_between_inclusive(get_clock(), SERVED_TIME, SERVED_TIME + 1,
				  "the Kconfig server was used instead of the runtime one");
}

/* Clearing the runtime server puts the unresolvable Kconfig server back in
 * charge, so the query fails and the clock keeps the value it had.
 */
ZTEST(net_config_sntp, test_clearing_falls_back_to_kconfig)
{
	zassert_ok(net_config_sntp_set_server("127.0.0.1"), "could not set the server");
	zassert_ok(net_config_sntp_set_server(NULL), "could not clear the server");

	set_clock(UNTOUCHED_TIME);

	zassert_not_equal(net_config_init_clock_via_sntp(), 0,
			  "a query to the unresolvable Kconfig server reported success");
	zassert_between_inclusive(get_clock(), UNTOUCHED_TIME, UNTOUCHED_TIME + 1,
				  "the clock was changed by a failed query");
}

/* The name is rejected whole: a truncated server is worse than no change. */
ZTEST(net_config_sntp, test_too_long_name_is_rejected)
{
	static const char too_long[] = "0123456789abcdef0";

	BUILD_ASSERT(sizeof(too_long) - 1 > CONFIG_NET_CONFIG_SNTP_INIT_SERVER_MAX_LEN,
		     "the test name has to exceed the configured maximum");

	zassert_ok(net_config_sntp_set_server("127.0.0.1"), "could not set the server");

	zassert_equal(net_config_sntp_set_server(too_long), -ENAMETOOLONG,
		      "an over-long server name was accepted");
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)net_config_sntp_set_server(NULL);
}

static void *setup(void)
{
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(SNTP_PORT),
		.sin_addr = {{{127, 0, 0, 1}}},
	};
	/* Keeps the server thread out of an unbounded wait, so it notices
	 * the teardown promptly.
	 */
	struct timeval timeout = {.tv_sec = 0, .tv_usec = 100000};

	sntp_server_fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(sntp_server_fd >= 0, "could not open the server socket");

	zassert_ok(zsock_bind(sntp_server_fd, (struct net_sockaddr *)&addr, sizeof(addr)),
		   "could not bind the server socket");

	zassert_ok(zsock_setsockopt(sntp_server_fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &timeout,
				    sizeof(timeout)),
		   "could not set the server socket's receive timeout");

	sntp_server_running = true;

	k_thread_create(&sntp_server_thread_data, sntp_server_stack,
			K_THREAD_STACK_SIZEOF(sntp_server_stack), sntp_server_thread, NULL, NULL,
			NULL, SNTP_SERVER_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&sntp_server_thread_data, "sntp_server");

	return NULL;
}

static void teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	sntp_server_running = false;
	zassert_ok(k_thread_join(&sntp_server_thread_data, K_SECONDS(2)),
		   "the server thread did not stop");

	(void)zsock_close(sntp_server_fd);
	sntp_server_fd = -1;
}

ZTEST_SUITE(net_config_sntp, NULL, setup, before, NULL, teardown);
