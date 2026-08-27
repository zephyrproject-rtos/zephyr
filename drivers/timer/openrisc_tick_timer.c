/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/clock.h>

#include <openrisc/openriscregs.h>

#define MAX_CYC SPR_TTMR_TP

static ALWAYS_INLINE void set_compare(uint32_t time)
{
	openrisc_write_spr(SPR_TTMR, SPR_TTMR_IE | SPR_TTMR_CR | time);
}

static ALWAYS_INLINE void clear_compare(void)
{
	openrisc_write_spr(SPR_TTMR, SPR_TTMR_CR);
}

static ALWAYS_INLINE uint32_t get_count(void)
{
	return openrisc_read_spr(SPR_TTCR);
}

/*
 * A 28-bit free-running count with a compare that matches only TTCR[27:0] == TP,
 * so a target written at or behind the count is missed until the count wraps,
 * 13.4 s at 20 MHz. That is the COMPARE_EXACT backend: the core writes the
 * register through its verify loop.
 */
#define TIMER_CORE_COUNTER_WIDTH 28
#define TIMER_CORE_BACKEND_COMPARE_EXACT

static inline uint32_t timer_driver_cycle_get(void)
{
	return get_count();
}

static inline void timer_driver_set_compare(uint64_t cycles)
{
	set_compare((uint32_t)cycles & MAX_CYC);
}

#include "system_timer_generic.h"

void z_openrisc_timer_isr(void)
{
	if (IS_ENABLED(CONFIG_TRACING_ISR)) {
		sys_trace_isr_enter();
	}

	const k_spinlock_key_t key = sys_clock_lock();

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		clear_compare();
	}

	timer_core_announce_from(key);

	if (IS_ENABLED(CONFIG_TRACING_ISR)) {
		sys_trace_isr_exit();
	}
}

static int sys_clock_driver_init(void)
{
	timer_core_init();

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
