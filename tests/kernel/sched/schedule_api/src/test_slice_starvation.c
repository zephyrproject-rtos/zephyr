/*
 * Copyright (c) 2026 TaiJu Wu
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "test_sched.h"

#ifdef CONFIG_TIMESLICING

#define SLICE_SIZE_MS 10
#define RUN_MS 500
#define BUSY_PRIO K_PRIO_PREEMPT(10)
#define H_PRIO K_PRIO_PREEMPT(8)
#define H2_PRIO K_PRIO_PREEMPT(7)
/* Below this share of the CPU time A and B get, one of them starves */
#define MIN_SHARE_PCT 10

BUILD_ASSERT(4 <= MAX_NUM_THREAD);

static struct k_thread busy_threads[2], h_thread, h2_thread;
static K_SEM_DEFINE(h2_sem, 0, 1);
static volatile uint32_t busy_count[2];
static volatile bool stop;
static int h_busy_us, h_period_ms;
static bool nested;

static void busy_entry(void *p1, void *p2, void *p3)
{
	int i = POINTER_TO_INT(p1);

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (!stop) {
		busy_count[i]++;
		/* In the posix arch, a busy loop takes no time */
		Z_SPIN_DELAY(10);
	}
}

static void h_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (!stop) {
		k_busy_wait(h_busy_us);
		if (nested) {
			k_sem_give(&h2_sem);
		}
		k_msleep(h_period_ms);
	}
}

static void h2_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while ((k_sem_take(&h2_sem, K_FOREVER) == 0) && !stop) {
		k_busy_wait(50);
	}
}

/*
 * Busy threads A and B share a priority and a time slice, and a higher
 * priority thread H preempts them more often than the slice lasts. Both
 * A and B must keep getting the CPU.
 */
static void check_no_starvation(const char *name, int busy_us, int period_ms, bool is_nested)
{
	uint64_t a, b;
	uint32_t a_pct, b_pct;

	stop = false;
	h_busy_us = busy_us;
	h_period_ms = period_ms;
	nested = is_nested;
	k_sem_reset(&h2_sem);
	for (int i = 0; i < 2; i++) {
		busy_count[i] = 0;
		k_thread_create(&busy_threads[i], tstacks[i], STACK_SIZE, busy_entry,
				INT_TO_POINTER(i), NULL, NULL, BUSY_PRIO, 0, K_NO_WAIT);
	}
	k_thread_create(&h_thread, tstacks[2], STACK_SIZE, h_entry, NULL, NULL, NULL, H_PRIO, 0,
			K_NO_WAIT);
	if (is_nested) {
		k_thread_create(&h2_thread, tstacks[3], STACK_SIZE, h2_entry, NULL, NULL, NULL,
				H2_PRIO, 0, K_NO_WAIT);
	}

	k_msleep(RUN_MS);
	stop = true;
	a = busy_count[0];
	b = busy_count[1];

	k_thread_join(&h_thread, K_FOREVER);
	if (is_nested) {
		k_sem_give(&h2_sem);
		k_thread_join(&h2_thread, K_FOREVER);
	}
	for (int i = 0; i < 2; i++) {
		k_thread_join(&busy_threads[i], K_FOREVER);
	}

	a_pct = (uint32_t)(a * 100U / (a + b + 1U));
	b_pct = (uint32_t)(b * 100U / (a + b + 1U));
	TC_PRINT("%s: A %u%%, B %u%%\n", name, a_pct, b_pct);
	zassert_true((a_pct >= MIN_SHARE_PCT) && (b_pct >= MIN_SHARE_PCT),
		     "%s: A %u%%, B %u%%: a thread starved", name, a_pct, b_pct);
}

#endif /* CONFIG_TIMESLICING */

/**
 * @brief Threads of one priority do not starve each other under preemption
 *
 * @details A thread preempted more often than its time slice lasts must
 * still give way to the other threads of its priority. Check it with a
 * higher priority thread preempting them every 2 ms, with the same
 * thread waking up an even higher priority one each time (nested
 * preemption), and with a thread running 6 ms out of every 7.
 *
 * @ingroup kernel_sched_tests
 *
 * @see k_sched_time_slice_set()
 */
ZTEST(threads_scheduling_1cpu, test_slice_preempted_no_starvation)
{
#ifdef CONFIG_TIMESLICING
	int old_prio = k_thread_priority_get(k_current_get());

	/* Outrank every thread of the test to stop them */
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(1));
	k_sched_time_slice_set(SLICE_SIZE_MS, BUSY_PRIO);

	check_no_starvation("preempted", 50, 2, false);
	check_no_starvation("nested", 50, 2, true);
	check_no_starvation("long", 6000, 1, false);

	k_sched_time_slice_set(0, K_PRIO_PREEMPT(0));
	k_thread_priority_set(k_current_get(), old_prio);
#else
	ztest_test_skip();
#endif
}
