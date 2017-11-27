/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * This file provides both a model of a HW timer and its driver
 */

#include <stdint.h>
#include "hw_models_top.h"
#include "irq_ctrl.h"

/*set this to 1 to run in real time, to 0 to run as fast as possible*/
#define CONFIG_ARCH_POSIX_RUN_AT_REAL_TIME 1


hwtime_t HWTimer_timer;

static hwtime_t tick_p = 10000; /*period of the ticker*/
static unsigned int silent_ticks;

#if (CONFIG_ARCH_POSIX_RUN_AT_REAL_TIME)
#include <time.h>

static hwtime_t Boot_time;
static struct timespec tv;
#endif

void hwtimer_init(void)
{
	silent_ticks = 0;
	HWTimer_timer = tick_p;
#if (CONFIG_ARCH_POSIX_RUN_AT_REAL_TIME)
	clock_gettime(CLOCK_MONOTONIC, &tv);
	Boot_time = tv.tv_sec*1e6 + tv.tv_nsec/1000;
#endif
}


void hwtimer_cleanup(void)
{

}


void hwtimer_timer_reached(void)
{
#if (CONFIG_ARCH_POSIX_RUN_AT_REAL_TIME)
	hwtime_t expected_realtime = Boot_time + HWTimer_timer;

	clock_gettime(CLOCK_MONOTONIC, &tv);
	hwtime_t actual_real_time = tv.tv_sec*1e6 + tv.tv_nsec/1000;

	int64_t diff = expected_realtime - actual_real_time;

	if (diff > 0) { /*we need to slow down*/
		struct timespec requested_time;
		struct timespec remaining;

		requested_time.tv_sec = diff / 1e6;
		requested_time.tv_nsec =
				(diff - requested_time.tv_sec*1e6)*1e3;

		int s = nanosleep(&requested_time, &remaining);

		if (s == -1) {
			ps_print_trace(
					"Interrupted or error\n");
		}
	}
#endif

	HWTimer_timer += tick_p;

	if (silent_ticks > 0) {
		silent_ticks -= 1;
	} else {
		hw_irq_controller(TIMER);
	}
}





/**
 * Return the current HW cycle counter
 * (number of microseconds since boot in 32bits)
 */
uint32_t _timer_cycle_get_32(void)
{
	return hwm_get_time();
}


void _timer_idle_enter(int32_t sys_ticks)
{
	silent_ticks = sys_ticks;
}

void _timer_idle_exit(void)
{
	silent_ticks = 0;
}
