/* Copyright (C) 2023 BeagleBoard.org Foundation
 * Copyright (C) 2023 S Prashanth
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_am654_dmtimer

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

#include <zephyr/drivers/timer/ti_dmtimer.h>

#define CYC_PER_TICK ((uint32_t)(sys_clock_hw_cycles_per_sec() \
				/ CONFIG_SYS_CLOCK_TICKS_PER_SEC))

#define MAX_TICKS \
	((k_ticks_t)(TI_DM_TIMER_TCRR_TIMER_COUNTER_MASK / CYC_PER_TICK) - 1)

static struct k_spinlock lock;

static uint32_t last_cycle;
static uint32_t last_tick;
static uint32_t last_elapsed;

void ti_dmtimer_isr(void *data)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t curr_cycle = sys_read32(TI_DM_TIMER_TCRR);
	uint32_t delta_cycles = curr_cycle - last_cycle;
	uint32_t delta_ticks = delta_cycles / CYC_PER_TICK;

	last_cycle += (delta_ticks * CYC_PER_TICK);
	last_tick += delta_ticks;
	last_elapsed = 0;

	uint32_t next_cycle = last_cycle + CYC_PER_TICK;

	sys_write32(next_cycle, TI_DM_TIMER_TMAR);
	sys_write32(1 << TI_DM_TIMER_IRQENABLE_SET_MAT_EN_FLAG_SHIFT,
			TI_DM_TIMER_IRQSTATUS);

	k_spin_unlock(&lock, key);

	sys_clock_announce(delta_ticks);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);
#ifdef CONFIG_TICKLESS_KERNEL
	ticks = ticks == K_TICKS_FOREVER ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t curr_cycle = sys_read32(TI_DM_TIMER_TCRR);
	uint32_t next_cycle = curr_cycle + (ticks * CYC_PER_TICK);

	sys_write32(next_cycle, TI_DM_TIMER_TMAR);

	k_spin_unlock(&lock, key);
#endif /* CONFIG_TICKLESS_KERNEL */
}

uint32_t sys_clock_cycle_get_32(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t tcrr_count = sys_read32(TI_DM_TIMER_TCRR);

	k_spin_unlock(&lock, key);

	return tcrr_count;
}

unsigned int sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t curr_cycle = sys_read32(TI_DM_TIMER_TCRR);
	uint32_t delta_cycles = curr_cycle - last_cycle;
	uint32_t delta_ticks = delta_cycles / CYC_PER_TICK;

	last_elapsed = delta_ticks;

	k_spin_unlock(&lock, key);

	return delta_ticks;
}

static int sys_clock_driver_init(void)
{
	last_cycle = 0;
	last_tick = 0;
	last_elapsed = 0;

	IRQ_CONNECT(TI_DM_TIMER_IRQ_NUM, TI_DM_TIMER_IRQ_PRIO,
		ti_dmtimer_isr, NULL, TI_DM_TIMER_IRQ_FLAGS);

	sys_write32((1 << TI_DM_TIMER_IRQENABLE_SET_MAT_EN_FLAG_SHIFT),
			TI_DM_TIMER_IRQENABLE_SET);

	sys_write32(0, TI_DM_TIMER_TLDR);
	sys_write32(CYC_PER_TICK, TI_DM_TIMER_TMAR);
	sys_write32(0, TI_DM_TIMER_TCRR);

	sys_write32(0, TI_DM_TIMER_TPIR);
	sys_write32(0, TI_DM_TIMER_TNIR);

	sys_write32((1 << TI_DM_TIMER_TCLR_AR_SHIFT) |
		(1 << TI_DM_TIMER_TCLR_CE_SHIFT), TI_DM_TIMER_TCLR);

	sys_write32(sys_read32(TI_DM_TIMER_TCLR) |
		(1 << TI_DM_TIMER_TCLR_ST_SHIFT), TI_DM_TIMER_TCLR);

	irq_enable(TI_DM_TIMER_IRQ_NUM);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
