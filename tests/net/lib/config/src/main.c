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

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_config.h>
#include <zephyr/ztest.h>

#include "test_util.h"

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
	sntp_test_server_start();

	return NULL;
}

static void teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	sntp_test_server_stop();
}

ZTEST_SUITE(net_config_sntp, NULL, setup, before, NULL, teardown);
