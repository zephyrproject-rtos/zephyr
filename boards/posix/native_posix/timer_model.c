/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * This file provides both a model of a simple HW timer and its driver
 *
 * If you want this timer model to slow down the execution to real time
 * set (CONFIG_)NATIVE_POSIX_SLOWDOWN_TO_REAL_TIME
 * You can also control it with the --rt and --no-rt options from command line
 */

#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include "hw_models_top.h"
#include "irq_ctrl.h"
#include "board_soc.h"
#include "zephyr/types.h"
#include "posix_soc_if.h"
#include "misc/util.h"


u64_t hw_timer_timer;

u64_t hw_timer_tick_timer;
u64_t hw_timer_awake_timer;

static u64_t tick_p; /* Period of the ticker */
static s64_t silent_ticks;

static bool real_time =
#if (CONFIG_NATIVE_POSIX_SLOWDOWN_TO_REAL_TIME)
	true;
#else
	false;
#endif

static u64_t Boot_time;
static struct timespec tv;

extern u64_t posix_get_hw_cycle(void);

void hwtimer_set_real_time(bool new_rt)
{
	real_time = new_rt;
}

static void hwtimer_update_timer(void)
{
	hw_timer_timer = min(hw_timer_tick_timer, hw_timer_awake_timer);
}

void hwtimer_init(void)
{
	silent_ticks = 0;
	hw_timer_tick_timer = NEVER;
	hw_timer_awake_timer = NEVER;
	hwtimer_update_timer();
	if (real_time) {
		clock_gettime(CLOCK_MONOTONIC, &tv);
		Boot_time = tv.tv_sec*1e6 + tv.tv_nsec/1000;
	}
}

void hwtimer_cleanup(void)
{

}

/**
 * Enable the HW timer tick interrupts with a period <period> in micoseconds
 */
void hwtimer_enable(u64_t period)
{
	tick_p = period;
	hw_timer_tick_timer = hwm_get_time() + tick_p;
	hwtimer_update_timer();
	hwm_find_next_timer();
}

static void hwtimer_tick_timer_reached(void)
{
	if (real_time) {
		u64_t expected_realtime = Boot_time + hw_timer_tick_timer;

		clock_gettime(CLOCK_MONOTONIC, &tv);
		u64_t actual_real_time = tv.tv_sec*1e6 + tv.tv_nsec/1000;

		int64_t diff = expected_realtime - actual_real_time;

		if (diff > 0) { /* we need to slow down */
			struct timespec requested_time;
			struct timespec remaining;

			requested_time.tv_sec  = diff / 1e6;
			requested_time.tv_nsec = (diff -
						 requested_time.tv_sec*1e6)*1e3;

			nanosleep(&requested_time, &remaining);
		}
	}

	hw_timer_tick_timer += tick_p;
	hwtimer_update_timer();

	if (silent_ticks > 0) {
		silent_ticks -= 1;
	} else {
		hw_irq_ctrl_set_irq(TIMER_TICK_IRQ);
	}
}

static void hwtimer_awake_timer_reached(void)
{
	hw_timer_awake_timer = NEVER;
	hwtimer_update_timer();
	hw_irq_ctrl_set_irq(PHONY_HARD_IRQ);
}

void hwtimer_timer_reached(void)
{
	u64_t Now = hw_timer_timer;

	if (hw_timer_awake_timer == Now) {
		hwtimer_awake_timer_reached();
	}

	if (hw_timer_tick_timer == Now) {
		hwtimer_tick_timer_reached();
	}
}

/**
 * The timer HW will awake the CPU (without an interrupt) at least when <time>
 * comes (it may awake it earlier)
 *
 * If there was a previous request for an earlier time, the old one will prevail
 *
 * This is meant for k_busy_wait() like functionality
 */
void hwtimer_wake_in_time(u64_t time)
{
	if (hw_timer_awake_timer > time) {
		hw_timer_awake_timer = time;
		hwtimer_update_timer();
	}
}

/**
 * The kernel wants to skip the next sys_ticks tick interrupts
 * If sys_ticks == 0, the next interrupt will be raised.
 */
void hwtimer_set_silent_ticks(s64_t sys_ticks)
{
	silent_ticks = sys_ticks;
}

s64_t hwtimer_get_pending_silent_ticks(void)
{
	return silent_ticks;
}
