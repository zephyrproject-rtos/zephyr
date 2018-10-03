/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Driver for the timer model of the POSIX native_posix board
 * It provides the interfaces required by the kernel and the sanity testcases
 * It also provides a custom k_busy_wait() which can be used with the
 * POSIX arch and InfClock SOC
 */

#include "zephyr/types.h"
#include "irq.h"
#include "device.h"
#include "drivers/system_timer.h"
#include "sys_clock.h"
#include "timer_model.h"
#include "soc.h"
#include "posix_trace.h"

static u64_t tick_period; /* System tick period in number of hw cycles */
static s64_t silent_ticks;

/**
 * Return the current HW cycle counter
 * (number of microseconds since boot in 32bits)
 */
u32_t _timer_cycle_get_32(void)
{
	return hwm_get_time();
}

#ifdef CONFIG_TICKLESS_IDLE

/*
 * Do not raise another ticker interrupt until the sys_ticks'th one
 * e.g. if sys_ticks is 10, do not raise the next 9 ones
 *
 * if sys_ticks is K_FOREVER (or another negative number),
 * we will effectively silence the tick interrupts forever
 */
void _timer_idle_enter(s32_t sys_ticks)
{
	if (silent_ticks > 0) { /* LCOV_EXCL_BR_LINE */
		/* LCOV_EXCL_START */
		posix_print_warning("native timer: Re-entering idle mode with "
				    "%i ticks pending\n",
				    silent_ticks);
		_timer_idle_exit();
		/* LCOV_EXCL_STOP */
	}
	if (sys_ticks < 0) {
		silent_ticks = INT64_MAX;
	} else if (sys_ticks > 0) {
		silent_ticks = sys_ticks - 1;
	} else {
		silent_ticks = 0;
	}
	hwtimer_set_silent_ticks(silent_ticks);
}

/*
 * Exit from idle mode
 *
 * If we have been silent for a number of ticks, announce immediately to the
 * kernel how many silent ticks have passed.
 * If this is called due to the 1st non silent timer interrupt, sp_timer_isr()
 * will be called right away, which will announce that last (non silent) one.
 *
 * Note that we do not assume this function is called before the interrupt is
 * raised (the interrupt can handle it announcing all ticks)
 */
void _timer_idle_exit(void)
{
	silent_ticks -= hwtimer_get_pending_silent_ticks();
	if (silent_ticks > 0) {
		_sys_idle_elapsed_ticks = silent_ticks;
		_sys_clock_tick_announce();
	}
	silent_ticks = 0;
	hwtimer_set_silent_ticks(0);
}
#endif

/**
 * Interrupt handler for the timer interrupt
 * Announce to the kernel that a tick has passed
 */
static void sp_timer_isr(void *arg)
{
	ARG_UNUSED(arg);
	_sys_idle_elapsed_ticks = silent_ticks + 1;
	silent_ticks = 0;
	_sys_clock_tick_announce();
}

/*
 * Enable the hw timer, setting its tick period, and setup its interrupt
 */
int _sys_clock_driver_init(struct device *device)
{
	ARG_UNUSED(device);

	tick_period = 1000000ul / sys_clock_ticks_per_sec;

	hwtimer_enable(tick_period);

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
	u64_t time_end = hwm_get_time() + usec_to_wait;

	while (hwm_get_time() < time_end) {
		/*There may be wakes due to other interrupts*/
		hwtimer_wake_in_time(time_end);
		posix_halt_cpu();
	}
}
#endif
