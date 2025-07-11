/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/irq.h>
#include <zephyr/init.h>
#include "hexagon_timer.h"
#include <hexagon_vm.h>

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = HEXAGON_TIMER_IRQ;
#endif

static struct hexagon_timer_data timer_data;
static bool timer_initialized;

/*
 * System timer for Hexagon under HVM.
 *
 * Programs the HVM timer via vmtimerop(hvmt_deltatimeout) for hardware-
 * backed tick generation.  The hvmt_deltatimeout parameter is in
 * nanoseconds; hvmt_getfreq returns a nanosecond-domain frequency
 * (TIMERHW_NSEC_FREQ = real_freq * ns_per_tick ~= 998.4 MHz).
 */

/* Program the next timer tick via vmtimerop(hvmt_deltatimeout) */
static void hexagon_timer_program_next(void)
{
	hexagon_vm_timerop(hvmt_deltatimeout, 0,
			   (uint64_t)timer_data.cycles_per_tick);
}

/* Timer interrupt handler */
static void hexagon_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* Accumulate cycles (software counter) */
	timer_data.accumulated_cycles += timer_data.cycles_per_tick;

	/* Program next tick (works on real hardware; harmless on QEMU) */
	hexagon_timer_program_next();

	/* Announce timer tick to kernel */
	sys_clock_announce(1);
}

/* Initialize system timer */
static int sys_clock_driver_init(void)
{
	uint64_t freq;

	/* Query the HVM timer frequency (returns nanosecond-domain freq) */
	freq = hexagon_vm_timerop(hvmt_getfreq, 0, 0);
	if (freq == 0) {
		/* Fallback to configured frequency */
		freq = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
	}

	timer_data.cycles_per_tick = (uint32_t)(freq / CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	timer_data.accumulated_cycles = 0;

	/* Connect and enable the timer IRQ */
	arch_irq_connect_dynamic(HEXAGON_TIMER_IRQ, HEXAGON_TIMER_IRQ_PRIORITY,
				 hexagon_timer_isr, NULL, 0);
	irq_enable(HEXAGON_TIMER_IRQ);

	/* Program the first tick */
	hexagon_timer_program_next();

	timer_initialized = true;
	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);

/*
 * Cycle counter -- returns the software accumulated_cycles counter.
 * Only advances on timer ticks, so it is coarse-grained but
 * monotonically increasing.
 */
uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)timer_data.accumulated_cycles;
}

uint64_t sys_clock_cycle_get_64(void)
{
	return timer_data.accumulated_cycles;
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(ticks);
	ARG_UNUSED(idle);

	/*
	 * Timer runs at a fixed periodic rate for simplicity.
	 * A tickless driver could reprogram the timeout here.
	 */
}

uint32_t sys_clock_elapsed(void)
{
	return 0;
}

void sys_clock_disable(void)
{
	if (timer_initialized) {
		irq_disable(HEXAGON_TIMER_IRQ);
	}
}

static inline void hexagon_pause(void)
{
	__asm__ volatile("pause(#255)" :::);
}

/*
 * Busy wait -- simple delay loop using the pause instruction.
 * Each iteration takes approximately 1 us on QEMU.
 */
void arch_busy_wait(uint32_t usec_to_wait)
{
	uint32_t usec_elapsed = 0;

	while (usec_elapsed < usec_to_wait) {
		hexagon_pause();
		usec_elapsed++;
	}
}
