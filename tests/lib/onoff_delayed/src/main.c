/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/onoff_delayed.h>
#include <random/rand32.h>

static struct onoff_delayed_client cli0;
static struct onoff_delayed_client cli1;
static struct onoff_delayed_client cli2;

static struct onoff_delayed_manager mgr;

static int64_t exp_timeout[10];
static bool skip_start_time_check;
static int start_cnt;
static int start_req_cnt;
static int stop_cnt;

#define INTERRUPT_LATENCY 100
#define STARTUP_TIME 1000
#define STOP_TIME 300

static void start(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	k_busy_wait(STARTUP_TIME);

	int64_t now = k_uptime_ticks();

	if (!skip_start_time_check && exp_timeout[start_cnt] != 0) {
		zassert_true(now <= exp_timeout[start_cnt],
			"Start too late %u (exp: %u), start count: %u",
			(uint32_t)now, (uint32_t)exp_timeout[start_cnt],
			start_cnt);
		zassert_true(now >= (exp_timeout[start_cnt] - K_MSEC(1).ticks),
			"Start too early %u (exp: %u, delta: %u), "
			"start count: %u",
			(uint32_t)now,
			(uint32_t)exp_timeout[start_cnt],
			(uint32_t)K_MSEC(1).ticks, start_cnt);
	}

	start_cnt++;

	notify(mgr, 0);
}

static void stop(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	stop_cnt++;
	notify(mgr, 0);
}

static struct onoff_transitions transitions = {
	.start = start,
	.stop = stop
};

static void setup(void)
{
	int err;
	uint32_t t_start =
		(uint32_t)(K_USEC(STARTUP_TIME + INTERRUPT_LATENCY).ticks);
	uint32_t t_stop = (uint32_t)(K_USEC(STOP_TIME).ticks);

	err = onoff_delayed_manager_init(&mgr, &transitions, t_start, t_stop);
	zassert_true(err >= 0, NULL);

	start_cnt = 0;
	stop_cnt = 0;
}

static void test_single_delayed_request(k_timeout_t t)
{
	int err;

	setup();

	if (K_TIMEOUT_EQ(t, K_NO_WAIT)) {
		exp_timeout[0] = 0;
	} else {
		exp_timeout[0] = t.ticks +
		((Z_TICK_ABS(t.ticks) < 0) ? k_uptime_ticks() : 0);
	}

	sys_notify_init_spinwait(&cli0.cli.notify);
	err = onoff_delayed_request(&mgr, &cli0, t);
	zassert_true(err >= 0, "Unexpected error: %d", err);

	if (!K_TIMEOUT_EQ(t, K_NO_WAIT)) {
		zassert_equal(start_cnt, 0, "Unexpected starts %d", start_cnt);
		zassert_equal(stop_cnt, 0, "Unexpected stops %d", stop_cnt);

		k_sleep(t);
	}

	/* scheduled start */
	zassert_equal(start_cnt, 1, "Unexpected starts %d", start_cnt);
	zassert_equal(stop_cnt, 0, "Unexpected stops %d", stop_cnt);

	err = onoff_delayed_release(&mgr);
	zassert_true(err >= 0, "Unexpected error: %d", err);
	zassert_equal(start_cnt, 1, "Unexpected starts %d", start_cnt);
	zassert_equal(stop_cnt, 1, "Unexpected stops %d", stop_cnt);
}

static void test_request_no_delay(void)
{
	test_single_delayed_request(K_NO_WAIT);
}

static void test_request_relative_delay(void)
{
	test_single_delayed_request(K_MSEC(10));
}

static void test_request_absolute_delay(void)
{
	k_timeout_t t = Z_TIMEOUT_TICKS(k_uptime_ticks() + K_MSEC(10).ticks);

	test_single_delayed_request(t);
}

k_timeout_t get_abs_timeout_from_now(uint32_t us)
{
	return Z_TIMEOUT_TICKS(Z_TICK_ABS(k_uptime_ticks() + K_USEC(us).ticks));
}

/**
 * Test is scheduling 3 onoff requests in 10ms, 20ms, 30ms from now. Onoff is
 * releasing in 15ms, 25ms, 35ms from now.
 */
static void test_multiple_delayed_requests(void)
{
	int err;
	k_timeout_t t0 = get_abs_timeout_from_now(30000);
	k_timeout_t t1 = get_abs_timeout_from_now(20000);
	k_timeout_t t2 = get_abs_timeout_from_now(10000);

	setup();

	exp_timeout[0] = Z_TICK_ABS(t2.ticks);
	exp_timeout[1] = Z_TICK_ABS(t1.ticks);
	exp_timeout[2] = Z_TICK_ABS(t0.ticks);

	sys_notify_init_spinwait(&cli0.cli.notify);
	err = onoff_delayed_request(&mgr, &cli0, t0);
	zassert_true(err >= 0, "Unexpected error: %d", err);

	sys_notify_init_spinwait(&cli1.cli.notify);
	err = onoff_delayed_request(&mgr, &cli1, t1);
	zassert_true(err >= 0, "Unexpected error: %d", err);

	sys_notify_init_spinwait(&cli2.cli.notify);
	err = onoff_delayed_request(&mgr, &cli2, t2);
	zassert_true(err >= 0, "Unexpected error: %d", err);

	k_msleep(15);
	zassert_equal(start_cnt, 1, "Unexpected starts %d", start_cnt);

	err = onoff_delayed_release(&mgr);
	zassert_true(err >= 0, "Unexpected error: %d", err);

	k_msleep(10);

	err = onoff_delayed_release(&mgr);
	zassert_true(err >= 0, "Unexpected error: %d", err);

	k_msleep(10);

	err = onoff_delayed_release(&mgr);
	zassert_true(err >= 0, "Unexpected error: %d", err);
	zassert_equal(start_cnt, 3, "Unexpected starts %d", start_cnt);
	zassert_equal(stop_cnt, 3, "Unexpected stops %d", stop_cnt);
}

static void test_canceling_delayed_request(void)
{
	int err;
	k_timeout_t t0 = get_abs_timeout_from_now(10000);
	k_timeout_t t1 = get_abs_timeout_from_now(20000);

	setup();

	exp_timeout[0] = Z_TICK_ABS(t1.ticks);

	sys_notify_init_spinwait(&cli0.cli.notify);
	err = onoff_delayed_request(&mgr, &cli0, t0);
	zassert_true(err >= 0, "Unexpected error: %d", err);

	sys_notify_init_spinwait(&cli1.cli.notify);
	err = onoff_delayed_request(&mgr, &cli1, t1);
	zassert_true(err >= 0, "Unexpected error: %d", err);

	onoff_delayed_cancel(&mgr, &cli0);

	k_msleep(20);

	err = onoff_delayed_release(&mgr);
	zassert_equal(start_cnt, 1, "Unexpected starts %d", start_cnt);
	zassert_equal(stop_cnt, 1, "Unexpected stops %d", stop_cnt);
}

static void test_delayed_request_when_active(void)
{
	int err;
	k_timeout_t t0 = get_abs_timeout_from_now(10000);

	setup();

	exp_timeout[0] = 0;

	sys_notify_init_spinwait(&cli0.cli.notify);
	err = onoff_request(&mgr.mgr, &cli0.cli);
	zassert_true(err >= 0, "Unexpected error: %d", err);

	sys_notify_init_spinwait(&cli1.cli.notify);
	err = onoff_delayed_request(&mgr, &cli1, t0);
	zassert_true(err >= 0, "Unexpected error: %d", err);

	zassert_equal(start_cnt, 1, "Unexpected starts %d", start_cnt);
	k_msleep(10);

	err = onoff_delayed_release(&mgr);
	zassert_true(err >= 0, "Unexpected error: %d", err);
	zassert_equal(stop_cnt, 0, "Unexpected stops %d", stop_cnt);

	err = onoff_delayed_release(&mgr);
	zassert_true(err >= 0, "Unexpected error: %d", err);
	zassert_equal(stop_cnt, 1, "Unexpected stops %d", stop_cnt);
	zassert_equal(start_cnt, 1, "Unexpected starts %d", start_cnt);
}

/* Test scenario when time between stop and next start is short (more than
 * startup time but less than stop+start time). In that case service must not
 * be stopped.
 */
static void test_skipped_turnaround(void)
{
	uint32_t delay_us = 2000;
	int err;

	setup();

	exp_timeout[0] = 0;

	sys_notify_init_spinwait(&cli0.cli.notify);
	err = onoff_delayed_request(&mgr, &cli0, K_NO_WAIT);
	zassert_true(err >= 0, "Unexpected error: %d", err);

	sys_notify_init_spinwait(&cli1.cli.notify);
	err = onoff_delayed_request(&mgr, &cli1, K_USEC(2000));
	zassert_true(err >= 0, "Unexpected error: %d", err);

	k_busy_wait(delay_us - STARTUP_TIME - STOP_TIME);
	err = onoff_delayed_cancel_or_release(&mgr, &cli0);
	zassert_true(err >= 0, "Unexpected error: %d", err);

	k_busy_wait(STARTUP_TIME + STOP_TIME + INTERRUPT_LATENCY);
	err = onoff_delayed_cancel_or_release(&mgr, &cli1);
	zassert_true(err >= 0, "Unexpected error: %d", err);

	zassert_equal(stop_cnt, 1, "Unexpected stops %d", stop_cnt);
	zassert_equal(start_cnt, 1, "Unexpected starts %d", start_cnt);

}

static void stress_test_callback(struct onoff_manager *mgr,
				 struct onoff_client *cli,
				 uint32_t state,
				 int res)
{
	struct onoff_delayed_client *dcli =
		CONTAINER_OF(cli, struct onoff_delayed_client, cli);
	int64_t t_exp = Z_TICK_ABS(dcli->timeout.ticks) +  INTERRUPT_LATENCY +
			STARTUP_TIME;
	int64_t now = z_tick_get();
	static int cnt;


	zassert_true(state == ONOFF_STATE_ON, "Unexpected state");
	zassert_true(now <= t_exp, "%d: Unexpected start time %u, exp: %d",
			cnt, (uint32_t)now, (uint32_t)t_exp);
	cnt++;
}

/* Randomly request and release service. In started callback check if start
 * was executed on time.
 */
static void test_stress(void)
{
	bool state[3] = {};
	struct onoff_delayed_client cli[3];
	uint32_t start = k_uptime_get_32();
	uint32_t test_time_ms = 5000;
	int err;

	setup();
	skip_start_time_check = true;
	start_req_cnt = 0;

	do {
		uint32_t r = sys_rand32_get();
		uint32_t idx = (r & 0xff) % 3;
		uint32_t delay = ((r >> 8) & 0xFF) % 30;

		if (state[idx]) {
			err = onoff_delayed_cancel_or_release(&mgr, &cli[idx]);
			zassert_true(err >= 0, "Unexpected error: %d", err);
		} else {
			k_timeout_t d = K_USEC((delay)*10 +
						STARTUP_TIME +
						INTERRUPT_LATENCY);

			sys_notify_init_callback(&cli[idx].cli.notify,
						 stress_test_callback);
			err = onoff_delayed_request(&mgr, &cli[idx], d);
			zassert_true(err >= 0, "Unexpected error: %d", err);
			start_req_cnt++;

		}

		state[idx] = !state[idx];
		k_busy_wait(5*delay);

	} while ((k_uptime_get_32() - start) < test_time_ms);

	PRINT("Number of start requests %u\n", start_req_cnt);
	PRINT("Number of service starts %u\n", start_cnt);
}

void test_main(void)
{
	ztest_test_suite(onoff_delayed_api,
			 ztest_unit_test(test_request_no_delay),
			 ztest_unit_test(test_request_relative_delay),
			 ztest_unit_test(test_request_absolute_delay),
			 ztest_unit_test(test_multiple_delayed_requests),
			 ztest_unit_test(test_canceling_delayed_request),
			 ztest_unit_test(test_delayed_request_when_active),
			 ztest_unit_test(test_skipped_turnaround),
			 ztest_unit_test(test_stress)
		);

	ztest_run_test_suite(onoff_delayed_api);
}
