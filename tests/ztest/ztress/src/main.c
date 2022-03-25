/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <ztress.h>

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

static void test_timeout(void)
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
	zassert_within(d, 1000, 200, NULL);

	/* Set of two threads and timer. Test is setup manually, without helper macro. */
	struct ztress_context_data timer_data =
		ZTRESS_CONTEXT_INITIALIZER(ztress_handler_busy, NULL, repeat, 0, t);
	struct ztress_context_data thread_data[] = {
		ZTRESS_CONTEXT_INITIALIZER(ztress_handler_busy, NULL, repeat, 1000, t),
		ZTRESS_CONTEXT_INITIALIZER(ztress_handler_busy, NULL, repeat, 1000, t)
	};

	d = k_uptime_get();
	err = ztress_execute(&timer_data, thread_data, ARRAY_SIZE(thread_data));
	d = k_uptime_get() - d;
	zassert_within(d, timeout + 500, 500, NULL);

	ztress_set_timeout(K_NO_WAIT);
}

static void timeout_abort(struct k_timer *timer)
{
	ztress_abort();
}

static void test_abort(void)
{
	struct k_timer timer;
	uint32_t repeat = 10000000;

	k_timer_init(&timer, timeout_abort, NULL);
	k_timer_start(&timer, K_MSEC(100), K_NO_WAIT);

	ZTRESS_EXECUTE(ZTRESS_THREAD(ztress_handler_busy, NULL, repeat, 0, K_MSEC(1)),
		       ZTRESS_THREAD(ztress_handler_busy, NULL, repeat, 0, K_MSEC(1)));

	zassert_true(ztress_exec_count(0) < repeat, NULL);
	zassert_true(ztress_exec_count(1) < repeat, NULL);
}

static void test_repeat_completion(void)
{
	uint32_t repeat = 10;

	/* Set of two threads */
	ZTRESS_EXECUTE(ZTRESS_THREAD(ztress_handler_busy, NULL, repeat, 0, Z_TIMEOUT_TICKS(20)),
		       ZTRESS_THREAD(ztress_handler_busy, NULL, repeat, 0, Z_TIMEOUT_TICKS(20)));

	for (int i = 0; i < 2; i++) {
		uint32_t exec_cnt = ztress_exec_count(i);

		zassert_true(exec_cnt >= repeat && exec_cnt < repeat + 10, NULL);
	}

	/* Set of two threads and timer */
	ZTRESS_EXECUTE(ZTRESS_TIMER(ztress_handler_busy, NULL, repeat, Z_TIMEOUT_TICKS(30)),
		       ZTRESS_THREAD(ztress_handler_busy, NULL, repeat, 0, Z_TIMEOUT_TICKS(30)),
		       ZTRESS_THREAD(ztress_handler_busy, NULL, repeat, 0, Z_TIMEOUT_TICKS(30)));

	for (int i = 0; i < 3; i++) {
		uint32_t exec_cnt = ztress_exec_count(i);

		zassert_true(exec_cnt >= repeat && exec_cnt < repeat + 10, NULL);
	}
}

static void test_no_context_requirements(void)
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
	zassert_true(exec_cnt >= repeat && exec_cnt < repeat + 10, NULL);
}

void test_main(void)
{
	ztest_test_suite(ztress_tests,
			 ztest_unit_test(test_timeout),
			 ztest_unit_test(test_abort),
			 ztest_unit_test(test_repeat_completion),
			 ztest_unit_test(test_no_context_requirements)
			 );

	ztest_run_test_suite(ztress_tests);
}
