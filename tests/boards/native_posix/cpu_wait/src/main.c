/*
 * Copyright (c) 2020 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <stdlib.h>
#include <stdio.h>
#include <zephyr/irq.h>
#include "board_soc.h"

/**
 * @brief Basic test of the POSIX arch k_busy_wait() and cpu_hold()
 * functions
 *
 * In this basic case, only one k_busy_wait() or posix_cpu_hold executes
 * at a time
 */
ZTEST(native_cpu_hold, test_cpu_hold_basic)
{
	uint32_t wait_times[] =  {1, 30, 0, 121, 10000};
	uint64_t time2, time1 = posix_get_hw_cycle();

	for (int i = 0; i < ARRAY_SIZE(wait_times); i++) {
		k_busy_wait(wait_times[i]);
		time2 = posix_get_hw_cycle();
		zassert_true(time2 - time1 == wait_times[i],
				"k_busy_wait failed "
				PRIu64"-"PRIu64"!="PRIu32"\n",
				time2, time1, wait_times[i]);
		time1 = time2;
	}

	for (int i = 0; i < ARRAY_SIZE(wait_times); i++) {
		posix_cpu_hold(wait_times[i]);
		time2 = posix_get_hw_cycle();
		zassert_true(time2 - time1 == wait_times[i],
				"posix_cpu_hold failed "
				PRIu64"-"PRIu64"!="PRIu32"\n",
				time2, time1, wait_times[i]);
		time1 = time2;
	}
}

#define WASTED_TIME 1000 /* 1ms */
#define THREAD_PRIO 0
#define THREAD_DELAY 0
/* Note: the duration of WASTED_TIME and thread priorities should not be changed
 * without thought, as they do matter for the test
 */

#define ONE_TICK_TIME (1000000ul / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define TWO_TICKS_TIME (2*1000000ul / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define ONE_AND_HALF_TICKS  (ONE_TICK_TIME + (ONE_TICK_TIME>>1))
#define TWO_AND_HALF_TICKS  ((ONE_TICK_TIME<<1) + (ONE_TICK_TIME>>1))

#if (WASTED_TIME > ONE_TICK_TIME/2)
#error "This test will not work with this system tick period"
#endif

static void thread_entry(void *p1, void *p2, void *p3);

K_THREAD_DEFINE(TIME_WASTER, CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE,
		thread_entry, 0, 0, 0,
		THREAD_PRIO, 0, THREAD_DELAY);
K_SEM_DEFINE(start_sema, 0, 1);
K_SEM_DEFINE(end_sema, 0, 1);

/**
 * Thread meant to come up and waste time during the k_busy_wait() and
 * posix_cpu_hold() calls of test_cpu_hold_with_another_thread()
 */
static void thread_entry(void *p1, void *p2, void *p3)
{
	int i;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (i = 0; i < 4; i++) {
		/* Synchronize start of subtest with test thread */
		k_sem_take(&start_sema, K_FOREVER);
		/* Sleep until next tick
		 * This sleep will take 2 ticks as the semaphore will
		 * be given just after the previous tick boundary
		 */
		k_sleep(Z_TIMEOUT_TICKS(1));
		/* Waste time */
		k_busy_wait(WASTED_TIME);
		/* Synchronize end of subtest with test thread */
		k_sem_give(&end_sema);
	}
}

/**
 * @brief Test the POSIX arch k_busy_wait and cpu_hold while another thread
 * takes time during this test thread waits
 *
 * Note: This test relies on the exact timing of the ticks.
 * For native_posix it works, with a tick of 10ms. In general this test will
 * probably give problems if the tick time is not a relatively even number
 * of microseconds
 */
ZTEST(native_cpu_hold, test_cpu_hold_with_another_thread)
{
	uint64_t time2, time1;

	/* k_busy_wait part: */

	k_sleep(Z_TIMEOUT_TICKS(1)); /* Wait until tick boundary */
	k_sem_give(&start_sema);

	time1 = posix_get_hw_cycle();
	k_busy_wait(TWO_TICKS_TIME + 1);
	/* The thread should have switched in and have used
	 * WASTED_TIME us (1ms) right after 2*one_tick_time
	 * As that is longer than 2 ticks + 1us, the total
	 * should be 2 ticks + WASTED_TIME
	 */
	time2 = posix_get_hw_cycle();

	zassert_true(time2 - time1 == TWO_TICKS_TIME + WASTED_TIME,
			"k_busy_wait failed "
			PRIu64"-"PRIu64"!="PRIu32"\n",
			time2, time1, TWO_TICKS_TIME + WASTED_TIME);

	k_sem_take(&end_sema, K_FOREVER);

	k_sleep(Z_TIMEOUT_TICKS(1)); /* Wait until tick boundary */
	k_sem_give(&start_sema);

	time1 = posix_get_hw_cycle();
	k_busy_wait(TWO_AND_HALF_TICKS);
	/* The thread should have used WASTED_TIME us (1ms) after
	 * 2*one_tick_time, but as that is lower than 2.5 ticks, in
	 * total the wait should be 2.5 ticks
	 */
	time2 = posix_get_hw_cycle();

	zassert_true(time2 - time1 == TWO_AND_HALF_TICKS,
			"k_busy_wait failed "
			PRIu64"-"PRIu64"!="PRIu32"\n",
			time2, time1, TWO_AND_HALF_TICKS);

	k_sem_take(&end_sema, K_FOREVER);

	/* CPU hold part: */

	k_sleep(Z_TIMEOUT_TICKS(1)); /* Wait until tick boundary */
	k_sem_give(&start_sema);

	time1 = posix_get_hw_cycle();
	posix_cpu_hold(TWO_TICKS_TIME + 1);
	/* The thread should have used WASTED_TIME us after 2*one_tick_time,
	 * so the total should be 2 ticks + WASTED_TIME + 1.
	 * That is we spend 2 ticks + 1 us in this context as requested.
	 */
	time2 = posix_get_hw_cycle();

	zassert_true(time2 - time1 == TWO_TICKS_TIME + WASTED_TIME + 1,
			"k_busy_wait failed "
			PRIu64"-"PRIu64"!="PRIu32"\n",
			time2, time1, TWO_TICKS_TIME + WASTED_TIME + 1);

	k_sem_take(&end_sema, K_FOREVER);

	k_sleep(Z_TIMEOUT_TICKS(1)); /* Wait until tick boundary */
	k_sem_give(&start_sema);

	time1 = posix_get_hw_cycle();
	posix_cpu_hold(TWO_AND_HALF_TICKS);
	/* The thread should have used WASTED_TIME us after 2*one_tick_time,
	 * so the total wait should be 2.5 ticks + WASTED_TIME.
	 * That is 2.5 ticks in this context, and WASTED_TIME in the other
	 * thread context
	 */

	time2 = posix_get_hw_cycle();

	zassert_true(time2 - time1 == TWO_AND_HALF_TICKS + WASTED_TIME,
			"k_busy_wait failed "
			PRIu64"-"PRIu64"!="PRIu32"\n",
			time2, time1, TWO_AND_HALF_TICKS + WASTED_TIME);

	k_sem_take(&end_sema, K_FOREVER);
}

/**
 * Replacement system tick timer interrupt handler which wastes time
 * before calling the real one
 */
static void np_timer_isr_test_replacement(const void *arg)
{
	ARG_UNUSED(arg);

	k_busy_wait(WASTED_TIME);

	void np_timer_isr_test_hook(const void *arg);
	np_timer_isr_test_hook(NULL);
}

/**
 * @brief Test posix arch k_busy_wait and cpu_hold with interrupts that take
 * time.
 * The test is timed so that interrupts arrive during the wait times.
 *
 * The kernel is configured as NOT-tickless, and the default tick period is
 * 10ms
 */
ZTEST(native_cpu_hold, test_cpu_hold_with_interrupts)
{
#if defined(CONFIG_BOARD_NATIVE_POSIX)
	/* So far we only have a test for native_posix.
	 * As the test hooks into an interrupt to cause an extra delay
	 * this is very platform specific
	 */
	uint64_t time2, time1;

	k_sleep(Z_TIMEOUT_TICKS(1)); /* Wait until a tick boundary */

	IRQ_CONNECT(TIMER_TICK_IRQ, 1, np_timer_isr_test_replacement, 0, 0);

	time1 = posix_get_hw_cycle();
	k_busy_wait(ONE_TICK_TIME + 1);
	/* Just after ONE_TICK_TIME (10ms) the timer interrupt has come,
	 * causing a delay of WASTED_TIME (1ms), so the k_busy_wait()
	 * returns immediately as it was waiting for 10.001 ms
	 */
	time2 = posix_get_hw_cycle();

	zassert_true(time2 - time1 == ONE_TICK_TIME + WASTED_TIME,
			"k_busy_wait failed "
			PRIu64"-"PRIu64"!="PRIu32"\n",
			time2, time1, ONE_TICK_TIME);


	k_sleep(Z_TIMEOUT_TICKS(1)); /* Wait until tick boundary */

	time1 = posix_get_hw_cycle();
	k_busy_wait(ONE_AND_HALF_TICKS);
	/* Just after ONE_TICK_TIME (10ms) the timer interrupt has come,
	 * causing a delay of WASTED_TIME (1ms), after that, the k_busy_wait()
	 * continues until 15ms
	 */
	time2 = posix_get_hw_cycle();

	zassert_true(time2 - time1 == ONE_AND_HALF_TICKS,
			"k_busy_wait failed "
			PRIu64"-"PRIu64"!="PRIu32"\n",
			time2, time1, ONE_TICK_TIME);



	k_sleep(Z_TIMEOUT_TICKS(1)); /* Wait until tick boundary */

	time1 = posix_get_hw_cycle();
	posix_cpu_hold(ONE_TICK_TIME + 1);
	/* Just after ONE_TICK_TIME (10ms) the timer interrupt has come,
	 * causing a delay of WASTED_TIME (1ms), but posix_cpu_hold continues
	 * until it spends 10.001 ms in this context. That is 11.001ms in total
	 */
	time2 = posix_get_hw_cycle();

	zassert_true(time2 - time1 == ONE_TICK_TIME + 1 + WASTED_TIME,
			"k_busy_wait failed "
			PRIu64"-"PRIu64"!="PRIu32"\n",
			time2, time1, ONE_TICK_TIME);


	k_sleep(Z_TIMEOUT_TICKS(1)); /* Wait until tick boundary */

	time1 = posix_get_hw_cycle();
	posix_cpu_hold(ONE_AND_HALF_TICKS);
	/* Just after ONE_TICK_TIME (10ms) the timer interrupt has come,
	 * causing a delay of WASTED_TIME (1ms), but posix_cpu_hold continues
	 * until it spends 15ms in this context. That is 16ms in total
	 */
	time2 = posix_get_hw_cycle();

	zassert_true(time2 - time1 == ONE_AND_HALF_TICKS + WASTED_TIME,
			"k_busy_wait failed "
			PRIu64"-"PRIu64"!="PRIu32"\n",
			time2, time1, ONE_TICK_TIME);

#endif /* defined(CONFIG_BOARD_NATIVE_POSIX) */
}

ZTEST_SUITE(native_cpu_hold, NULL, NULL, NULL, NULL, NULL);
