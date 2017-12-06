/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * This file provides both a model of a simple HW timer and its driver
 *
 * If you want this timer model to slow down the execution to real time
 * set (CONFIG_)SIMPLE_PROCESS_SLOWDOWN_TO_REAL_TIME
 */

#include <stdint.h>
#include "hw_models_top.h"
#include "irq_ctrl.h"
#include "board_soc.h"
#include "zephyr/types.h"
#include "posix_soc_if.h"
#include "misc/util.h"


hwtime_t HWTimer_timer;

hwtime_t HWTimer_tick_timer;
hwtime_t HWTimer_awake_timer;

static hwtime_t tick_p = 10000; /*period of the ticker*/
static unsigned int silent_ticks;

#if (CONFIG_SIMPLE_PROCESS_SLOWDOWN_TO_REAL_TIME)
#include <time.h>

static hwtime_t Boot_time;
static struct timespec tv;
#endif

static void hwtimer_update_timer(void)
{
	HWTimer_timer = min(HWTimer_tick_timer, HWTimer_awake_timer);
}


void hwtimer_init(void)
{
	silent_ticks = 0;
	HWTimer_tick_timer = tick_p;
	HWTimer_awake_timer = NEVER;
	hwtimer_update_timer();
#if (CONFIG_SIMPLE_PROCESS_SLOWDOWN_TO_REAL_TIME)
	clock_gettime(CLOCK_MONOTONIC, &tv);
	Boot_time = tv.tv_sec*1e6 + tv.tv_nsec/1000;
#endif
}


void hwtimer_cleanup(void)
{

}

static void hwtimer_tick_timer_reached(void)
{
#if (CONFIG_SIMPLE_PROCESS_SLOWDOWN_TO_REAL_TIME)
	hwtime_t expected_realtime = Boot_time + HWTimer_tick_timer;

	clock_gettime(CLOCK_MONOTONIC, &tv);
	hwtime_t actual_real_time = tv.tv_sec*1e6 + tv.tv_nsec/1000;

	int64_t diff = expected_realtime - actual_real_time;

	if (diff > 0) { /*we need to slow down*/
		struct timespec requested_time;
		struct timespec remaining;

		requested_time.tv_sec  = diff / 1e6;
		requested_time.tv_nsec = (diff - requested_time.tv_sec*1e6)*1e3;

		int s = nanosleep(&requested_time, &remaining);

		if (s == -1) {
			ps_print_trace("Interrupted or error\n");
		}
	}
#endif

	HWTimer_tick_timer += tick_p;
	hwtimer_update_timer();

	if (silent_ticks > 0) {
		silent_ticks -= 1;
	} else {
		hw_irq_ctrl_set_irq(TIMER_TICK_IRQ);
	}
}


static void hwtimer_awake_timer_reached(void)
{
	HWTimer_awake_timer = NEVER;
	hwtimer_update_timer();
	hw_irq_ctrl_set_irq(PHONY_HARD_IRQ);
}

void hwtimer_timer_reached(void)
{
	hwtime_t Now = HWTimer_timer;

	if (HWTimer_awake_timer == Now) {
		hwtimer_awake_timer_reached();
	}

	if (HWTimer_tick_timer == Now) {
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
void hwtimer_wake_in_time(hwtime_t time)
{
	if (HWTimer_awake_timer > time) {
		HWTimer_awake_timer = time;
		hwtimer_update_timer();
	}
}


void hwtimer_set_silent_ticks(int sys_ticks)
{
	silent_ticks = sys_ticks;
}


