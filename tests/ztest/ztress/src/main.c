/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ztress.h>

volatile int ztress_dummy;

bool ztress_handler_busy(void *user_data, uint32_t cnt, bool last, int prio)
{
	/* On nios2 k_busy_wait in k_timer handler context is hanging. */
	if (IS_ENABLED(CONFIG_NIOS2) && k_is_in_isr()) {
		for (int i = 0; i < 1000; i++) {
			ztress_dummy++;
		}
	} else {
		k_busy_wait((prio+1) * 100);
	}

	return true;
}

ZTEST(ztress, test_timeout)
{
	int64_t d;
	uint32_t repeat = 1000000;
	k_timeout_t t = Z_TIMEOUT_TICKS(20);
	int err;
	int timeout = 1000;

	ztress_set_timeout(K_MSEC(timeout));

	d = k_uptime_get();

	/* Set of two threads */
	ZTRESS_EXECUTE(ZTRESS_THREAD(ztress_handler_busy, NULL, repeat, 0, t),
		       ZTRESS_THREAD(ztress_handler_busy, NULL, repeat, 1000, t));

	d = k_uptime_get() - d;
	zassert_within(d, 1000, 200);

	/* Set of two threads and timer. Test is setup manually, without helper macro. */
	struct ztress_context_data timer_data =
		ZTRESS_CONTEXT_INITIALIZER(ztress_handler_busy, NULL, repeat, 0, t);
	struct ztress_context_data thread_data[] = {
		ZTRESS_CONTEXT_INITIALIZER(ztress_handler_busy, NULL, repeat, 1000, t),
		ZTRESS_CONTEXT_INITIALIZER(ztress_handler_busy, NULL, repeat, 1000, t)
	};

	d = k_uptime_get();
	err = ztress_execute(&timer_data, thread_data, ARRAY_SIZE(thread_data));
	zassert_equal(err, 0, "ztress_execute failed (err: %d)", err);
	d = k_uptime_get() - d;
	zassert_within(d, timeout + 500, 500);

	ztress_set_timeout(K_NO_WAIT);
}

static void timeout_abort(struct k_timer *timer)
{
	ztress_abort();
}

ZTEST(ztress, test_abort)
{
	struct k_timer timer;
	uint32_t repeat = 10000000;

	k_timer_init(&timer, timeout_abort, NULL);
	k_timer_start(&timer, K_MSEC(100), K_NO_WAIT);

	ZTRESS_EXECUTE(ZTRESS_THREAD(ztress_handler_busy, NULL, repeat, 0, K_MSEC(1)),
		       ZTRESS_THREAD(ztress_handler_busy, NULL, repeat, 0, K_MSEC(1)));

	zassert_true(ztress_exec_count(0) < repeat);
	zassert_true(ztress_exec_count(1) < repeat);
}

ZTEST(ztress, test_repeat_completion)
{
	uint32_t repeat = 10;

	/* Set of two threads */
	ZTRESS_EXECUTE(ZTRESS_THREAD(ztress_handler_busy, NULL, repeat, 0, Z_TIMEOUT_TICKS(20)),
		       ZTRESS_THREAD(ztress_handler_busy, NULL, repeat, 0, Z_TIMEOUT_TICKS(20)));

	for (int i = 0; i < 2; i++) {
		uint32_t exec_cnt = ztress_exec_count(i);

		zassert_true(exec_cnt >= repeat && exec_cnt < repeat + 10);
	}

	/* Set of two threads and timer */
	ZTRESS_EXECUTE(ZTRESS_TIMER(ztress_handler_busy, NULL, repeat, Z_TIMEOUT_TICKS(30)),
		       ZTRESS_THREAD(ztress_handler_busy, NULL, repeat, 0, Z_TIMEOUT_TICKS(30)),
		       ZTRESS_THREAD(ztress_handler_busy, NULL, repeat, 0, Z_TIMEOUT_TICKS(30)));

	for (int i = 0; i < 3; i++) {
		uint32_t exec_cnt = ztress_exec_count(i);

		zassert_true(exec_cnt >= repeat && exec_cnt < repeat + 10);
	}
}

ZTEST(ztress, test_no_context_requirements)
{
	uint32_t repeat = 10;

	/* Set of two threads. First thread has no ending condition (exec_cnt and
	 * preempt_cnt are 0)
	 */
	ZTRESS_EXECUTE(ZTRESS_THREAD(ztress_handler_busy, NULL, 0, 0, Z_TIMEOUT_TICKS(20)),
		       ZTRESS_THREAD(ztress_handler_busy, NULL, repeat, 0, Z_TIMEOUT_TICKS(20)));

	uint32_t exec_cnt = ztress_exec_count(1);

	zassert_true(exec_cnt >= repeat && exec_cnt < repeat + 10, "exec_cnt: %u", exec_cnt);

	/* Set of two threads. Second thread and timer context has no ending
	 * condition (exec_cnt and preempt_cnt are 0).
	 */
	ZTRESS_EXECUTE(ZTRESS_TIMER(ztress_handler_busy, NULL, 0, Z_TIMEOUT_TICKS(30)),
		       ZTRESS_THREAD(ztress_handler_busy, NULL, repeat, 0, Z_TIMEOUT_TICKS(30)),
		       ZTRESS_THREAD(ztress_handler_busy, NULL, 0, 0, Z_TIMEOUT_TICKS(30)));

	exec_cnt = ztress_exec_count(1);
	zassert_true(exec_cnt >= repeat && exec_cnt < repeat + 10);
}

ZTEST(ztress, test_too_many_threads)
{
	uint32_t repeat = 10;
	k_timeout_t t = Z_TIMEOUT_TICKS(20);
	int err;

	/* Negative check on too many threads set and a timer.
	 * Assuming ZTRESS_MAX_THREADS=3
	 */
	struct ztress_context_data timer_data =
		ZTRESS_CONTEXT_INITIALIZER(ztress_handler_busy, NULL, repeat, 0, t);
	struct ztress_context_data thread_data[] = {
		ZTRESS_CONTEXT_INITIALIZER(ztress_handler_busy, NULL, repeat, 1000, t),
		ZTRESS_CONTEXT_INITIALIZER(ztress_handler_busy, NULL, repeat, 1000, t),
		ZTRESS_CONTEXT_INITIALIZER(ztress_handler_busy, NULL, repeat, 1000, t)
	};

	err = ztress_execute(&timer_data, thread_data, ARRAY_SIZE(thread_data));
	zassert_equal(err, -EINVAL, "ztress_execute: unexpected err=%d (expected -EINVAL)", err);
}

ZTEST_SUITE(ztress, NULL, NULL, NULL, NULL, NULL);
