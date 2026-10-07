/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 *
 * What the socket client writes to the log: the reasons its return codes do
 * not carry, as debug records, and nothing when a query simply works.
 */
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "log_capture.h"
#include "server.h"

/* What the module writes depends on the level it is built with, and on the
 * ceiling the system sets for every module.
 */
#define DBG_ON (MIN(CONFIG_STUN_LOG_LEVEL, CONFIG_LOG_MAX_LEVEL) >= LOG_LEVEL_DBG)

static int sock = -1;

static void *suite_setup(void)
{
	server_start();
	log_capture_start();
	return NULL;
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	server_reset();
	sock = udp_socket(FAMILY);
	log_capture_clear();
}

static void after_each(void *fixture)
{
	ARG_UNUSED(fixture);
	if (sock >= 0) {
		(void)zsock_close(sock);
		sock = -1;
	}
}

ZTEST_SUITE(stun_client_log, NULL, suite_setup, before_each, after_each, NULL);

/* All the records are debug records, and there are this many of them. */
static void expect_debug_records(size_t n)
{
	size_t expected = DBG_ON ? n : 0U;

	if (log_capture_count() != expected) {
		log_capture_dump();
	}
	zassert_equal(log_capture_count(), expected, "%u records",
		      (unsigned int)log_capture_count());
	zassert_equal(log_capture_count_level(LOG_LEVEL_DBG), log_capture_count(),
		      "a record above the debug level");
}

ZTEST(stun_client_log, test_an_answered_query_leaves_no_record)
{
	struct net_sockaddr srv, mapped;

	set_loopback(&srv, FAMILY, SERVER_PORT);
	zassert_ok(stun_client_query(sock, &srv, addr_len(FAMILY), 2000, &mapped));
	expect_debug_records(0);
}

ZTEST(stun_client_log, test_ignored_datagrams_are_debug_records)
{
	struct net_sockaddr srv, mapped;

	/* Three datagrams that are not the answer come first: the caller is
	 * told nothing about them, the log one line each.
	 */
	atomic_set(&server.mode, MODE_NOISE_FIRST);
	set_loopback(&srv, FAMILY, SERVER_PORT);
	zassert_ok(stun_client_query(sock, &srv, addr_len(FAMILY), 2000, &mapped));
	expect_debug_records(3);
}

ZTEST(stun_client_log, test_an_error_response_is_a_debug_record)
{
	struct net_sockaddr srv, mapped;

	/* -EPROTO does not say which error the server answered with. */
	atomic_set(&server.mode, MODE_ERROR_500);
	set_loopback(&srv, FAMILY, SERVER_PORT);
	zassert_equal(stun_client_query(sock, &srv, addr_len(FAMILY), 2000, &mapped), -EPROTO);
	expect_debug_records(1);
	if (DBG_ON) {
		zassert_true(log_capture_contains("500"));
	}
}

ZTEST(stun_client_log, test_a_name_that_does_not_resolve_is_a_debug_record)
{
	struct net_sockaddr mapped;

	/* -EHOSTUNREACH stands for every code the resolver has. */
	zassert_equal(stun_client_simple("no-such-host.invalid", 0, 100, &mapped), -EHOSTUNREACH);
	expect_debug_records(1);
	if (DBG_ON) {
		zassert_true(log_capture_contains("no-such-host.invalid"));
	}
}

ZTEST(stun_client_log, test_a_timeout_leaves_no_record)
{
	struct net_sockaddr srv, mapped;

	/* The return code says it all. */
	atomic_set(&server.mode, MODE_SILENT);
	set_loopback(&srv, FAMILY, SERVER_PORT);
	zassert_equal(stun_client_query(sock, &srv, addr_len(FAMILY), 100, &mapped), -ETIMEDOUT);
	expect_debug_records(0);
}

ZTEST(stun_client_log, test_the_end_of_the_schedule_leaves_no_record)
{
	struct net_sockaddr srv, mapped;

	/* 39.5 seconds: nothing where they are real ones. */
	if (!IS_ENABLED(CONFIG_ARCH_POSIX)) {
		ztest_test_skip();
	}

	/* The other way a query times out: not the caller's time, but the
	 * retransmissions of the transaction running out.
	 */
	atomic_set(&server.mode, MODE_SILENT);
	set_loopback(&srv, FAMILY, SERVER_PORT);
	zassert_equal(stun_client_query(sock, &srv, addr_len(FAMILY), 0, &mapped), -ETIMEDOUT);
	zassert_equal(atomic_get(&server.requests), 7);
	expect_debug_records(0);
}
