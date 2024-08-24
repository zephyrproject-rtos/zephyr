/* Copyright (C) 2023 BeagleBoard.org Foundation
 * Copyright (C) 2023 S Prashanth
 * Copyright (c) 2024 Texas Instruments Incorporated
 *	Andrew Davis <afd@ti.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

#include <zephyr/drivers/timer/ti_dmtimer.h>

#define DT_DRV_COMPAT ti_am654_timer

#define TIMER_BASE_ADDR DT_INST_REG_ADDR(0)

#define TIMER_IRQ_NUM   DT_INST_IRQN(0)
#define TIMER_IRQ_PRIO  DT_INST_IRQ(0, priority)
#define TIMER_IRQ_FLAGS DT_INST_IRQ(0, flags)

#define CYC_PER_TICK ((uint32_t)(sys_clock_hw_cycles_per_sec() \
				/ CONFIG_SYS_CLOCK_TICKS_PER_SEC))

#define MAX_TICKS ((k_ticks_t)(UINT32_MAX / CYC_PER_TICK) - 1)

static struct k_spinlock lock;

static uint32_t last_cycle;

#define TI_DM_TIMER_READ(reg) sys_read32(TIMER_BASE_ADDR + TI_DM_TIMER_ ## reg)

#define TI_DM_TIMER_MASK(reg) TI_DM_TIMER_ ## reg ## _MASK
#define TI_DM_TIMER_SHIFT(reg) TI_DM_TIMER_ ## reg ## _SHIFT
#define TI_DM_TIMER_WRITE(data, reg, bits) \
	ti_dm_timer_write_masks(data, \
		TIMER_BASE_ADDR + TI_DM_TIMER_ ## reg, \
		TI_DM_TIMER_MASK(reg ## _ ## bits), \
		TI_DM_TIMER_SHIFT(reg ## _ ## bits))

static void ti_dm_timer_write_masks(uint32_t data, uint32_t reg, uint32_t mask, uint32_t shift)
{
	uint32_t reg_val;

	reg_val = sys_read32(reg);
	reg_val = (reg_val & ~(mask)) | (data << shift);
	sys_write32(reg_val, reg);
}

static void ti_dmtimer_isr(void *data)
{
	/* If no pending event */
	if (!TI_DM_TIMER_READ(IRQSTATUS)) {
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t curr_cycle = TI_DM_TIMER_READ(TCRR);
	uint32_t delta_cycles = curr_cycle - last_cycle;
	uint32_t delta_ticks = delta_cycles / CYC_PER_TICK;

	last_cycle = curr_cycle;

	/* ACK match interrupt */
	TI_DM_TIMER_WRITE(1, IRQSTATUS, MAT_IT_FLAG);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Setup next match time */
		uint64_t next_cycle = curr_cycle + CYC_PER_TICK;

		TI_DM_TIMER_WRITE(next_cycle, TMAR, COMPARE_VALUE);
	}

	k_spin_unlock(&lock, key);

	sys_clock_announce(delta_ticks);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Not supported on tickful kernels */
		return;
	}

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks, 1, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&lock);

	/* Setup next match time */
	uint32_t curr_cycle = TI_DM_TIMER_READ(TCRR);
	uint32_t next_cycle = curr_cycle + (ticks * CYC_PER_TICK);

	TI_DM_TIMER_WRITE(next_cycle, TMAR, COMPARE_VALUE);

	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_cycle_get_32(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t curr_cycle = TI_DM_TIMER_READ(TCRR);

	k_spin_unlock(&lock, key);

	return curr_cycle;
}

unsigned int sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Always return 0 for tickful kernel system */
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t curr_cycle = TI_DM_TIMER_READ(TCRR);
	uint32_t delta_cycles = curr_cycle - last_cycle;
	uint32_t delta_ticks = delta_cycles / CYC_PER_TICK;

	k_spin_unlock(&lock, key);

	return delta_ticks;
}

static int sys_clock_driver_init(void)
{
	last_cycle = 0;

	IRQ_CONNECT(TIMER_IRQ_NUM, TIMER_IRQ_PRIO, ti_dmtimer_isr, NULL, TIMER_IRQ_FLAGS);

	/* Select autoreload mode */
	TI_DM_TIMER_WRITE(1, TCLR, AR);

	/* Enable match interrupt */
	TI_DM_TIMER_WRITE(1, IRQENABLE_SET, MAT_EN_FLAG);

	/* Load timer counter value */
	TI_DM_TIMER_WRITE(0, TCRR, TIMER_COUNTER);

	/* Load timer load value */
	TI_DM_TIMER_WRITE(0, TLDR, LOAD_VALUE);

	/* Load timer compare value */
	TI_DM_TIMER_WRITE(CYC_PER_TICK, TMAR, COMPARE_VALUE);

	/* Enable compare mode */
	TI_DM_TIMER_WRITE(1, TCLR, CE);

	/* Start the timer */
	TI_DM_TIMER_WRITE(1, TCLR, ST);

	irq_enable(TIMER_IRQ_NUM);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
