/*
 * Copyright (c) 2017-2019 Oticon A/S
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
#include <zephyr/irq.h>
#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include "timer_model.h"
#include "soc.h"
#include <zephyr/arch/posix/posix_trace.h>

static uint64_t tick_period; /* System tick period in microseconds */
/* Time (microseconds since boot) of the last timer tick interrupt */
static uint64_t last_tick_time;

/**
 * Return the current HW cycle counter
 * (number of microseconds since boot in 32bits)
 */
uint32_t sys_clock_cycle_get_32(void)
{
	return hwm_get_time();
}

uint64_t sys_clock_cycle_get_64(void)
{
	return hwm_get_time();
}

/**
 * Interrupt handler for the timer interrupt
 * Announce to the kernel that a number of ticks have passed
 */
static void np_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	uint64_t now = hwm_get_time();
	int32_t elapsed_ticks = (now - last_tick_time)/tick_period;

	last_tick_time += elapsed_ticks*tick_period;
	sys_clock_announce(elapsed_ticks);
}

/**
 * This function exists only to enable tests to call into the timer ISR
 */
void np_timer_isr_test_hook(const void *arg)
{
	np_timer_isr(NULL);
}

/**
 * @brief Set system clock timeout
 *
 * Informs the system clock driver that the next needed call to
 * sys_clock_announce() will not be until the specified number of ticks
 * from the the current time have elapsed.
 *
 * See system_timer.h for more information
 *
 * @param ticks Timeout in tick units
 * @param idle Hint to the driver that the system is about to enter
 *        the idle state immediately after setting the timeout
 */
void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#if defined(CONFIG_TICKLESS_KERNEL)
	uint64_t silent_ticks;

	/* Note that we treat INT_MAX literally as anyhow the maximum amount of
	 * ticks we can report with sys_clock_announce() is INT_MAX
	 */
	if (ticks == K_TICKS_FOREVER) {
		silent_ticks = INT64_MAX;
	} else if (ticks > 0) {
		silent_ticks = ticks - 1;
	} else {
		silent_ticks = 0;
	}
	hwtimer_set_silent_ticks(silent_ticks);
#endif
}

/**
 * @brief Ticks elapsed since last sys_clock_announce() call
 *
 * Queries the clock driver for the current time elapsed since the
 * last call to sys_clock_announce() was made.  The kernel will call
 * this with appropriate locking, the driver needs only provide an
 * instantaneous answer.
 */
uint32_t sys_clock_elapsed(void)
{
	return (hwm_get_time() - last_tick_time)/tick_period;
}

/**
 * @brief Stop announcing sys ticks into the kernel
 *
 * Disable the system ticks generation
 */
void sys_clock_disable(void)
{
	irq_disable(TIMER_TICK_IRQ);
	hwtimer_set_silent_ticks(INT64_MAX);
}

/**
 * @brief Initialize system timer driver
 *
 * Enable the hw timer, setting its tick period, and setup its interrupt
 */
static int sys_clock_driver_init(void)
{

	tick_period = 1000000ul / CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	last_tick_time = hwm_get_time();
	hwtimer_enable(tick_period);

	IRQ_CONNECT(TIMER_TICK_IRQ, 1, np_timer_isr, 0, 0);
	irq_enable(TIMER_TICK_IRQ);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_1,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
