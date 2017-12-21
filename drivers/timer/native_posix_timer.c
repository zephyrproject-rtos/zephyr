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
#include "timer_model.h"
#include "soc.h"

/**
 * Return the current HW cycle counter
 * (number of microseconds since boot in 32bits)
 */
u32_t _timer_cycle_get_32(void)
{
	return hwm_get_time();
}

#ifdef CONFIG_TICKLESS_IDLE
void _timer_idle_enter(int32_t sys_ticks)
{
	hwtimer_set_silent_ticks(sys_ticks);
}

void _timer_idle_exit(void)
{
	hwtimer_set_silent_ticks(0);
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
	u64_t time_end = hwm_get_time() + usec_to_wait;

	while (hwm_get_time() < time_end) {
		/*There may be wakes due to other interrupts*/
		hwtimer_wake_in_time(time_end);
		posix_halt_cpu();
	}
}
#endif
