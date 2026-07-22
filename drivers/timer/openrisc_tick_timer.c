/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>

#include <openrisc/openriscregs.h>

#define MAX_CYC   SPR_TTMR_TP
#define MIN_DELAY 1000

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

/* Signed distance between two counts in the 28-bit compare domain: mask to the
 * TP field width, then shift the top bit to bit 31 so the arithmetic shift
 * sign-extends it back.
 */
static ALWAYS_INLINE int32_t cyc_diff(uint32_t a, uint32_t b)
{
	return (int32_t)(((a - b) & MAX_CYC) << 4) >> 4;
}

/*
 * Free-running TTCR count with an equality-match TTMR compare: a COMPARE
 * backend. The compare (TTMR.TP) is only 28 bits wide while TTCR is 32, so the
 * counter is treated as 28-bit (TIMER_CORE_CYCLES_WIDTH) and the target is masked to that
 * width. TTMR matches only TTCR[27:0] == TP, so timer_driver_set_compare() bumps an
 * already-past target at least MIN_DELAY ahead rather than let it wait for the
 * 28-bit count to wrap, meeting the core's "must not miss a past deadline"
 * contract; writing TTMR also clears the pending interrupt.
 */
#define TIMER_CORE_CYCLES_WIDTH 28
#define TIMER_CORE_BACKEND_COMPARE

static inline uint64_t timer_driver_cycle_get(void)
{
	return get_count();
}

static inline void timer_driver_set_compare(uint64_t cycles)
{
	uint32_t next = (uint32_t)cycles & MAX_CYC;

	set_compare(next);

	uint32_t now = get_count() & MAX_CYC;

	while (cyc_diff(next, now) <= 0) {
		next = (now + MIN_DELAY) & MAX_CYC;
		set_compare(next);
		now = get_count() & MAX_CYC;
	}
}

#include "system_timer_generic.h"

void z_openrisc_timer_isr(void)
{
	if (IS_ENABLED(CONFIG_TRACING_ISR)) {
		sys_trace_isr_enter();
	}

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Disarm; the kernel re-arms through sys_clock_set_timeout() after
		 * the announce. A tickful kernel re-arms in timer_core_announce().
		 */
		clear_compare();
	}

	timer_core_announce();

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
