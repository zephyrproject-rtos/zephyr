/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Original ESP32 (Xtensa) per-core system timer driver.
 *
 * A private fork of xtensa_sys_timer.c, needed because that driver's
 * tick-tracking state (last_count and friends) is a single shared variable
 * per instance, which is only correct on Xtensa SoCs where CCOUNT/CCOMPARE
 * is a single shared hardware resource (e.g. intel_adsp's external,
 * cross-core-synchronized wall clock timer, which is why
 * drivers/timer/intel_adsp_timer.c does not need this). Original ESP32's
 * CCOUNT/CCOMPARE are genuine per-core Xtensa special registers with no
 * shared oscillator guarantee between cores, so under CONFIG_SMP each core
 * needs its own independent copy of this state -- otherwise one core's
 * tick delta math silently mixes in another core's unrelated CCOUNT base.
 *
 * Kept as a separate driver (selected in place of the generic XTENSA_TIMER
 * for SOC_SERIES_ESP32, see Kconfig.xtensa_esp32) rather than a change to
 * the shared driver: xtensa_sys_timer.c is also used by AMD, NXP,
 * MediaTek, and Cadence Xtensa SoCs, none of which are SMP-capable there
 * today.
 */

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/irq.h>

#include "xtensa_sys_timer.h"

/* Original ESP32 always uses timer 0 -- no board overrides
 * CONFIG_XTENSA_TIMER_ID away from its default, and referencing that
 * symbol here would be awkward since it depends on CONFIG_XTENSA_TIMER,
 * which this driver deliberately disables (see Kconfig.xtensa_esp32).
 */
#define TIMER_IRQ XCHAL_TIMER0_INTERRUPT

#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_CYC      0xffffffffu
#define MAX_TICKS    ((MAX_CYC - CYC_PER_TICK) / CYC_PER_TICK)
#define MIN_DELAY    1000

/* Each core has its own independent, free-running CCOUNT register with no
 * shared oscillator guarantee between cores -- these must be per-core
 * state, not single shared variables, or one core's delta math silently
 * mixes in another core's unrelated CCOUNT base.
 */
static unsigned int last_count[CONFIG_MP_MAX_NUM_CPUS];
static uint32_t ccount_compensation[CONFIG_MP_MAX_NUM_CPUS];
static uint32_t ccount_pre_idle[CONFIG_MP_MAX_NUM_CPUS];
static uint64_t lptim_pre_idle[CONFIG_MP_MAX_NUM_CPUS];
static bool timeout_idle[CONFIG_MP_MAX_NUM_CPUS];

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = XCHAL_TIMER0_INTERRUPT;
#endif

static uint32_t ccount_comp(void)
{
	if (IS_ENABLED(CONFIG_XTENSA_TIMER_LPM_TIMER_HOOK)) {
		return ccount_compensation[_current_cpu->id];
	}

	return 0;
}

static void set_ccompare(uint32_t val)
{
	__asm__ volatile("wsr.CCOMPARE0 %0" ::"r"(val));
}

static uint32_t ccount(void)
{
	uint32_t val;

	__asm__ volatile("rsr.CCOUNT %0" : "=r"(val));
	return val + ccount_comp();
}

static uint32_t sys_clock_elapsed_ticks(uint32_t curr)
{
	unsigned int id = _current_cpu->id;
	uint32_t dticks = (curr - last_count[id]) / CYC_PER_TICK;

	last_count[id] += dticks * CYC_PER_TICK;

	return dticks;
}

static void ccompare_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = sys_clock_lock();

	uint32_t curr = ccount();
	uint32_t dticks = sys_clock_elapsed_ticks(curr);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint32_t next = last_count[_current_cpu->id] + CYC_PER_TICK;

		if ((int32_t)(next - curr) < MIN_DELAY) {
			next += CYC_PER_TICK;
		}
		set_ccompare(next - ccount_comp());
	}

	sys_clock_announce_locked(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? dticks : 1, key);
}

void sys_clock_set_timeout(uint32_t ticks, bool idle)
{
	__ASSERT(sys_clock_is_locked(), "system clock lock not held");

#if defined(CONFIG_TICKLESS_KERNEL)
	unsigned int id = _current_cpu->id;

	ticks = CLAMP(ticks, 1, MAX_TICKS) - 1;

	uint32_t curr = ccount(), cyc, adj;

	/* Round up to next tick boundary */
	cyc = ticks * CYC_PER_TICK;
	adj = (curr - last_count[id]) + (CYC_PER_TICK - 1);
	if (cyc <= MAX_CYC - adj) {
		cyc += adj;
	} else {
		cyc = MAX_CYC;
	}
	cyc = (cyc / CYC_PER_TICK) * CYC_PER_TICK;
	cyc += last_count[id];

	if ((cyc - curr) < MIN_DELAY) {
		cyc += CYC_PER_TICK;
	}

	set_ccompare(cyc - ccount_comp());

	if (IS_ENABLED(CONFIG_XTENSA_TIMER_LPM_TIMER_HOOK)) {
		if (idle) {
			uint64_t timeout_us =
				((uint64_t)ticks * USEC_PER_SEC) / CONFIG_SYS_CLOCK_TICKS_PER_SEC;

			lptim_pre_idle[id] = z_xtensa_lptim_hook_on_lpm_entry(timeout_us);
			ccount_pre_idle[id] = ccount();
			timeout_idle[id] = true;
		}
	} else {
		ARG_UNUSED(idle);
	}

#endif
}

uint32_t sys_clock_elapsed(void)
{
	__ASSERT(sys_clock_is_locked(), "system clock lock not held");

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	return (ccount() - last_count[_current_cpu->id]) / CYC_PER_TICK;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return ccount();
}

#ifdef CONFIG_SMP
void smp_timer_init(void)
{
	set_ccompare(ccount() + CYC_PER_TICK);
	irq_enable(TIMER_IRQ);
}
#endif

void sys_clock_idle_exit(void)
{
	if (IS_ENABLED(CONFIG_XTENSA_TIMER_LPM_TIMER_HOOK)) {
		unsigned int id = _current_cpu->id;

		if (!timeout_idle[id]) {
			return;
		}

		k_spinlock_key_t key = sys_clock_lock();

		uint64_t lptim_now = z_xtensa_lptim_hook_on_lpm_exit();
		uint64_t ccount_now = ccount();
		uint64_t lptim_diff = lptim_now - lptim_pre_idle[id];
		uint32_t ccount_diff = ccount_now - ccount_pre_idle[id];
		uint64_t expected_cycles = (lptim_diff * sys_clock_hw_cycles_per_sec()) /
					   z_xtensa_lptim_hook_get_freq();
		uint32_t missed_cycles = 0;

		if (expected_cycles > ccount_diff) {
			missed_cycles = (uint32_t)(expected_cycles - ccount_diff);
		}

		ccount_compensation[id] += missed_cycles;

		ccount_now = ccount();
		uint32_t dticks = sys_clock_elapsed_ticks(ccount_now);

		timeout_idle[id] = false;

		/* Announce corrected ticks as CCOUNT remained stalled during LPM */
		sys_clock_announce_locked(dticks, key);
	}
}

static int sys_clock_driver_init(void)
{
	IRQ_CONNECT(TIMER_IRQ, 0, ccompare_isr, 0, 0);
	set_ccompare(ccount() + CYC_PER_TICK);
	irq_enable(TIMER_IRQ);
	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
