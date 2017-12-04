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




#include "irq.h"
#include "device.h"
#include "drivers/system_timer.h"

/**
 * Return the current HW cycle counter
 * (number of microseconds since boot in 32bits)
 */
uint32_t _timer_cycle_get_32(void)
{
	return hwm_get_time();
}

#ifdef CONFIG_TICKLESS_IDLE
void _timer_idle_enter(int32_t sys_ticks)
{
	silent_ticks = sys_ticks;
}

void _timer_idle_exit(void)
{
	silent_ticks = 0;
}
#endif

static void sp_timer_isr(void *arg)
{
	ARG_UNUSED(arg);
	_sys_clock_tick_announce();
}

int _sys_clock_driver_init(struct device *device)
{
	ARG_UNUSED(device);

	IRQ_CONNECT(TIMER_TICK_IRQ, 1, sp_timer_isr, 0, 0);
	irq_enable(TIMER_TICK_IRQ);

	return 0;
}


#if defined(CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT)
/**
 * Replacement to the kernel k_busy_wait()
 * Will block this thread (and therefore the whole zephyr) during usec_to_wait
 *
 * Note that interrupts may be received in the meanwhile and that therefore this
 * thread may loose context
 */
void k_busy_wait(u32_t usec_to_wait)
{
	hwtime_t time_end = hwm_get_time() + usec_to_wait;

	while (hwm_get_time() < time_end) {
		/*There may be wakes due to other interrupts*/
		hwtimer_wake_in_time(time_end);
		ps_halt_cpu();
	}
}
#endif
