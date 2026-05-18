/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cpu_workload/cpu_workload.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define PROFILE_THREAD_STACK_SIZE 1024
#define PROFILE_THREAD_PRIORITY K_PRIO_PREEMPT(0)
#define BURN_ITERATIONS 20000U

K_SEM_DEFINE(profile_start_sem, 0, 1);
K_SEM_DEFINE(profile_done_sem, 0, 1);
K_SEM_DEFINE(work_done_sem, 0, 1);

K_THREAD_STACK_DEFINE(profile_thread_stack, PROFILE_THREAD_STACK_SIZE);
static struct k_thread profile_thread;
static bool profile_thread_started;
static volatile uint32_t burn_sink;

static void burn_cycles(void)
{
	for (uint32_t i = 0U; i < BURN_ITERATIONS; i++) {
		burn_sink += i;
	}
}

static void profile_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&profile_start_sem, K_FOREVER);
		burn_cycles();
		k_sem_give(&profile_done_sem);
	}
}

static void prime_thread_profile(void)
{
	k_thread_runtime_stats_t stats;
	int ret;

	k_sem_give(&profile_start_sem);
	zassert_ok(k_sem_take(&profile_done_sem, K_SECONDS(1)));
	k_sleep(K_MSEC(10));

	ret = k_thread_runtime_stats_get(&profile_thread, &stats);
	zassert_ok(ret);
	zassert_true(stats.window_count > 0U, "thread stats did not collect windows");
	zassert_true(stats.total_cycles > 0U, "thread stats has no total cycles");
}

static void *workload_test_setup(void)
{
	if (profile_thread_started) {
		return NULL;
	}

	k_thread_create(&profile_thread, profile_thread_stack,
			K_THREAD_STACK_SIZEOF(profile_thread_stack), profile_thread_entry,
			NULL, NULL, NULL, PROFILE_THREAD_PRIORITY, 0, K_NO_WAIT);
	profile_thread_started = true;

	return NULL;
}

ZTEST(cpu_workload_estimator, test_ready_backlog_and_arrival)
{
	struct cpu_workload_ready_backlog backlog;
	struct cpu_workload_arrival arrival;
	int ret;

	prime_thread_profile();

	k_sched_lock();
	k_sem_give(&profile_start_sem);

	ret = cpu_workload_ready_backlog_get(0, &backlog);
	zassert_ok(ret);
	zassert_true(backlog.runnable_threads > 0U, "expected runnable thread");
	zassert_true(backlog.profiled_threads > 0U, "expected profiled runnable thread");
	zassert_true(backlog.ready_backlog_cycles > 0U, "expected backlog cycles");
	zassert_true((backlog.source_mask & CPU_WORKLOAD_SOURCE_READY_BACKLOG) != 0U);
	zassert_true((backlog.source_mask & CPU_WORKLOAD_SOURCE_THREAD_BURST_PROFILE) != 0U);
	zassert_true(backlog.confidence > 0U, "expected backlog confidence");

	ret = cpu_workload_arrival_get(0, &arrival);
	zassert_ok(ret);
	zassert_true(arrival.arrivals > 0U, "expected attributed arrival");
	zassert_true(arrival.profiled_arrivals > 0U, "expected profiled arrival");
	zassert_true(arrival.expected_arrival_cycles > 0U, "expected arrival cycles");
	zassert_true((arrival.source_mask & CPU_WORKLOAD_SOURCE_ARRIVAL) != 0U);

	ret = cpu_workload_arrival_get(0, &arrival);
	zassert_ok(ret);
	zassert_equal(arrival.arrivals, 0U, "arrival workload was not consumed");
	zassert_equal(arrival.source_mask, 0U, "empty arrival workload should not report sources");

	k_sched_unlock();
	zassert_ok(k_sem_take(&profile_done_sem, K_SECONDS(1)));
}

ZTEST(cpu_workload_estimator, test_unified_estimate)
{
	struct cpu_workload_estimate estimate;
	int ret;

	prime_thread_profile();

	k_sched_lock();
	k_sem_give(&profile_start_sem);

	ret = cpu_workload_estimate_get(0, &estimate);
	zassert_ok(ret);
	zassert_true(estimate.ready_backlog_cycles > 0U, "expected backlog contribution");
	zassert_true(estimate.expected_arrival_cycles > 0U, "expected arrival contribution");
	zassert_true(estimate.estimated_cycles >= estimate.ready_backlog_cycles,
		     "estimate should include backlog");
	zassert_true(estimate.estimated_cycles >= estimate.expected_arrival_cycles,
		     "estimate should include arrival");
	zassert_true((estimate.source_mask & CPU_WORKLOAD_SOURCE_READY_BACKLOG) != 0U);
	zassert_true((estimate.source_mask & CPU_WORKLOAD_SOURCE_ARRIVAL) != 0U);
	zassert_true(estimate.confidence > 0U, "expected estimate confidence");

	k_sched_unlock();
	zassert_ok(k_sem_take(&profile_done_sem, K_SECONDS(1)));

	burn_cycles();

	ret = cpu_workload_estimate_get(0, &estimate);
	zassert_ok(ret);
	zassert_true((estimate.source_mask & CPU_WORKLOAD_SOURCE_RUNTIME_HISTORY) != 0U,
		     "expected runtime-history contribution");
	zassert_true(estimate.history_cycles > 0U, "expected runtime-history cycles");
	zassert_true(estimate.history_window_us > 0U, "expected runtime-history window");
}

static void profiled_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	burn_cycles();
	k_sem_give(&work_done_sem);
}

K_WORK_DEFINE(profiled_work, profiled_work_handler);

ZTEST(cpu_workload_estimator, test_work_handler_profile)
{
	struct cpu_workload_work_profile profile;
	int ret;

	ret = k_work_submit(&profiled_work);
	zassert_true(ret >= 0, "work submit failed: %d", ret);
	zassert_ok(k_sem_take(&work_done_sem, K_SECONDS(1)));

	ret = cpu_workload_work_get(&profiled_work, &profile);
	zassert_ok(ret);
	zassert_true(profile.sample_count > 0U, "work profile did not collect samples");
	zassert_true(profile.handler_cycles > 0U, "work profile has no handler cycles");
	zassert_true((profile.source_mask & CPU_WORKLOAD_SOURCE_WORK_PROFILE) != 0U);
	zassert_true(profile.confidence > 0U, "work profile has no confidence");
}

ZTEST_SUITE(cpu_workload_estimator, NULL, workload_test_setup, NULL, NULL, NULL);
