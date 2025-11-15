/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>

#include <openrisc/openriscregs.h>

#define MAX_CYC SPR_TTMR_TP

static struct k_spinlock lock;
static uint32_t last_count;
static uint64_t last_ticks;
static uint32_t last_elapsed;
static uint32_t cyc_per_tick;

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

void z_openrisc_timer_isr(void)
{
	if (IS_ENABLED(CONFIG_TRACING_ISR)) {
		sys_trace_isr_enter();
	}

	const k_spinlock_key_t key = k_spin_lock(&lock);

	const uint32_t current_count = get_count();
	const uint32_t delta_count = current_count - last_count;
	const uint32_t delta_ticks = delta_count / cyc_per_tick;

	last_count += delta_ticks * cyc_per_tick;
	last_ticks += delta_ticks;
	last_elapsed = 0;

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		clear_compare();
	} else {
		set_compare((last_count + cyc_per_tick) & MAX_CYC);
	}

	k_spin_unlock(&lock, key);
	sys_clock_announce(delta_ticks);

	if (IS_ENABLED(CONFIG_TRACING_ISR)) {
		sys_trace_isr_exit();
	}
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
#if defined(CONFIG_TICKLESS_KERNEL)
	if (ticks == K_TICKS_FOREVER) {
		if (idle) {
			return;
		}

		ticks = INT32_MAX;
	}

	/*
	 * Clamp the max period length to a number of cycles that can fit
	 * in half the range of a cycle_diff_t for native width divisions
	 * to be usable elsewhere. The half range gives us extra room to cope
	 * with the unavoidable IRQ servicing latency.
	 */
	ticks = CLAMP(ticks, 0, MAX_CYC / 2 / cyc_per_tick);

	const uint32_t compare =
		((last_ticks + last_elapsed + (uint32_t)ticks) * cyc_per_tick) & MAX_CYC;
	const k_spinlock_key_t key = k_spin_lock(&lock);

	set_compare(compare);
	k_spin_unlock(&lock, key);

#else  /* CONFIG_TICKLESS_KERNEL */
	ARG_UNUSED(ticks);
	ARG_UNUSED(idle);
#endif
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	const k_spinlock_key_t key = k_spin_lock(&lock);

	const uint32_t current_count = get_count();
	const uint32_t delta_count = current_count - last_count;
	const uint32_t delta_ticks = delta_count / cyc_per_tick;

	last_elapsed = delta_ticks;
	k_spin_unlock(&lock, key);
	return delta_ticks;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return get_count();
}

static int sys_clock_driver_init(void)
{
	cyc_per_tick = (uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec() /
		(uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC);

	last_ticks = get_count() / cyc_per_tick;
	last_count = last_ticks * cyc_per_tick;
	last_elapsed = 0;

	set_compare((last_count + cyc_per_tick) & MAX_CYC);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
