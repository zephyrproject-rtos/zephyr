/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <device.h>
#include <system_timer.h>
#include <altera_common.h>

#include "altera_avalon_timer_regs.h"
#include "altera_avalon_timer.h"

static u32_t accumulated_cycle_count;

static void timer_irq_handler(void *unused)
{
	ARG_UNUSED(unused);

#ifdef CONFIG_EXECUTION_BENCHMARKING
	extern void read_timer_start_of_tick_handler(void);
	read_timer_start_of_tick_handler();
#endif

	accumulated_cycle_count += sys_clock_hw_cycles_per_tick;

	/* Clear the interrupt */
	alt_handle_irq((void *)TIMER_0_BASE, TIMER_0_IRQ);

	_sys_clock_tick_announce();

#ifdef CONFIG_EXECUTION_BENCHMARKING
	extern void read_timer_end_of_tick_handler(void);
	read_timer_end_of_tick_handler();
#endif
}

int _sys_clock_driver_init(struct device *device)
{
	ARG_UNUSED(device);

	IOWR_ALTERA_AVALON_TIMER_PERIODL(TIMER_0_BASE,
			sys_clock_hw_cycles_per_tick & 0xFFFF);
	IOWR_ALTERA_AVALON_TIMER_PERIODH(TIMER_0_BASE,
			(sys_clock_hw_cycles_per_tick >> 16) & 0xFFFF);

	IRQ_CONNECT(TIMER_0_IRQ, 0, timer_irq_handler, NULL, 0);
	irq_enable(TIMER_0_IRQ);

	alt_avalon_timer_sc_init((void *)TIMER_0_BASE, 0,
			TIMER_0_IRQ, sys_clock_hw_cycles_per_tick);

	return 0;
}

u32_t _timer_cycle_get_32(void)
{
	/* Per the Altera Embedded IP Peripherals guide, you cannot
	 * use a timer instance for both the system clock and timestamps
	 * at the same time.
	 *
	 * Having this function return accumulated_cycle_count + get_snapshot()
	 * does not work reliably. It's possible for the current countdown
	 * to reset to the next interval before the timer interrupt is
	 * delivered (and accumulated cycle count gets updated). The result
	 * is an unlucky call to this function will appear to jump backward
	 * in time.
	 *
	 * To properly obtain timestamps, the CPU must be configured with
	 * a second timer peripheral instance that is configured to
	 * count down from some large initial 64-bit value. This
	 * is currently unimplemented.
	 */
	return accumulated_cycle_count;
}
