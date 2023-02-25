/*
 * Copyright (c) 2023 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 * Written by Nicolas Pitre
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/*
 * This test is designed to validate the time slice expiration mechanism
 * and scheduler interaction on SMP systems, although it can be valuable
 * on !SMP systems too. This implies proper sys_clock_announce() invocations
 * by the platform's timer driver on the appropriate CPU at the appropriate
 * time, whether or not the announced tick value is 0, etc.
 */

#define THREADS_PER_CPU 2
#define TIMESLICE_ROUNDS 3

#define NB_THREADS (THREADS_PER_CPU * CONFIG_MP_MAX_NUM_CPUS)
#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

static K_KERNEL_STACK_ARRAY_DEFINE(thread_stacks, NB_THREADS, STACK_SIZE);
static struct k_thread threads[NB_THREADS];

/* native_posix doesn't like k_thread_abort() on never-started threads */
static k_tid_t started_threads[NB_THREADS];

/*
 * This thread busily checks for tick progression until it is preempted
 * due to time slicing. Time slice transitions are detected when a sudden
 * tick increase is larger than 1, at which point the actual slice duration
 * and "time away" duration are validated against the expected duration.
 */
static void thread_fn(void *p1, void *p2, void *p3)
{
	uint64_t curr_tick = sys_clock_tick_get(), last_tick = curr_tick;
	uint32_t curr_cycle = k_cycle_get_32(), last_cycle = curr_cycle;
	uint32_t tick_diff = 0, cycle_diff = 0;
	uint32_t tick_changes = 0, slice_rounds = 0;
	uint32_t thread_id = POINTER_TO_UINT(p1);
	uint32_t expected_start_time = POINTER_TO_UINT(p2);
	uint32_t slice_ticks = POINTER_TO_UINT(p3);

	TC_PRINT("th%d: sr=%d ct=%lld td=%d tc=%d cc=%d cd=%d\n",
		 thread_id, slice_rounds, curr_tick, tick_diff,
		 tick_changes, curr_cycle, cycle_diff);

	/*
	 * Make sure the start delay was respected.
	 * Note: z_add_timeout() always adds 1 to relative timeouts.
	 */
	expected_start_time += 1;
	zassert_true(expected_start_time == (uint32_t)curr_tick,
		     "%d vs %d", expected_start_time, (uint32_t)curr_tick);

	do {
		k_busy_wait(200);

		/* spin until we move to another tick */
		curr_tick = sys_clock_tick_get();
		if (curr_tick == last_tick) {
			continue;
		}

		/*
		 * The tick transition corresponding to the end of a time slice
		 * might have happened while the above sys_clock_tick_get() had
		 * the timeout_lock locked preventing the timer IRQ from being
		 * serviced right away. Because the thread would have been
		 * suspended right before returning, the returned value would
		 * no longer be up to date.
		 *
		 * Another possibility is sys_clock_tick_get() sampling the
		 * hardware timer counter past the new tick transition but
		 * the actual timer match IRQ may take some time to propagate
		 * (especially notable on QEMU).
		 *
		 * To work around those issues, we busy wait for a quarter of
		 * a tick duration and update curr_tick again.
		 */
		k_busy_wait((k_ticks_to_us_ceil32(1) + 3) / 4);
		curr_tick = sys_clock_tick_get();

		tick_diff = curr_tick - last_tick;
		last_tick = curr_tick;
		tick_changes++;

		curr_cycle = k_cycle_get_32();
		cycle_diff = curr_cycle - last_cycle;
		last_cycle = curr_cycle;

		TC_PRINT("th%d: sr=%d ct=%lld td=%d tc=%d cc=%d cd=%d\n",
			 thread_id, slice_rounds, curr_tick, tick_diff,
			 tick_changes, curr_cycle, cycle_diff);

		/* make sure ticks and hardware cycles are in sync */
		zassert_true(k_cyc_to_ticks_near32(cycle_diff) == tick_diff,
			     "%d vs %d", k_cyc_to_ticks_near32(cycle_diff), tick_diff);

		/*
		 * We're expecting a tick step of 1 within a time slice.
		 * Therefore there should not be more than `slice_ticks`
		 * consecutive ticks in that case.
		 *
		 * When the slice expires, the CPU is switched away to spend
		 * ticks in the other threads on this CPU. The time spent
		 * away is: (THREADS_PER_CPU - 1) * slice_ticks.
		 * However, we didn't have the chance to register the end of
		 * the last tick period before being preempted, so one tick
		 * must be added to that number.
		 */
		uint32_t ticks_away = (THREADS_PER_CPU - 1) * slice_ticks + 1;

		if (tick_diff == 1) {
			/* still in the same time slice */
			zassert_true(tick_changes < slice_ticks,
				     "tick_changes=%d slice_ticks=%d",
				     tick_changes, slice_ticks);
		} else if (tick_diff == ticks_away) {
			/* we started a new time slice */
			zassert_true(tick_changes == slice_ticks,
				     "tick_changes=%d slice_ticks=%d",
				     tick_changes, slice_ticks);
			tick_changes = 0;
			slice_rounds++;
		} else {
			zassert_unreachable("tick_diff=%d (is neither 1 nor %d)",
					    tick_diff, ticks_away);
		}
	} while (slice_rounds < TIMESLICE_ROUNDS);

	TC_PRINT("th%d: done\n", thread_id);

	/* fill this last slice not to switch to the other threads too soon */
	k_busy_wait(k_ticks_to_us_ceil32(1 + slice_ticks));
}

/*
 * Start threads to make all CPUs busy. The start delay makes those threads'
 * time slices either all synchronous across all CPUs, or staggered so that
 * none of the slices coincide.
 */
static void create_threads(bool staggered_timeslices)
{
	uint32_t nb_cpus = arch_num_cpus();
	uint32_t i, start_delay_ticks;
	char name[8];

	/*
	 * Beware k_sched_time_slice_set() takes ms not ticks.
	 * In the staggered case, we want slices to be long enough so
	 * that each CPU can expire its slices alone i.e. never at the
	 * same time as another CPU. And a slice needs to be at least
	 * 2 ticks long to be detectable by the code above.
	 */
	uint32_t slice_ms = k_ticks_to_ms_ceil32(2) + k_ticks_to_ms_ceil32(1) * nb_cpus;
	uint32_t slice_ticks = k_ms_to_ticks_ceil32(slice_ms);

	TC_PRINT("creating %d threads per CPU on %d CPUs, with %d ticks per time slice\n",
		 THREADS_PER_CPU, nb_cpus, slice_ticks);

	k_sched_time_slice_set(slice_ms, 0);

	/* synchronize to a tick edge */
	k_msleep(1);
	uint32_t now = sys_clock_tick_get();

	for (i = 0; i < (nb_cpus * THREADS_PER_CPU); i++) {
		start_delay_ticks = 1 + (i / nb_cpus) * slice_ticks;
		start_delay_ticks += (staggered_timeslices) ? (i % nb_cpus) : 0;
		started_threads[i] =
			k_thread_create(&threads[i], thread_stacks[i],
					K_THREAD_STACK_SIZEOF(thread_stacks[i]),
					thread_fn,
					UINT_TO_POINTER(i),
					UINT_TO_POINTER(now + start_delay_ticks),
					UINT_TO_POINTER(slice_ticks),
					1, 0, K_TICKS(start_delay_ticks));
		snprintf(name, sizeof(name), "th%d", i);
		k_thread_name_set(&threads[i], name);
	}
}

static void clean_threads(void)
{
	uint32_t nb_cpus = arch_num_cpus();
	uint32_t i;

	for (i = 0; i < (nb_cpus * THREADS_PER_CPU); i++) {
		k_thread_join(&threads[i], K_FOREVER);
		started_threads[i] = NULL;
	}
}

/* forcefully stop all thread if one of them failed and ended the test */
static void force_cleanup(void *unused)
{
	uint32_t nb_cpus = arch_num_cpus();
	uint32_t i;

	for (i = 0; i < (nb_cpus * THREADS_PER_CPU); i++) {
		if (started_threads[i] != NULL) {
			k_thread_abort(&threads[i]);
			started_threads[i] = NULL;
		}
	}
}

/*
 * Synchronous test scenario:
 *
 *         CPU0         CPU1         CPU2         CPU3
 * t1  +----------+ +----------+ +----------+ +----------+
 * t2  |          | |          | |          | |          |
 * t3  |          | |          | |          | |          |
 * t3  | thread 0 | | thread 1 | | thread 2 | | thread 3 |
 * t4  |          | |          | |          | |          |
 * t5  |          | |          | |          | |          |
 * t6  +----------+ +----------+ +----------+ +----------+
 * t7  |          | |          | |          | |          |
 * t8  |          | |          | |          | |          |
 * t9  | thread 4 | | thread 5 | | thread 6 | | thread 7 |
 * t10 |          | |          | |          | |          |
 * t11 |          | |          | |          | |          |
 * t12 +----------+ +----------+ +----------+ +----------+
 * t13 |          | |          | |          | |          |
 * t14 |          | |          | |          | |          |
 * t15 | thread 0 | | thread 1 | | thread 2 | | thread 3 |
 * t16 |          | |          | |          | |          |
 * ... .          . .          . .          . .          .
 *
 * Here the time slice expiries happen synchronously on all CPUs.
 * The second wave of threads is made runnable through timeouts which
 * coincide with the end of the first wave's time slices. The scheduler
 * must pick the second set of threads and not the still-runnable first
 * set which might or might not have been requeued faster due to the inherent
 * race between the one CPU that is processing all global timeouts and the
 * others which only have their own time slice expiration to process.
 */
ZTEST(timeslicing, test_timeslicing_synchronous)
{
	create_threads(false);
	clean_threads();
}

/*
 * Staggered test scenario:
 *
 *         CPU0         CPU1         CPU2         CPU3
 * t1  +----------+ .          . .          . .          .
 * t2  |          | +----------+ .          . .          .
 * t3  |          | |          | +----------+ .          .
 * t4  | thread 0 | |          | |          | +----------+
 * t5  |          | | thread 1 | |          | |          |
 * t6  |          | |          | | thread 2 | |          |
 * t7  +----------+ |          | |          | | thread 3 |
 * t8  |          | +----------+ |          | |          |
 * t9  |          | |          | +----------+ |          |
 * t10 | thread 4 | |          | |          | +----------+
 * t11 |          | | thread 5 | |          | |          |
 * t12 |          | |          | | thread 6 | |          |
 * t13 +----------+ |          | |          | | thread 7 |
 * t14 |          | +----------+ |          | |          |
 * t15 |          | |          | +----------+ |          |
 * t16 | thread 0 | |          | |          | +----------+
 * t17 |          | | thread 1 | |          | |          |
 * t18 |          | |          | | thread 2 | |          |
 * t19 +----------+ |          | |          | | thread 3 |
 * t20 |          | +----------+ |          | |          |
 * t21 |          | |          | +----------+ |          |
 * t22 | thread 4 | |          | |          | +----------+
 * ... .          . .          . .          . .          .
 *
 * Here the time slice expiries happen independently on each CPU.
 * Like in the synchronous case, the second wave of threads is made
 * runnable through a timeout which expiration coincide with the end
 * of a time slice. However some CPUs will see timeouts that don't match
 * their corresponding time slice and they must be able to rearm their own
 * timer accordingly.
 *
 * Also, scheduler fairness requires that CPU1 picks up thread 5 that is
 * made runnable at t8 and not thread 0 which was still runnable and
 * re-queued at t7, etc.
 */
ZTEST(timeslicing, test_timeslicing_staggered)
{
	if (!IS_ENABLED(CONFIG_SMP) || !(arch_num_cpus() > 1)) {
		/* no point without multiple CPUs */
		ztest_test_skip();
	}
	create_threads(true);
	clean_threads();
}

static void *display_params(void)
{
	TC_PRINT("hardware clock frequency: %d cycles/sec\n", sys_clock_hw_cycles_per_sec());
	TC_PRINT("system tick frequency:    %d ticks/sec\n", CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	TC_PRINT("system tick duration:     %d cycles/tick\n",
		 sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	PRINT_LINE;
	return NULL;
}

ZTEST_SUITE(timeslicing, NULL, display_params, NULL, force_cleanup, NULL);
