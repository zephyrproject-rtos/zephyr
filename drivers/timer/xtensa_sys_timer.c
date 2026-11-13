/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys/clock.h>
#include <zephyr/irq.h>

#include "xtensa_sys_timer.h"

#define TIMER_IRQ UTIL_CAT(XCHAL_TIMER,		\
			   UTIL_CAT(CONFIG_XTENSA_TIMER_ID, _INTERRUPT))

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = UTIL_CAT(XCHAL_TIMER,
					 UTIL_CAT(CONFIG_XTENSA_TIMER_ID, _INTERRUPT));
#endif

static uint32_t ccount_compensation;
#ifdef CONFIG_XTENSA_TIMER_LPM_TIMER_HOOK
static uint32_t ccount_pre_idle;
static uint64_t lptim_pre_idle;
static bool timeout_idle;
#endif

static uint32_t ccount_comp(void)
{
	if (IS_ENABLED(CONFIG_XTENSA_TIMER_LPM_TIMER_HOOK)) {
		return ccount_compensation;
	}

	return 0;
}

static void set_ccompare(uint32_t val)
{
	__asm__ volatile ("wsr.CCOMPARE" STRINGIFY(CONFIG_XTENSA_TIMER_ID) " %0"
			  :: "r"(val));
}

static uint32_t ccount_raw(void)
{
	uint32_t val;

	__asm__ volatile ("rsr.CCOUNT %0" : "=r"(val));
	return val;
}

static uint32_t ccount(void)
{
	return ccount_raw() + ccount_comp();
}

/*
 * Free-running 32-bit CCOUNT plus a CCOMPARE register that fires only on
 * CCOUNT == CCOMPARE, so an already-past target is missed until CCOUNT wraps.
 * That is the COMPARE_EXACT backend: the core rewrites through its verify loop,
 * replacing the MIN_DELAY floor this driver used to apply by hand. Writing
 * CCOMPARE clears the pending interrupt. The cycle count carries the low-power
 * compensation (ccount_comp), so the raw register value written is the target
 * minus that offset.
 */
#define TIMER_CORE_BACKEND_COMPARE_EXACT

static inline uint32_t timer_driver_cycle_get(void)
{
	return ccount();
}

static inline void timer_driver_set_compare(uint64_t cycles)
{
	set_ccompare((uint32_t)cycles - ccount_comp());
}

#include "system_timer_generic.h"

static void ccompare_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = sys_clock_lock();

	/* Disarm the IRQ. Writing CCOMPARE is what clears the pending timer
	 * interrupt, and the match is on equality, so a compare one behind the
	 * current count both acknowledges it and puts the next match a whole
	 * counter period away, which is as far as a 32-bit comparator reaches.
	 * This interrupt then fires once for the deadline it was armed with,
	 * whether or not another one is programmed afterwards.
	 */
	set_ccompare(ccount_raw() - 1);

	timer_core_announce_from(key);
}

#ifdef CONFIG_SMP
void smp_timer_init(void)
{
	timer_core_smp_prime();
	irq_enable(TIMER_IRQ);
}
#endif

#ifdef CONFIG_XTENSA_TIMER_LPM_TIMER_HOOK
void sys_clock_idle_enter(uint32_t ticks)
{
	/* Arm the comparator for the wakeup, then hand off to the low-power
	 * timer that keeps time while CCOUNT is stalled.
	 */
	sys_clock_set_timeout(ticks, false);

	/* ticks may be SYS_CLOCK_IDLE_FOREVER, which would overflow the conversion.
	 * The core has already clamped what it armed above, so clamp this the
	 * same way.
	 */
	uint32_t lp_ticks = MIN(ticks, k_cyc_to_ticks_floor32(TIMER_CORE_ALARM_MAX_CYCLES));
	uint64_t timeout_us = k_ticks_to_us_ceil64(lp_ticks);

	lptim_pre_idle = z_xtensa_lptim_hook_on_lpm_entry(timeout_us);
	ccount_pre_idle = ccount();
	timeout_idle = true;
}

void sys_clock_idle_exit(void)
{
	if (!timeout_idle) {
		return;
	}

	k_spinlock_key_t key = sys_clock_lock();

	uint64_t lptim_now = z_xtensa_lptim_hook_on_lpm_exit();
	uint32_t ccount_diff = (uint32_t)ccount() - ccount_pre_idle;
	uint64_t lptim_diff = lptim_now - lptim_pre_idle;
	uint64_t expected_cycles =
		(lptim_diff * sys_clock_hw_cycles_per_sec()) / z_xtensa_lptim_hook_get_freq();

	uint64_t missed_cycles = 0;

	if (expected_cycles > ccount_diff) {
		/* CCOUNT stalled during LPM. Fold the missed cycles into the
		 * compensation so the cycle count picks up the real elapsed time,
		 * and hand the span to the core directly: a sleep can outrun the
		 * 32-bit CCOUNT, so the announce cannot come from the counter.
		 * What CCOUNT did see stays there for the next announce.
		 */
		missed_cycles = expected_cycles - ccount_diff;
		ccount_compensation += (uint32_t)missed_cycles;
	}

	timeout_idle = false;

	timer_core_announce_cycles64_from(key, missed_cycles);
}
#endif /* CONFIG_XTENSA_TIMER_LPM_TIMER_HOOK */

static int sys_clock_driver_init(void)
{
	IRQ_CONNECT(TIMER_IRQ, 0, ccompare_isr, 0, 0);
	timer_core_init();
	irq_enable(TIMER_IRQ);
	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
