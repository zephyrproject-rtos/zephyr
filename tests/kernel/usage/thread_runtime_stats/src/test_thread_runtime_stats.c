/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#define HELPER_STACK_SIZE 500

/**
 * @brief Verify @a va1 and @a val2 are within @a pcnt % of each other
 */
#define TEST_WITHIN_X_PERCENT(val1, val2, pcnt)                       \
	((((val1) * 100) < ((val2) * (100 + (pcnt)))) &&              \
	 (((val1) * 100) > ((val2) * (100 - (pcnt))))) ? true : false

#if defined(CONFIG_RISCV)
#define IDLE_EVENT_STATS_PRECISION 7
#elif defined(CONFIG_QEMU_TARGET)
#define IDLE_EVENT_STATS_PRECISION 3
#else
#define IDLE_EVENT_STATS_PRECISION 1
#endif

static struct k_thread helper_thread;
static K_THREAD_STACK_DEFINE(helper_stack, HELPER_STACK_SIZE);

static struct k_thread *main_thread;

/**
 * @brief Helper thread to test_thread_runtime_stats_get()
 */
void helper1(void *p1, void *p2, void *p3)
{
	while (1) {
	}
}

/**
 * @brief Busy wait for specified number of ticks
 */
void busy_loop(uint32_t ticks)
{
	uint32_t  tick = sys_clock_tick_get_32();

	while (sys_clock_tick_get_32() < (tick + ticks)) {
	}
}

/**
 * @brief Test the k_threads_runtime_stats_all_get() API
 *
 * 1. Create a helper thread.
 * 2. Busy loop for 2 ticks.
 *    - Idle time should not increase.
 * 3. Sleep for two ticks. Helper executes and busy loops.
 *    - Idle time should not increase
 * 4. Kill helper thread, and sleep for 2 ticks
 *    - Idle time should increase
 * 5. Busy loop for 3 ticks
 *    - Idle time should not increase
 *    - current, peak and average cycles should be different
 */
ZTEST(usage_api, test_all_stats_usage)
{
	int  priority;
	k_tid_t  tid;
	k_thread_runtime_stats_t  stats1;
	k_thread_runtime_stats_t  stats2;
	k_thread_runtime_stats_t  stats3;
	k_thread_runtime_stats_t  stats4;
	k_thread_runtime_stats_t  stats5;

	priority = k_thread_priority_get(_current);
	tid = k_thread_create(&helper_thread, helper_stack,
			      K_THREAD_STACK_SIZEOF(helper_stack),
			      helper1, NULL, NULL, NULL,
			      priority + 2, 0, K_NO_WAIT);

	k_thread_runtime_stats_all_get(&stats1);

	busy_loop(2);   /* Busy wait 2 ticks */

	k_thread_runtime_stats_all_get(&stats2);

	k_sleep(K_TICKS(2));  /* Helper runs for 2 ticks */

	k_thread_runtime_stats_all_get(&stats3);

	k_thread_abort(tid);

	k_sleep(K_TICKS(2));  /* Idle for 2 ticks */

	k_thread_runtime_stats_all_get(&stats4);

	busy_loop(3);   /* Busy wait for 3 ticks */

	k_thread_runtime_stats_all_get(&stats5);

	/*
	 * Verify that before the system idles for 2 ticks that
	 * [execution_cycles] is increasing, [total_cycles + idle_cycles] matches
	 * [execution_cycles] and [idle_cycles] is not changing (as the
	 * system is not going to idle during that test).
	 */

	zassert_true(stats2.execution_cycles > stats1.execution_cycles);
	zassert_true(stats3.execution_cycles > stats2.execution_cycles);
	zassert_true(stats1.execution_cycles == (stats1.total_cycles + stats1.idle_cycles));
	zassert_true(stats2.execution_cycles == (stats2.total_cycles + stats2.idle_cycles));
	zassert_true(stats3.execution_cycles == (stats3.total_cycles + stats3.idle_cycles));
#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
	zassert_true(stats1.idle_cycles == stats2.idle_cycles);
	zassert_true(stats1.idle_cycles == stats3.idle_cycles);
#endif

#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS

	/*
	 * The analysis fields should behave as follows prior to the system
	 * going idle.
	 * 1. [current_cycles] increases.
	 * 2. [peak_cycles] matches [current_cycles].
	 * 3. [average_cycles] is 0 if system has not gone idle yet
	 * 4. [current_cycles] matches [execution_cycles] if system has not gone idle yet
	 */

	zassert_true(stats2.current_cycles > stats1.current_cycles);
	zassert_true(stats3.current_cycles > stats2.current_cycles);

	zassert_true(stats1.peak_cycles == stats1.current_cycles);
	zassert_true(stats2.peak_cycles == stats2.current_cycles);
	zassert_true(stats3.peak_cycles == stats3.current_cycles);

	if (stats1.idle_cycles == 0) {
		zassert_true(stats1.average_cycles == 0);
		zassert_true(stats2.average_cycles == 0);
		zassert_true(stats3.average_cycles == 0);

		zassert_true(stats1.current_cycles == stats1.execution_cycles);
		zassert_true(stats2.current_cycles == stats2.execution_cycles);
		zassert_true(stats3.current_cycles == stats3.execution_cycles);
	} else {
		zassert_true(stats1.current_cycles < stats1.execution_cycles);
		zassert_true(stats2.current_cycles < stats2.execution_cycles);
		zassert_true(stats3.current_cycles < stats3.execution_cycles);
	}
#endif

	/*
	 * Now process the statistics after the idle event.
	 *
	 * 1. [execution_cycles] continues to increase
	 * 2. [total_cycles] increases
	 * 3. [current_cycles] had a reset event but still increases
	 * 4. [peak_cycles] does not change
	 * 5. [average_cycles] increases
	 * 6. [idle_cycles] increased once.
	 */

	zassert_true(stats4.execution_cycles > stats3.execution_cycles);
	zassert_true(stats5.execution_cycles > stats4.execution_cycles);

	/*
	 * If the frequency is low enough, the [total_cycles] might not
	 * increase between sample points 3 and 4. Count this as acceptable.
	 */

	zassert_true(stats4.total_cycles >= stats3.total_cycles);
	zassert_true(stats5.total_cycles > stats4.total_cycles);

#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
	zassert_true(stats4.current_cycles <= stats1.current_cycles);
	zassert_true(stats5.current_cycles > stats4.current_cycles);

	zassert_true(TEST_WITHIN_X_PERCENT(stats4.peak_cycles,
					   stats3.peak_cycles, IDLE_EVENT_STATS_PRECISION), NULL);
	zassert_true(stats4.peak_cycles == stats5.peak_cycles);

	zassert_true(stats4.average_cycles > 0);
	zassert_true(stats5.average_cycles > stats4.average_cycles);
#endif

#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
	zassert_true(stats4.idle_cycles > stats3.idle_cycles);
	zassert_true(stats4.idle_cycles == stats5.idle_cycles);
#endif
}

#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
/**
 * @brief Test the k_thread_runtime_stats_enable/disable APIs
 */
ZTEST(usage_api, test_thread_stats_enable_disable)
{
	k_tid_t  tid;
	k_thread_runtime_stats_t  stats1;
	k_thread_runtime_stats_t  stats2;
	k_thread_runtime_stats_t  helper_stats1;
	k_thread_runtime_stats_t  helper_stats2;
	k_thread_runtime_stats_t  helper_stats3;
	int  priority;

	priority = k_thread_priority_get(_current);
	tid = k_thread_create(&helper_thread, helper_stack,
			      K_THREAD_STACK_SIZEOF(helper_stack),
			      helper1, NULL, NULL, NULL,
			      priority + 2, 0, K_NO_WAIT);

	/*
	 * Sleep to let the helper thread execute for some time before
	 * disabling the runtime stats on the helper thread.
	 */

	k_sleep(K_TICKS(5));

	k_thread_runtime_stats_get(_current, &stats1);
	k_thread_runtime_stats_get(tid, &helper_stats1);
	k_thread_runtime_stats_disable(tid);

	/*
	 * Busy wait for the remaining tick before re-enabling the thread
	 * runtime stats on the helper thread.
	 */

	busy_loop(1);

	/* Sleep for two ticks to let the helper thread execute. */

	k_sleep(K_TICKS(2));

	k_thread_runtime_stats_enable(tid);
	k_thread_runtime_stats_get(_current, &stats2);
	k_thread_runtime_stats_get(tid, &helper_stats2);

	/* Sleep for two ticks to let the helper thread execute again. */

	k_sleep(K_TICKS(2));

	k_thread_runtime_stats_get(tid, &helper_stats3);

	/*
	 * Verify that the between sample sets 1 and 2 that additional stats
	 * were not gathered for the helper thread, but were gathered for the
	 * main current thread.
	 */

	zassert_true(helper_stats1.execution_cycles ==
		     helper_stats2.execution_cycles, NULL);
	zassert_true(stats1.execution_cycles < stats2.execution_cycles);

	/*
	 * Verify that between sample sets 2 and 3 that additional stats were
	 * gathered for the helper thread.
	 */

	zassert_true(helper_stats2.execution_cycles <
		     helper_stats3.execution_cycles, NULL);

	k_thread_abort(tid);
}
#else
ZTEST(usage_api, test_thread_stats_enable_disable)
{
}
#endif

#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
/**
 * @brief Test the k_sys_runtime_stats_enable/disable APIs
 */
ZTEST(usage_api, test_sys_stats_enable_disable)
{
	k_thread_runtime_stats_t  sys_stats1;
	k_thread_runtime_stats_t  sys_stats2;
	k_thread_runtime_stats_t  sys_stats3;
	k_thread_runtime_stats_t  thread_stats1;
	k_thread_runtime_stats_t  thread_stats2;
	k_thread_runtime_stats_t  thread_stats3;

	/*
	 * Disable system runtime stats gathering.
	 * This should not impact thread runtime stats gathering.
	 */

	k_sys_runtime_stats_disable();

	k_thread_runtime_stats_get(_current, &thread_stats1);
	k_thread_runtime_stats_all_get(&sys_stats1);

	busy_loop(2);

	k_thread_runtime_stats_get(_current, &thread_stats2);
	k_thread_runtime_stats_all_get(&sys_stats2);

	/*
	 * Enable system runtime stats gathering.
	 * This should not impact thread runtime stats gathering.
	 */

	k_sys_runtime_stats_enable();

	busy_loop(2);

	k_thread_runtime_stats_get(_current, &thread_stats3);
	k_thread_runtime_stats_all_get(&sys_stats3);

	/*
	 * There ought to be no differences between sys_stat1 and sys_stat2.
	 * Although a memory compare of the two structures would be sufficient,
	 * each individual field is being tested in case to more easily
	 * isolate the cause of any error.
	 */

	zassert_true(sys_stats1.execution_cycles == sys_stats2.execution_cycles,
		     NULL);
	zassert_true(sys_stats1.total_cycles == sys_stats2.total_cycles);

#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
	zassert_true(sys_stats1.current_cycles == sys_stats2.current_cycles,
		     NULL);
	zassert_true(sys_stats1.peak_cycles == sys_stats2.peak_cycles);
	zassert_true(sys_stats1.average_cycles == sys_stats2.average_cycles,
		     NULL);
#endif

#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
	zassert_true(sys_stats1.idle_cycles == sys_stats2.idle_cycles);
#endif

	/*
	 * As only system stats have been disabled, thread stats should be
	 * unaffected. To simplify things, just check [execution_cycles] and
	 * [current_cycles] (if enabled).
	 */

	zassert_true(thread_stats1.execution_cycles <
		     thread_stats2.execution_cycles, NULL);

#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
	zassert_true(thread_stats1.current_cycles <
		     thread_stats2.current_cycles, NULL);
#endif

	/*
	 * Now verify that the enabling of system runtime stats gathering
	 * has resulted in the gathering of system runtime stats. Again,
	 * thread runtime stats gathering should be unaffected.
	 */

	zassert_true(sys_stats2.execution_cycles < sys_stats3.execution_cycles,
		     NULL);
	zassert_true(sys_stats2.total_cycles < sys_stats3.total_cycles);

#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS

	/*
	 * As enabling reset [current_cycles], it is not easy to predict
	 * what its value should be. For now, settle for ensuring that it
	 * is different and not zero.
	 */

	zassert_true(sys_stats2.current_cycles != sys_stats3.current_cycles,
		     NULL);
	zassert_true(sys_stats2.current_cycles != 0);
	zassert_true(sys_stats2.peak_cycles == sys_stats3.peak_cycles);
	zassert_true(sys_stats2.average_cycles > sys_stats3.average_cycles,
		     NULL);
#endif

#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
	zassert_true(sys_stats2.idle_cycles == sys_stats3.idle_cycles);
#endif
}
#else
ZTEST(usage_api, test_sys_stats_enable_disable)
{
}
#endif

/**
 * @brief Timer handler to resume the main thread
 */
void resume_main(struct k_timer *timer)
{
	k_thread_resume(main_thread);
}

/**
 * @brief Test the k_thread_runtime_stats_get() API
 *
 * This routine tests the k_thread_runtime_stats_get() routine. It verifies
 * that the contents of the fields guarded by CONFIG_SCHED_THREAD_USAGE
 * are correct.
 */
ZTEST(usage_api, test_thread_stats_usage)
{
	k_tid_t  tid;
	int  priority;
	int  status;
	struct k_timer   timer;
	k_thread_runtime_stats_t  stats1;
	k_thread_runtime_stats_t  stats2;
	k_thread_runtime_stats_t  stats3;

	priority = k_thread_priority_get(_current);

	/*
	 * Verify that k_thread_runtime_stats_get() returns the expected
	 * values for error cases.
	 */

	status = k_thread_runtime_stats_get(NULL, &stats1);
	zassert_true(status == -EINVAL);

	status = k_thread_runtime_stats_get(_current, NULL);
	zassert_true(status == -EINVAL);

	/* Align to the next tick */

	k_sleep(K_TICKS(1));

	/* Create a low priority helper thread to start in 1 tick. */

	tid = k_thread_create(&helper_thread, helper_stack,
			      K_THREAD_STACK_SIZEOF(helper_stack),
			      helper1, NULL, NULL, NULL,
			      priority + 2, 0, K_TICKS(1));

	main_thread = _current;
	k_timer_init(&timer, resume_main, NULL);
	k_timer_start(&timer, K_TICKS(1), K_TICKS(10));

	/* Verify thread creation succeeded */

	zassert_true(tid == &helper_thread);

	/* Get a valid set of thread runtime stats */

	status = k_thread_runtime_stats_get(tid, &stats1);
	zassert_true(status == 0);

	/*
	 * Suspend main thread. Timer will wake it 1 tick to sample
	 * the helper threads runtime stats.
	 */

	k_thread_suspend(_current);

	/*
	 * T = 1.
	 * Timer woke the main thread. Sample runtime stats for helper thread
	 * before suspending.
	 */

	k_thread_runtime_stats_get(tid, &stats1);
	k_thread_suspend(_current);

	/*
	 * T = 11.
	 * Timer woke the main thread. Suspend main thread again.
	 */

	k_thread_suspend(_current);

	/*
	 * T = 21.
	 * Timer woke the main thread. Sample runtime stats for helper thread
	 * before suspending.
	 */

	k_thread_runtime_stats_get(tid, &stats2);
	k_thread_suspend(_current);

	/*
	 * T = 31.
	 * Timer woke the main thread. Sample runtime stats for helper thread
	 * and stop the timer.
	 */

	k_thread_runtime_stats_get(tid, &stats3);
	k_timer_stop(&timer);

#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
	k_thread_runtime_stats_t  stats4;
	k_thread_runtime_stats_t  stats5;

	/*
	 * Sleep for 20 ticks, and then 1 tick. This will allow the helper
	 * thread to have two different scheduled execution windows.
	 */

	k_sleep(K_TICKS(20));
	k_thread_runtime_stats_get(tid, &stats4);

	k_sleep(K_TICKS(1));
	k_thread_runtime_stats_get(tid, &stats5);
#endif

	/* Verify execution_cycles are identical to total_cycles */

	zassert_true(stats1.execution_cycles == stats1.total_cycles);
	zassert_true(stats2.execution_cycles == stats2.total_cycles);

#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
	zassert_true(stats3.idle_cycles == 0);
#endif

	/*
	 * Verify that the time for which the helper thread executed between
	 * the first and second samplings is more than that between the
	 * second and third.
	 */

	uint64_t  diff12;
	uint64_t  diff23;

	diff12 = stats2.execution_cycles - stats1.execution_cycles;
	diff23 = stats3.execution_cycles - stats2.execution_cycles;

	zassert_true(diff12 > diff23);

#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS

	/* Verify that [current_cycles] change as expected. */

	zassert_true(stats4.current_cycles >= stats5.current_cycles);
	zassert_true(stats4.current_cycles > stats3.current_cycles);
	zassert_true(stats5.current_cycles < stats3.current_cycles);

	/* Verify that [peak_cycles] change as expected */

	zassert_true(stats4.peak_cycles > stats2.peak_cycles);
	zassert_true(stats4.peak_cycles == stats5.peak_cycles);
	zassert_true(stats4.peak_cycles == stats4.current_cycles);

	/* Verify that [average_cycles] change as expected */

	zassert_true(stats4.average_cycles > stats3.average_cycles);
	zassert_true(stats4.average_cycles > stats5.average_cycles);
	zassert_true(stats5.average_cycles >= stats3.average_cycles);
#endif

	/* Abort the helper thread */

	k_thread_abort(tid);
}

ZTEST_SUITE(usage_api, NULL, NULL,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
