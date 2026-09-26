/*
 * Copyright (c) 2026 YunHung Hua
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "usbc_timer.h"

/*
 * The timers the stack uses live in statically allocated state machine
 * structs, so they start out zeroed. The tests below use zeroed storage for
 * the same reason: usbc_timer_init() sets up the kernel timer and the timeout,
 * but leaves the status flags untouched.
 */

/* Long enough to observe the timer running, short enough to keep the test quick. */
#define TIMEOUT_MS 20

/* Slack added when waiting for a timeout to elapse. */
#define SLACK_MS 10

static void wait_for_timeout(void)
{
	k_msleep(TIMEOUT_MS + SLACK_MS);
}

ZTEST(usbc_timer, test_init_leaves_timer_idle)
{
	struct usbc_timer_t timer = {0};

	usbc_timer_init(&timer, TIMEOUT_MS);

	zassert_false(usbc_timer_running(&timer), "timer runs before being started");
	zassert_false(usbc_timer_expired(&timer), "timer expires before being started");
}

ZTEST(usbc_timer, test_started_timer_runs_until_it_expires)
{
	struct usbc_timer_t timer = {0};

	usbc_timer_init(&timer, TIMEOUT_MS);
	usbc_timer_start(&timer);

	zassert_true(usbc_timer_running(&timer), "timer does not run after being started");
	zassert_false(usbc_timer_expired(&timer), "timer expires before its timeout elapses");

	wait_for_timeout();

	zassert_true(usbc_timer_expired(&timer), "timer does not expire after its timeout");
}

/*
 * usbc_timer_expired() reports an expiry once and then clears it, so that a
 * state machine polling it does not act on the same timeout twice.
 */
ZTEST(usbc_timer, test_expiry_is_reported_once)
{
	struct usbc_timer_t timer = {0};

	usbc_timer_init(&timer, TIMEOUT_MS);
	usbc_timer_start(&timer);
	wait_for_timeout();

	zassert_true(usbc_timer_expired(&timer), "first poll does not report the expiry");
	zassert_false(usbc_timer_expired(&timer), "second poll reports the expiry again");
}

/*
 * The timer stays "running" after the timeout elapses and is only cleared once
 * the expiry has been reported.
 */
ZTEST(usbc_timer, test_timer_runs_until_expiry_is_reported)
{
	struct usbc_timer_t timer = {0};

	usbc_timer_init(&timer, TIMEOUT_MS);
	usbc_timer_start(&timer);
	wait_for_timeout();

	zassert_true(usbc_timer_running(&timer), "timer stops before the expiry is reported");

	(void)usbc_timer_expired(&timer);

	zassert_false(usbc_timer_running(&timer), "timer runs after the expiry is reported");
}

ZTEST(usbc_timer, test_stopped_timer_does_not_expire)
{
	struct usbc_timer_t timer = {0};

	usbc_timer_init(&timer, TIMEOUT_MS);
	usbc_timer_start(&timer);
	usbc_timer_stop(&timer);

	zassert_false(usbc_timer_running(&timer), "timer runs after being stopped");

	wait_for_timeout();

	zassert_false(usbc_timer_expired(&timer), "stopped timer expires");
	zassert_false(usbc_timer_running(&timer), "stopped timer runs after its timeout");
}

ZTEST(usbc_timer, test_start_with_value_replaces_the_timeout)
{
	struct usbc_timer_t timer = {0};

	/* Initialized with a timeout that would outlast the test. */
	usbc_timer_init(&timer, TIMEOUT_MS * 100);
	usbc_timer_start_with_value(&timer, TIMEOUT_MS);

	wait_for_timeout();

	zassert_true(usbc_timer_expired(&timer),
		     "timer does not use the value it was started with");
}

ZTEST(usbc_timer, test_timer_can_be_restarted_after_expiring)
{
	struct usbc_timer_t timer = {0};

	usbc_timer_init(&timer, TIMEOUT_MS);
	usbc_timer_start(&timer);
	wait_for_timeout();
	zassert_true(usbc_timer_expired(&timer), "timer does not expire on the first run");

	usbc_timer_start(&timer);

	zassert_true(usbc_timer_running(&timer), "restarted timer does not run");
	zassert_false(usbc_timer_expired(&timer), "restarted timer reports the previous expiry");

	wait_for_timeout();

	zassert_true(usbc_timer_expired(&timer), "timer does not expire on the second run");
}

/*
 * Stopping the timer must also cancel the kernel timer behind it, so that it
 * does not stay armed and wake the system for an expiry nothing will act on.
 */
ZTEST(usbc_timer, test_stop_cancels_the_kernel_timer)
{
	struct usbc_timer_t timer = {0};

	usbc_timer_init(&timer, TIMEOUT_MS);
	usbc_timer_start(&timer);

	zassert_not_equal(k_timer_remaining_get(&timer.timer), 0,
			  "kernel timer is not armed while the timer runs");

	usbc_timer_stop(&timer);

	zassert_equal(k_timer_remaining_get(&timer.timer), 0,
		      "kernel timer stays armed after the timer is stopped");
}

ZTEST_SUITE(usbc_timer, NULL, NULL, NULL, NULL, NULL);
