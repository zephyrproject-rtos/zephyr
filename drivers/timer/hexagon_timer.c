/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys/clock.h>
#include <zephyr/irq.h>
#include <zephyr/init.h>
#include "hexagon_timer.h"
#include <hexagon_vm.h>

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = HEXAGON_TIMER_IRQ;
#endif

/*
 * Tickless system timer for Hexagon under HVM.  Cycle counts are raw
 * hvmt_gettime nanosecond values; the generic core owns tick accounting.
 *
 * hvmt_settimeout matches on equality only: a deadline at or before "now"
 * maps to BIGBANG, which disables the timer instead of firing it.  That
 * makes this a COMPARE_EXACT backend, relying on the core's verify loop
 * to re-arm missed deadlines.
 */

#define TIMER_CORE_BACKEND_COMPARE_EXACT
#define TIMER_CORE_COUNTER_WIDTH 64

/* Read the free-running hypervisor counter (absolute nanosecond-domain time). */
static inline uint64_t timer_driver_cycle_get(void)
{
	return hexagon_vm_timerop(hvmt_gettime, 0, 0);
}

/* Arm an interrupt for the absolute cycle count @p deadline. */
static inline void timer_driver_set_compare(uint64_t deadline)
{
	hexagon_vm_timerop(hvmt_settimeout, 0, deadline);
}

#include "system_timer_generic.h"

/* Timer interrupt handler.  The H2 virtual timer interrupt is consumed by the
 * dispatch path, so the ISR only has to drive the tick accounting.
 */
static void hexagon_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	timer_core_announce();
}

static int sys_clock_driver_init(void)
{
	extern unsigned int z_clock_hw_cycles_per_sec;
	uint64_t freq = hexagon_vm_timerop(hvmt_getfreq, 0, 0);

	__ASSERT(freq != 0, "hvmt_getfreq returned 0: hypervisor timer is unusable");

	/* Publish the counter rate before timer_core_init() derives cycles-per-tick
	 * (CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME reads it back through
	 * sys_clock_hw_cycles_per_sec()).
	 */
	z_clock_hw_cycles_per_sec = (unsigned int)freq;

	arch_irq_connect_dynamic(HEXAGON_TIMER_IRQ, HEXAGON_TIMER_IRQ_PRIORITY,
				 hexagon_timer_isr, NULL, 0);
	irq_enable(HEXAGON_TIMER_IRQ);

	timer_core_init();

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);

void sys_clock_disable(void)
{
	irq_disable(HEXAGON_TIMER_IRQ);
}

void arch_busy_wait(uint32_t usec_to_wait)
{
	if (usec_to_wait == 0U) {
		return;
	}

	uint64_t freq = sys_clock_hw_cycles_per_sec();
	uint64_t start = hexagon_vm_timerop(hvmt_gettime, 0, 0);
	uint64_t wait_cycles = (freq * usec_to_wait) / 1000000ULL;
	uint64_t target = start + wait_cycles;

	while (hexagon_vm_timerop(hvmt_gettime, 0, 0) < target) {
		/* spin */
	}
}
