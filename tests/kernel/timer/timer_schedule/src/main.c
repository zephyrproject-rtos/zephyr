/*
 * Copyright (c) 2018 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* T1 is a periodic timer with a 100 ms interval.  When it fires, it
 * schedules T2 as a one-shot timer due in 50 ms.
 *
 * To produce the theoretical mis-handling we need to construct a
 * situation where tick processing is delayed such that when T1 fires
 * there is at least one tick remaining that is used to prematurely
 * reduce the delay of the T2 that gets scheduled when T1 is
 * processed.
 *
 * We do this by having the main loop wait until T2 fires the 3d time,
 * indicated by a semaphore.  When it can take the semaphore it locks
 * interrupt handling for T1's period minus half of T2's timeout,
 * which means the next T1 will fire half T2's timeout late, and the
 * delay for T2 should be reduced by half.  It then waits for T2 to
 * run.  The delay for T2 will be shorter than in the non-blocking
 * case if the mis-handling occurs.
 */

#include <zephyr.h>
#include <ztest.h>

#define T1_PERIOD		1000	/* [ms] */
#define	T2_TIMEOUT		50	/* [ms] */
#define CYC_PER_TICK (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define T2_TIMEOUT_TICK (K_MSEC(T2_TIMEOUT)			\
			 * CONFIG_SYS_CLOCK_TICKS_PER_SEC	\
			 / MSEC_PER_SEC)

static struct k_timer timer1;
static struct k_timer timer2;
static struct k_timer sync_timer;
static struct k_sem semaphore;

static struct state {
	unsigned int run;

	/** k_uptime_get_32() when T1 last expired */
	u32_t t1_exec_ut;
	/** k_cycle_get_32() when T1 last expired */
	u32_t t1_exec_ct;

	/** Difference in k_cycle_get() between most recent two T1 expires */
	s32_t t1_delay_ct;
	/** Difference in k_uptime_get() between most recent two T1 expires */
	s32_t t1_delay_ut;
	/** Difference in k_cycle_get() between T2 start and callback */
	s32_t t2_delay_ct;
	/** Difference in k_uptime_get() between T2 start and callback */
	s32_t t2_delay_ut;
	/** Tick-corrected measured realtime between T2 start and callback */
	s32_t t2_delay_us;
} state;

static void timer1_expire(struct k_timer *timer)
{
	state.t1_exec_ut = k_uptime_get_32();
	state.t1_exec_ct = k_cycle_get_32();
	k_timer_start(&timer2, K_MSEC(T2_TIMEOUT), 0);
}

static void timer2_expire(struct k_timer *timer)
{
	static u32_t t1_prev_ct;
	static u32_t t1_prev_ut;
	u32_t now_ct = k_cycle_get_32();
	u32_t now_ut = k_uptime_get_32();

	state.t1_delay_ct = state.t1_exec_ct - t1_prev_ct;
	state.t1_delay_ut = state.t1_exec_ut - t1_prev_ut;
	state.t2_delay_ct = now_ct - state.t1_exec_ct;
	state.t2_delay_ut = now_ut - state.t1_exec_ut;

	if (USEC_PER_SEC < sys_clock_hw_cycles_per_sec()) {
		u32_t div = sys_clock_hw_cycles_per_sec()
			/ USEC_PER_SEC;
		state.t2_delay_us = state.t2_delay_ct / div;
	} else {
		state.t2_delay_us = state.t2_delay_ct
			* (u64_t)USEC_PER_SEC
			/ sys_clock_hw_cycles_per_sec();
	}
	t1_prev_ct = state.t1_exec_ct;
	t1_prev_ut = state.t1_exec_ut;

	k_sem_give(&semaphore);
}


static void test_schedule(void)
{
	k_timer_init(&timer1, timer1_expire, NULL);
	k_timer_init(&timer2, timer2_expire, NULL);
	k_sem_init(&semaphore, 0, 1);

	TC_PRINT("T1 interval %u ms, T2 timeout %u ms, %u sysclock per tick\n",
		 T1_PERIOD, T2_TIMEOUT, sys_clock_hw_cycles_per_sec());

	k_timer_init(&sync_timer, NULL, NULL);
	k_timer_start(&sync_timer, 0, 1);
	k_timer_status_sync(&sync_timer);
	k_timer_stop(&sync_timer);

	k_timer_start(&timer1, K_MSEC(T1_PERIOD), K_MSEC(T1_PERIOD));

	while (state.run < 6) {
		static s32_t t2_lower_tick = T2_TIMEOUT_TICK - 1;
		static s32_t t2_upper_tick = T2_TIMEOUT_TICK + 1;
		s32_t t2_delay_tick;

		k_sem_take(&semaphore, K_FOREVER);

		if (state.run > 0) {
			t2_delay_tick = state.t2_delay_us
				* (u64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC
				/ USEC_PER_SEC;

			TC_PRINT("Run %u timer1 last %u interval %d/%d; "
				 " timer2 delay %d/%d = %d us = %d tick\n",
				 state.run, state.t1_exec_ut,
				 state.t1_delay_ct, state.t1_delay_ut,
				 state.t2_delay_ct, state.t2_delay_ut,
				 state.t2_delay_us, t2_delay_tick);

			zassert_true(t2_delay_tick >= t2_lower_tick,
				     "expected delay %d >= %d",
				     t2_delay_tick, t2_lower_tick);
			zassert_true(t2_delay_tick <= t2_upper_tick,
				     "expected delay %d <= %d",
				     t2_delay_tick, t2_upper_tick);
		}

		if (state.run == 3) {
			unsigned int key;

			TC_PRINT("blocking\n");

			key = irq_lock();
			k_busy_wait(K_MSEC(T1_PERIOD - T2_TIMEOUT / 2)
				    * USEC_PER_MSEC);
			irq_unlock(key);
		}

		++state.run;
	}

	k_timer_stop(&timer1);
}

void test_main(void)
{
	ztest_test_suite(timer_fn, ztest_unit_test(test_schedule));
	ztest_run_test_suite(timer_fn);
}
