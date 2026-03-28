/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>

#include "xtensa_sys_timer.h"

#define TIMER_IRQ UTIL_CAT(XCHAL_TIMER,		\
			   UTIL_CAT(CONFIG_XTENSA_TIMER_ID, _INTERRUPT))

#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec()	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_CYC 0xffffffffu
#define MAX_TICKS ((MAX_CYC - CYC_PER_TICK) / CYC_PER_TICK)
#define MIN_DELAY 1000

static struct k_spinlock lock;
static unsigned int last_count;

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = UTIL_CAT(XCHAL_TIMER,
					 UTIL_CAT(CONFIG_XTENSA_TIMER_ID, _INTERRUPT));
#endif

static uint32_t ccount_compensation;
static uint32_t ccount_pre_idle;
static uint64_t lptim_pre_idle;
static bool timeout_idle;

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

static uint32_t ccount(void)
{
	uint32_t val;

	__asm__ volatile ("rsr.CCOUNT %0" : "=r"(val));
	return val + ccount_comp();
}

static uint32_t sys_clock_elapsed_ticks(uint32_t curr)
{
	uint32_t dticks = (curr - last_count) / CYC_PER_TICK;

	last_count += dticks * CYC_PER_TICK;

	return dticks;
}

static void ccompare_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t curr = ccount();
	uint32_t dticks = sys_clock_elapsed_ticks(curr);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint32_t next = last_count + CYC_PER_TICK;

		if ((int32_t)(next - curr) < MIN_DELAY) {
			next += CYC_PER_TICK;
		}
		set_ccompare(next - ccount_comp());
	}

	k_spin_unlock(&lock, key);
	sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? dticks : 1);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
#if defined(CONFIG_TICKLESS_KERNEL)
	ticks = ticks == K_TICKS_FOREVER ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t curr = ccount(), cyc, adj;

	/* Round up to next tick boundary */
	cyc = ticks * CYC_PER_TICK;
	adj = (curr - last_count) + (CYC_PER_TICK - 1);
	if (cyc <= MAX_CYC - adj) {
		cyc += adj;
	} else {
		cyc = MAX_CYC;
	}
	cyc = (cyc / CYC_PER_TICK) * CYC_PER_TICK;
	cyc += last_count;

	if ((cyc - curr) < MIN_DELAY) {
		cyc += CYC_PER_TICK;
	}

	set_ccompare(cyc - ccount_comp());

	if (IS_ENABLED(CONFIG_XTENSA_TIMER_LPM_TIMER_HOOK)) {
		if (idle && ticks != K_TICKS_FOREVER) {
			uint64_t timeout_us =
				((uint64_t)ticks * USEC_PER_SEC) / CONFIG_SYS_CLOCK_TICKS_PER_SEC;

			lptim_pre_idle = z_xtensa_lptim_hook_on_lpm_entry(timeout_us);
			ccount_pre_idle = ccount();
			timeout_idle = true;
		} else {
			timeout_idle = false;
		}
	} else {
		ARG_UNUSED(idle);
	}

	k_spin_unlock(&lock, key);
#endif
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = (ccount() - last_count) / CYC_PER_TICK;

	k_spin_unlock(&lock, key);
	return ret;
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

		if (!timeout_idle) {
			return;
		}

		k_spinlock_key_t key = k_spin_lock(&lock);

		uint64_t lptim_now = z_xtensa_lptim_hook_on_lpm_exit();
		uint64_t ccount_now = ccount();
		uint64_t lptim_diff = lptim_now - lptim_pre_idle;
		uint32_t ccount_diff = ccount_now - ccount_pre_idle;
		uint64_t expected_cycles = (lptim_diff * sys_clock_hw_cycles_per_sec()) /
					   z_xtensa_lptim_hook_get_freq();
		uint32_t missed_cycles = 0;

		if (expected_cycles > ccount_diff) {
			missed_cycles = (uint32_t)(expected_cycles - ccount_diff);
		}

		ccount_compensation += missed_cycles;

		ccount_now = ccount();
		uint32_t dticks = sys_clock_elapsed_ticks(ccount_now);

		timeout_idle = false;

		k_spin_unlock(&lock, key);

		/* Announce corrected ticks as CCOUNT remained stalled during LPM */
		sys_clock_announce(dticks);
	}
}

static int sys_clock_driver_init(void)
{
	IRQ_CONNECT(TIMER_IRQ, 0, ccompare_isr, 0, 0);
	set_ccompare(ccount() + CYC_PER_TICK);
	irq_enable(TIMER_IRQ);
	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
