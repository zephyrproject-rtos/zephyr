/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: Copyright 2026 inovex GmbH
 */

/*
 * Tests for the periodic SNTP resync.
 *
 * The resync runs on the system work queue and reports nothing back, so the
 * tests drive it through the two inputs it has - the runtime server and the
 * server's willingness to answer - and read every outcome off the realtime
 * clock. A resync that ran against the test server leaves the clock at
 * SERVED_TIME; one that did not run, or ran against the unresolvable Kconfig
 * server, leaves it where the test put it.
 *
 * "127.0.0.1" is a numeric name, which the resolver answers from the calling
 * context, so a resync against it needs no DNS server. The Kconfig server is
 * a name and no DNS server is configured, so a resync against it fails
 * outright - which is how the tests get a failing resync.
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_config.h>
#include <zephyr/ztest.h>

#include "test_util.h"

/* Long enough that a resync arriving on the interval rather than on request
 * is not mistaken for a prompt one.
 */
#define PROMPT_RESYNC_TIMEOUT_SEC 5
#define PROMPT_RESYNC_TIMEOUT     K_SECONDS(PROMPT_RESYNC_TIMEOUT_SEC)

/* The tests raise no L4 events, so the resync has to be enabled from the
 * start, which it only is without the connection manager.
 */
BUILD_ASSERT(!IS_ENABLED(CONFIG_NET_CONFIG_SNTP_INIT_USE_CONNECTION_MANAGER),
	     "the resync tests drive no L4 events, so they cannot run with the connection "
	     "manager holding the resync disabled");

BUILD_ASSERT(CONFIG_NET_CONFIG_SNTP_INIT_RESYNC_ON_FAILURE_INTERVAL > PROMPT_RESYNC_TIMEOUT_SEC,
	     "the failure interval has to outlast the wait, or the tests cannot tell a prompt "
	     "resync from a rescheduled one");
BUILD_ASSERT(CONFIG_NET_CONFIG_SNTP_INIT_RESYNC_INTERVAL > PROMPT_RESYNC_TIMEOUT_SEC,
	     "the resync interval has to outlast the wait, or the tests cannot tell a prompt "
	     "resync from a periodic one");

/* Polls, because what a successful resync does is write the clock and there
 * is no completion the test could wait on instead.
 */
static bool wait_for_resync(k_timeout_t timeout)
{
	k_timepoint_t deadline = sys_timepoint_calc(timeout);

	do {
		if (get_clock() >= SERVED_TIME) {
			return true;
		}

		k_sleep(K_MSEC(10));
	} while (!sys_timepoint_expired(deadline));

	return false;
}

/* Nothing in the test calls net_config_init_clock_via_sntp(): setting the
 * server has to get the clock set on its own.
 */
ZTEST(net_config_sntp_resync, test_setting_the_server_resyncs)
{
	set_clock(UNTOUCHED_TIME);

	zassert_ok(net_config_sntp_set_server("127.0.0.1"), "could not set the server");

	zassert_true(wait_for_resync(PROMPT_RESYNC_TIMEOUT),
		     "setting the server did not resync the clock");
}

/* A resync has to leave its successor scheduled. Only the interval armed by
 * the first one can produce the second, since the test asks for nothing in
 * between.
 */
ZTEST(net_config_sntp_resync, test_resync_repeats_on_the_interval)
{
	set_clock(UNTOUCHED_TIME);
	zassert_ok(net_config_sntp_set_server("127.0.0.1"), "could not set the server");
	zassert_true(wait_for_resync(PROMPT_RESYNC_TIMEOUT), "the first resync did not run");

	set_clock(UNTOUCHED_TIME);

	zassert_true(wait_for_resync(K_SECONDS(CONFIG_NET_CONFIG_SNTP_INIT_RESYNC_INTERVAL + 5)),
		     "a successful resync did not schedule the next one");
}

/* A resync that fails must not leave the resync state machine wedged: the
 * next one still has to run.
 */
ZTEST(net_config_sntp_resync, test_resync_recovers_from_a_failed_query)
{
	/* No runtime server hands the unresolvable Kconfig server the query. */
	zassert_ok(net_config_sntp_set_server(NULL), "could not clear the server");

	set_clock(UNTOUCHED_TIME);
	zassert_false(wait_for_resync(K_MSEC(500)),
		      "a resync against the unresolvable Kconfig server set the clock");

	zassert_ok(net_config_sntp_set_server("127.0.0.1"), "could not set the server");

	zassert_true(wait_for_resync(PROMPT_RESYNC_TIMEOUT),
		     "no resync ran after a failed one");
}

/* A resync requested while one is in flight is deferred, not dropped, and
 * the in-flight one finishing must not push the deferred one out to the
 * failure interval.
 */
ZTEST(net_config_sntp_resync, test_resync_requested_during_a_query_is_not_lost)
{
	set_clock(UNTOUCHED_TIME);

	/* This resync gets no answer, so it is still waiting on its timeout
	 * when the request below arrives.
	 */
	sntp_test_server_set_answering(false);
	zassert_ok(net_config_sntp_set_server("127.0.0.1"), "could not set the server");

	/* Long enough for the query to have gone out, short enough to stay
	 * inside its timeout.
	 */
	k_sleep(K_MSEC(CONFIG_NET_CONFIG_SNTP_INIT_TIMEOUT / 4));

	sntp_test_server_set_answering(true);
	zassert_ok(net_config_sntp_set_server("127.0.0.1"), "could not set the server");

	zassert_true(wait_for_resync(PROMPT_RESYNC_TIMEOUT),
		     "the resync requested during an in-flight one was dropped or deferred to "
		     "the failure interval");
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	sntp_test_server_set_answering(true);

	/* Clearing the server also asks for a resync, which fails against the
	 * Kconfig server and so leaves nothing in flight for the next test.
	 */
	(void)net_config_sntp_set_server(NULL);
	k_sleep(K_MSEC(CONFIG_NET_CONFIG_SNTP_INIT_TIMEOUT * 2));
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

ZTEST_SUITE(net_config_sntp_resync, NULL, setup, before, NULL, teardown);
