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

#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec()	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_TICKS ((k_ticks_t)(COUNTER_MAX / CYC_PER_TICK) - 1)
#define MAX_CYCLES (MAX_TICKS * CYC_PER_TICK)
#define MIN_DELAY 1

#define TIMER_IRQ (DT_INST_IRQN(0))

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = TIMER_IRQ;
#endif

/* Value of STIMER counter when the previous kernel tick was announced */
static atomic_t g_last_count;

/* Spinlock to sync between Compare ISR and update of Compare register */
static struct k_spinlock g_lock;

static void stimer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	uint32_t irq_status = am_hal_stimer_int_status_get(false);

	if (irq_status & AM_HAL_STIMER_INT_COMPAREA) {
		am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREA);

		k_spinlock_key_t key = k_spin_lock(&g_lock);

		uint32_t now = am_hal_stimer_counter_get();
		uint32_t dticks = (uint32_t)((now - g_last_count) / CYC_PER_TICK);

		g_last_count += dticks * CYC_PER_TICK;

		if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
			uint32_t next = g_last_count + CYC_PER_TICK;

			if ((int32_t)(next - now) < MIN_DELAY) {
				next += CYC_PER_TICK;
			}
			am_hal_stimer_compare_delta_set(0, next - g_last_count);
		}

		k_spin_unlock(&g_lock, key);
		sys_clock_announce(dticks);
	}
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	if (ticks == K_TICKS_FOREVER) {
		return;
	}

	ticks = MIN(MAX_TICKS, ticks);
	/* If tick is 0, set delta cyc to MIN_DELAY to trigger tick isr asap */
	uint32_t cyc = MAX(ticks * CYC_PER_TICK, MIN_DELAY);

	k_spinlock_key_t key = k_spin_lock(&g_lock);

	am_hal_stimer_compare_delta_set(0, cyc);

	k_spin_unlock(&g_lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&g_lock);
	uint32_t ret = (am_hal_stimer_counter_get()
			- g_last_count) / CYC_PER_TICK;

	k_spin_unlock(&g_lock, key);
	return ret;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return am_hal_stimer_counter_get();
}

static int stimer_init(void)
{
	uint32_t oldCfg;
	k_spinlock_key_t key = k_spin_lock(&g_lock);

	oldCfg = am_hal_stimer_config(AM_HAL_STIMER_CFG_FREEZE);

	am_hal_stimer_config((oldCfg & ~(AM_HAL_STIMER_CFG_FREEZE | STIMER_STCFG_CLKSEL_Msk))
			| AM_HAL_STIMER_XTAL_32KHZ
			| AM_HAL_STIMER_CFG_COMPARE_A_ENABLE);

	g_last_count = am_hal_stimer_counter_get();

	k_spin_unlock(&g_lock, key);

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

SYS_INIT(stimer_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
