/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_stimer

/**
 * @file
 * @brief Ambiq Apollo STIMER-based sys_clock driver
 *
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>

/* ambiq-sdk includes */
#include <am_mcu_apollo.h>

#define COUNTER_MAX UINT32_MAX

#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_TICKS    ((k_ticks_t)(COUNTER_MAX / CYC_PER_TICK) - 1)
#define MAX_CYCLES   (MAX_TICKS * CYC_PER_TICK)
#define MIN_DELAY    1

#define TIMER_IRQ (DT_INST_IRQN(0))

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = TIMER_IRQ;
#endif

/* Elapsed ticks since the previous kernel tick was announced, It will get accumulated every time
 * stimer_isr is triggered, or sys_clock_set_timeout/sys_clock_elapsed API is called.
 * It will be cleared after sys_clock_announce is called,.
 */
static uint32_t g_tick_elapsed;

/* Value of STIMER counter when the previous timer API is called, this value is
 * aligned to tick boundary. It is updated along with the g_tick_elapsed value.
 */
static uint32_t g_last_time_stamp;

/* Spinlock to sync between Compare ISR and update of Compare register */
static struct k_spinlock g_lock;

static void update_tick_counter(void)
{
	/* Read current cycle count. */
	uint32_t now = am_hal_stimer_counter_get();

	/* If current cycle count is smaller than the last time stamp, a counter overflow happened.
	 * We need to extend the current counter value to 64 bits and add it with 0xFFFFFFFF
	 * to get the correct elapsed cycles.
	 */
	uint64_t now_64 = (g_last_time_stamp <= now) ? (uint64_t)now : (uint64_t)now + COUNTER_MAX;

	/* Get elapsed cycles */
	uint32_t elapsed_cycle = (now_64 - g_last_time_stamp);

	/* Get elapsed ticks. */
	uint32_t dticks = elapsed_cycle / CYC_PER_TICK;

	g_last_time_stamp += dticks * CYC_PER_TICK;
	g_tick_elapsed += dticks;
}

static void stimer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	uint32_t irq_status = am_hal_stimer_int_status_get(false);

	if (irq_status & AM_HAL_STIMER_INT_COMPAREA) {
		am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREA);

		k_spinlock_key_t key = k_spin_lock(&g_lock);

		/*Calculate the elapsed ticks based on the current cycle count*/
		update_tick_counter();

		if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {

			/* Get the counter value to trigger the next tick interrupt. */
			uint64_t next = (uint64_t)g_last_time_stamp + CYC_PER_TICK;

			/* Read current cycle count. */
			uint32_t now = am_hal_stimer_counter_get();

			/* If current cycle count is smaller than the last time stamp, a counter
			 * overflow happened. We need to extend the current counter value to 64 bits
			 * and add 0xFFFFFFFF to get the correct elapsed cycles.
			 */
			uint64_t now_64 = (g_last_time_stamp <= now) ? (uint64_t)now
								     : (uint64_t)now + COUNTER_MAX;

			uint32_t delta = (now_64 + MIN_DELAY < next) ? (next - now_64) : MIN_DELAY;

			/* Set delta. */
			am_hal_stimer_compare_delta_set(0, delta);
		}

		k_spin_unlock(&g_lock, key);

		sys_clock_announce(g_tick_elapsed);
		g_tick_elapsed = 0;
	}
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	/* Adjust the ticks to the range of [1, MAX_TICKS]. */
	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks, 1, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&g_lock);

	/* Update the internal tick counter*/
	update_tick_counter();

	/* Get current hardware counter value.*/
	uint32_t now = am_hal_stimer_counter_get();

	/* last: the last recorded counter value.
	 * now_64: current counter value. Extended to uint64_t to easy the handing of hardware
	 *         counter overflow.
	 * next: counter values where to trigger the scheduled timeout.
	 * last < now_64 < next
	 */
	uint64_t last = (uint64_t)g_last_time_stamp;
	uint64_t now_64 = (g_last_time_stamp <= now) ? (uint64_t)now : (uint64_t)now + COUNTER_MAX;
	uint64_t next = now_64 + ticks * CYC_PER_TICK;

	uint32_t gap = next - last;
	uint32_t gap_aligned = (gap / CYC_PER_TICK) * CYC_PER_TICK;
	uint64_t next_aligned = last + gap_aligned;

	uint32_t delta = next_aligned - now_64;

	if (delta <= MIN_DELAY) {
		/*If the delta value is smaller than MIN_DELAY, trigger a interrupt immediately*/
		am_hal_stimer_int_set(AM_HAL_STIMER_INT_COMPAREA);
	} else {
		am_hal_stimer_compare_delta_set(0, delta);
	}

	k_spin_unlock(&g_lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&g_lock);
	update_tick_counter();
	k_spin_unlock(&g_lock, key);

	return g_tick_elapsed;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return am_hal_stimer_counter_get();
}

static int stimer_init(void)
{
	uint32_t oldCfg;

	oldCfg = am_hal_stimer_config(AM_HAL_STIMER_CFG_FREEZE);

#if defined(CONFIG_SOC_SERIES_APOLLO3X)
	am_hal_stimer_config((oldCfg & ~(AM_HAL_STIMER_CFG_FREEZE | CTIMER_STCFG_CLKSEL_Msk)) |
			     AM_HAL_STIMER_XTAL_32KHZ | AM_HAL_STIMER_CFG_COMPARE_A_ENABLE);
#else
	am_hal_stimer_config((oldCfg & ~(AM_HAL_STIMER_CFG_FREEZE | STIMER_STCFG_CLKSEL_Msk)) |
			     AM_HAL_STIMER_XTAL_32KHZ | AM_HAL_STIMER_CFG_COMPARE_A_ENABLE);
#endif
	g_last_time_stamp = am_hal_stimer_counter_get();

	NVIC_ClearPendingIRQ(TIMER_IRQ);
	IRQ_CONNECT(TIMER_IRQ, 0, stimer_isr, 0, 0);
	irq_enable(TIMER_IRQ);

	am_hal_stimer_int_enable(AM_HAL_STIMER_INT_COMPAREA);
	/* Start timer with period CYC_PER_TICK if tickless is not enabled */
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		am_hal_stimer_compare_delta_set(0, CYC_PER_TICK);
	}
	return 0;
}

SYS_INIT(stimer_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
