/*
 * Copyright (c) 2020, 2021 Antony Pavlov <antonynpavlov@gmail.com>
 * Copyright (c) 2021 Remy Luisant <remy@luisant.ca>
 *
 * Based on riscv_machine_timer.c and xtensa_sys_timer.c
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>
#include <soc.h>
#include <mips/mipsregs.h>

#define MIN_DELAY 1000

static ALWAYS_INLINE void set_cp0_compare(uint32_t time)
{
	_mips_write_32bit_c0_register(CP0_COMPARE, time);
}

static ALWAYS_INLINE uint32_t get_cp0_count(void)
{
	return _mips_read_32bit_c0_register(CP0_COUNT);
}

/*
 * A free-running 32-bit CP0 Count with an equality-match Compare register: a
 * COMPARE backend on a 32-bit counter (TIMER_CORE_CYCLES_WIDTH defaults to the native
 * width, so the core masks deltas at the 2^32 wrap). Because Compare fires only
 * on Count == Compare, an already-past target is missed until Count wraps, so
 * timer_driver_set_compare() bumps the target until it is at least MIN_DELAY ahead,
 * satisfying the core's "must not miss a past deadline" contract. Writing
 * Compare also clears the pending interrupt.
 */
#define TIMER_CORE_BACKEND_COMPARE

static inline uint64_t timer_driver_cycle_get(void)
{
	return get_cp0_count();
}

static inline void timer_driver_set_compare(uint64_t cycles)
{
	uint32_t next = (uint32_t)cycles;

	set_cp0_compare(next);

	uint32_t now = get_cp0_count();

	while ((int32_t)(next - now) < (int32_t)MIN_DELAY) {
		next = now + MIN_DELAY;
		set_cp0_compare(next);
		now = get_cp0_count();
	}
}

#include "system_timer_generic.h"

static void timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	timer_core_announce();
}

static int sys_clock_driver_init(void)
{

	IRQ_CONNECT(MIPS_MACHINE_TIMER_IRQ, 0, timer_isr, NULL, 0);
	timer_core_init();
	irq_enable(MIPS_MACHINE_TIMER_IRQ);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
