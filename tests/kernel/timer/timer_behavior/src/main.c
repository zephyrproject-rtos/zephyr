/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#define REPEAT 8

static const k_timeout_t test_timeout = K_MSEC(10);
static int64_t exp_cyc;

enum timer_premature_test_mode {
	FROM_THREAD,
	FROM_IRQ,
	FROM_IRQ_POST_DELAY,
};

/* Test data structure. */
struct test_data {
	struct k_timer timer;
	uint32_t delay;
	struct k_sem sem;

	/* In relative timeouts it hold timestamp when k_timer_start was called. */
	uint32_t start_ts;

	/* Ticks value when next time shall expire. */
	k_ticks_t ticks;

	/* Counting down number of iterations. */
	uint32_t iter;

	/* Run details when test failed. */
	uint32_t error_val;
	uint32_t error_iter;

	/* Test mode. */
	enum timer_premature_test_mode mode;

	/* if true absolute timers are used. */
	bool abs;
};

static void timer_start(struct test_data *data)
{
	k_timeout_t t;

	if (data->abs) {
		/* Get absolute timeout for test ticks from now. */
		data->ticks = k_cyc_to_ticks_near32(k_cycle_get_32()) + test_timeout.ticks;
		t = K_TIMEOUT_ABS_TICKS(data->ticks);
	} else {
		/* Store the moment when clock is started. */
		data->start_ts = k_cycle_get_32();
		t = test_timeout;
	}

	k_timer_start(&data->timer, t, K_NO_WAIT);
}

/* Expiration callback */
static void test_premature_handler(struct k_timer *timer)
{
	static const uint32_t delay_inc = k_ticks_to_us_near32(2) / REPEAT;
	struct test_data *data = CONTAINER_OF(timer, struct test_data, timer);
	int64_t now = k_cycle_get_32();
	int64_t diff = now - data->start_ts;
	bool ok;

	/* Check if timer did not expire prematurely. */
	if (data->abs) {
		ok = k_cyc_to_ticks_near32(now) >= (uint32_t)data->ticks;
	} else {
		ok = diff >= exp_cyc;
	}

	if (!ok) {
		/* Timeout occurred earlier than expected. Don't use zassert
		 * here because we are in the interrupt context.
		 */
		data->error_val = diff;
		data->error_iter = REPEAT - data->iter;
		data->iter = 0;
	} else {
		data->iter--;
	}

	/* Busy wait simulates delay between kernel timeout expiration and the moment
	 * when next timer is started. In real life application it may occur due to
	 * multiple timers expiring simultaneously, some processing happens in
	 * the timer handler or higher priority interrupt preempting current context.
	 */
	if (data->mode != FROM_IRQ_POST_DELAY) {
		k_busy_wait(data->delay);
	}

	data->delay += delay_inc;

	if (data->iter == 0) {
		/* Test end. Wake up test thread. */
		k_sem_give(&data->sem);
	} else if ((data->mode == FROM_IRQ) || (data->mode == FROM_IRQ_POST_DELAY)) {
		timer_start(data);
		if (data->mode == FROM_IRQ_POST_DELAY) {
			/* Simulated delay. */
			k_busy_wait(data->delay);
		}
	}
}

/* Test starts same single shot timer number of times. Depending on the test mode
 * next timer is started from thread or from the expiration callback.
 *
 * @param mode Test mode.
 * @param abs If true then absolute timers are started else relative.
 */
static void test_timer_premature(enum timer_premature_test_mode mode, bool abs)
{
	int err;
	static struct test_data tdata;

	memset(&tdata, 0, sizeof(tdata));
	tdata.abs = abs;
	tdata.mode = mode;
	tdata.iter = REPEAT;
	exp_cyc = k_ticks_to_cyc_near64(test_timeout.ticks);
	k_timer_init(&tdata.timer, test_premature_handler, NULL);
	k_sem_init(&tdata.sem, 0, 1);

	if (mode == FROM_THREAD) {
		for (int i = 0; i < REPEAT; i++) {
			timer_start(&tdata);
			k_msleep(k_ticks_to_ms_ceil64(test_timeout.ticks) + 5);
		}
	} else {
		timer_start(&tdata);
	}

	uint64_t ticks = test_timeout.ticks;
	uint32_t total_timeout = (uint32_t)k_ticks_to_ms_ceil64(ticks) * REPEAT + 10;

	err = k_sem_take(&tdata.sem, K_MSEC(total_timeout));
	zassert_equal(err, 0);

	zassert_equal(tdata.error_val, 0,
		"Test failed, on %d iteration timer expired earlier than expected %d, exp:%d",
		tdata.error_iter, tdata.error_val, exp_cyc);

}

/* Relative timer started from the expiration handler with variable delay added
 * after timer start.
 */
ZTEST(timer_premature, test_timer_from_irq_post_delay)
{
	test_timer_premature(FROM_IRQ_POST_DELAY, false);
}

/* Relative timer started from the expiration handler with variable delay added
 * before timer start.
 */
ZTEST(timer_premature, test_timer_from_irq)
{
	test_timer_premature(FROM_IRQ, false);
}

/* Relative timer started from the thread.
 */
ZTEST(timer_premature, test_timer_from_thread)
{
	test_timer_premature(FROM_THREAD, false);
}

/* Absolute timer started from the expiration handler with variable delay added
 * after timer start.
 */
ZTEST(timer_premature, test_abs_timer_from_irq_post_delay)
{
	test_timer_premature(FROM_IRQ_POST_DELAY, true);
}

/* Absolute timer started from the expiration handler with variable delay added
 * before timer start.
 */
ZTEST(timer_premature, test_abs_timer_from_irq)
{
	test_timer_premature(FROM_IRQ, true);
}

/* Absolute timer started from the thread.
 */
ZTEST(timer_premature, test_abs_timer_from_thread)
{
	test_timer_premature(FROM_THREAD, true);
}

ZTEST_SUITE(timer_premature, NULL, NULL, NULL, NULL, NULL);

void test_main(void)
{
	ztest_run_test_suite(timer_jitter_drift, false, 1, 1);
	ztest_run_test_suite(timer_premature, false, 1, 1);
#ifndef CONFIG_TIMER_EXTERNAL_TEST
	ztest_run_test_suite(timer_tick_train, false, 1, 1);
#endif
}
