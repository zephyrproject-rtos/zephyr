/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 * Copyright (c) 2025, Ambiq Micro Inc. <www.ambiq.com>
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
#include <soc.h>

#define COUNTER_MAX UINT32_MAX

#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_TICKS    ((k_ticks_t)(COUNTER_MAX / CYC_PER_TICK) - 1)
#define MAX_CYCLES   (MAX_TICKS * CYC_PER_TICK)
#if defined(CONFIG_SOC_SERIES_APOLLO3X) || defined(CONFIG_SOC_SERIES_APOLLO5X)
#define MIN_DELAY 1
#else
#define MIN_DELAY 4
#endif

#if defined(CONFIG_SOC_SERIES_APOLLO5X)
#define COMPARE_INTERRUPT AM_HAL_STIMER_INT_COMPAREA
#else
/* A Possible clock glitch could rarely cause the Stimer interrupt to be lost.
 * Set up a backup comparator to handle this case
 */
#define COMPARE_INTERRUPT (AM_HAL_STIMER_INT_COMPAREA | AM_HAL_STIMER_INT_COMPAREB)
#endif

#define COMPAREA_IRQ (DT_INST_IRQN(0))
#define COMPAREB_IRQ (COMPAREA_IRQ + 1)

#define TIMER_CLKSRC (DT_INST_PROP(0, clk_source))

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = COMPAREA_IRQ;
#endif

/* Elapsed ticks since the previous kernel tick was announced, It will get accumulated every time
 * stimer_isr is triggered, or sys_clock_set_timeout/sys_clock_elapsed API is called.
 * It will be cleared after sys_clock_announce is called.
 */
static volatile uint32_t g_tick_elapsed;

/* Value of STIMER counter when the previous timer API is called */
static volatile uint32_t g_last_time_stamp;

/* Value of g_last_time_stamp from a whole tick */
static volatile uint32_t g_remainder;

/* Spinlock to sync between Compare ISR and update of Compare register */
static struct k_spinlock g_lock;

static void update_tick_counter_with_now(uint32_t now)
{
	/* unsigned subtraction handles HW wrap modulo 2^32 */
	uint32_t elapsed = now - g_last_time_stamp;

	/* do the sum in 64-bit to avoid wrapping */
	uint64_t total_cycles = (uint64_t)elapsed + (uint64_t)g_remainder;

	/* fold into ticks and keep remainder < CYC_PER_TICK */
	uint32_t dticks = (uint32_t)(total_cycles / (uint64_t)CYC_PER_TICK);

	g_remainder = (uint32_t)(total_cycles % (uint64_t)CYC_PER_TICK);

	/* update state */
	g_last_time_stamp = now;
	g_tick_elapsed += dticks;
}

static void ambiq_stimer_delta_set(uint32_t ui32Delta)
{
	am_hal_stimer_compare_delta_set(0, ui32Delta);
#if !defined(CONFIG_SOC_SERIES_APOLLO5X)
	am_hal_stimer_compare_delta_set(1, ui32Delta + 1);
#endif
}

void stimer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	uint32_t irq_status = am_hal_stimer_int_status_get(true);

	if (irq_status & COMPARE_INTERRUPT) {
		am_hal_stimer_int_clear(COMPARE_INTERRUPT);

		k_spinlock_key_t key = k_spin_lock(&g_lock);

		/* Read current cycle count. */
		uint32_t now = am_hal_stimer_counter_get();

		/*Calculate the elapsed ticks based on the current cycle count*/
		update_tick_counter_with_now(now);

		uint32_t ticks_to_announce = g_tick_elapsed;

		g_tick_elapsed = 0;

		if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
			uint32_t delta = CYC_PER_TICK < MIN_DELAY ? MIN_DELAY : CYC_PER_TICK;

			ambiq_stimer_delta_set(delta);
		}

		k_spin_unlock(&g_lock, key);

		sys_clock_announce(ticks_to_announce);
	} else {
		am_hal_stimer_int_clear(irq_status);
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
	uint32_t delta = ticks * CYC_PER_TICK;

	if (delta <= MIN_DELAY) {
		/*If the delta value is smaller than MIN_DELAY, trigger a interrupt immediately*/
		am_hal_stimer_int_set(COMPARE_INTERRUPT);
	} else {
		ambiq_stimer_delta_set(delta);
	}

	k_spin_unlock(&g_lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&g_lock);

	uint32_t now = am_hal_stimer_counter_get();

	update_tick_counter_with_now(now);

	uint32_t elapsed = g_tick_elapsed;

	k_spin_unlock(&g_lock, key);

	return elapsed;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return am_hal_stimer_counter_get();
}

static int stimer_init(void)
{
	uint32_t oldCfg;

	oldCfg = am_hal_stimer_config(TIMER_CLKSRC | AM_HAL_STIMER_CFG_FREEZE);

#if defined(CONFIG_SOC_SERIES_APOLLO3X)
	am_hal_stimer_config((oldCfg & ~(AM_HAL_STIMER_CFG_FREEZE | CTIMER_STCFG_CLKSEL_Msk)) |
			     TIMER_CLKSRC | AM_HAL_STIMER_CFG_COMPARE_A_ENABLE |
			     AM_HAL_STIMER_CFG_COMPARE_B_ENABLE);
#elif defined(CONFIG_SOC_SERIES_APOLLO4X)
	am_hal_stimer_config((oldCfg & ~(AM_HAL_STIMER_CFG_FREEZE | STIMER_STCFG_CLKSEL_Msk)) |
			     TIMER_CLKSRC | AM_HAL_STIMER_CFG_COMPARE_A_ENABLE |
			     AM_HAL_STIMER_CFG_COMPARE_B_ENABLE);
#elif defined(CONFIG_SOC_SERIES_APOLLO5X)
	/* No need for backup comparator any more */
	am_hal_stimer_config((oldCfg & ~(AM_HAL_STIMER_CFG_FREEZE | STIMER_STCFG_CLKSEL_Msk)) |
			     TIMER_CLKSRC | AM_HAL_STIMER_CFG_COMPARE_A_ENABLE);
#endif
	g_last_time_stamp = am_hal_stimer_counter_get();
	g_remainder = 0;

	/* A Possible clock glitch could rarely cause the Stimer interrupt to be lost.
	 * Set up a backup comparator to handle this case
	 */
	NVIC_ClearPendingIRQ(COMPAREA_IRQ);
	IRQ_CONNECT(COMPAREA_IRQ, 0, stimer_isr, 0, 0);
	irq_enable(COMPAREA_IRQ);
#if !defined(CONFIG_SOC_SERIES_APOLLO5X)
	NVIC_ClearPendingIRQ(COMPAREB_IRQ);
	IRQ_CONNECT(COMPAREB_IRQ, 0, stimer_isr, 0, 0);
	irq_enable(COMPAREB_IRQ);
#endif
	am_hal_stimer_int_enable(COMPARE_INTERRUPT);
	/* Start timer with period CYC_PER_TICK if tickless is not enabled */
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		ambiq_stimer_delta_set(CYC_PER_TICK);
	}
	return 0;
}

SYS_INIT(stimer_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
