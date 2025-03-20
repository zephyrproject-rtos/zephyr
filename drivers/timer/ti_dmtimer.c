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

#define TIMER_IRQ_NUM   DT_INST_IRQN(0)
#define TIMER_IRQ_PRIO  DT_INST_IRQ(0, priority)
#define TIMER_IRQ_FLAGS DT_INST_IRQ(0, flags)

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = TIMER_IRQ_NUM;
#endif

#define CYC_PER_TICK ((uint32_t)(sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC))

#define MAX_TICKS ((k_ticks_t)(UINT32_MAX / CYC_PER_TICK) - 1)

#define DEV_CFG(_dev)  ((const struct ti_dm_timer_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct ti_dm_timer_data *)(_dev)->data)

struct ti_dm_timer_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
};

struct ti_dm_timer_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	struct k_spinlock lock;
	uint32_t last_cycle;
};

static const struct device *systick_timer_dev;

#define TI_DM_TIMER_MASK(reg)  TI_DM_TIMER_##reg##_MASK
#define TI_DM_TIMER_SHIFT(reg) TI_DM_TIMER_##reg##_SHIFT

#define TI_DM_TIMER_READ(dev, reg) sys_read32(DEVICE_MMIO_GET(dev) + TI_DM_TIMER_##reg)
#define TI_DM_TIMER_WRITE(dev, data, reg, bits)                                                    \
	ti_dm_timer_write_masks(data, DEVICE_MMIO_GET(dev) + TI_DM_TIMER_##reg,                    \
				TI_DM_TIMER_MASK(reg##_##bits), TI_DM_TIMER_SHIFT(reg##_##bits))

static void ti_dm_timer_write_masks(uint32_t data, uint32_t reg, uint32_t mask, uint32_t shift)
{
	uint32_t reg_val;

	reg_val = sys_read32(reg);
	reg_val = (reg_val & ~(mask)) | (data << shift);
	sys_write32(reg_val, reg);
}

static void ti_dmtimer_isr(void *param)
{
	ARG_UNUSED(param);

	struct ti_dm_timer_data *data = systick_timer_dev->data;

	/* If no pending event */
	if (!TI_DM_TIMER_READ(systick_timer_dev, IRQSTATUS)) {
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	uint32_t curr_cycle = TI_DM_TIMER_READ(systick_timer_dev, TCRR);
	uint32_t delta_cycles = curr_cycle - data->last_cycle;
	uint32_t delta_ticks = delta_cycles / CYC_PER_TICK;

	data->last_cycle = curr_cycle;

	/* ACK match interrupt */
	TI_DM_TIMER_WRITE(systick_timer_dev, 1, IRQSTATUS, MAT_IT_FLAG);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Setup next match time */
		uint64_t next_cycle = curr_cycle + CYC_PER_TICK;

		TI_DM_TIMER_WRITE(systick_timer_dev, next_cycle, TMAR, COMPARE_VALUE);
	}

	k_spin_unlock(&data->lock, key);

	sys_clock_announce(delta_ticks);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	struct ti_dm_timer_data *data = systick_timer_dev->data;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Not supported on tickful kernels */
		return;
	}

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks, 1, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* Setup next match time */
	uint32_t curr_cycle = TI_DM_TIMER_READ(systick_timer_dev, TCRR);
	uint32_t next_cycle = curr_cycle + (ticks * CYC_PER_TICK);

	TI_DM_TIMER_WRITE(systick_timer_dev, next_cycle, TMAR, COMPARE_VALUE);

	k_spin_unlock(&data->lock, key);
}

uint32_t sys_clock_cycle_get_32(void)
{
	struct ti_dm_timer_data *data = systick_timer_dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	uint32_t curr_cycle = TI_DM_TIMER_READ(systick_timer_dev, TCRR);

	k_spin_unlock(&data->lock, key);

	return curr_cycle;
}

unsigned int sys_clock_elapsed(void)
{
	struct ti_dm_timer_data *data = systick_timer_dev->data;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Always return 0 for tickful kernel system */
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	uint32_t curr_cycle = TI_DM_TIMER_READ(systick_timer_dev, TCRR);
	uint32_t delta_cycles = curr_cycle - data->last_cycle;
	uint32_t delta_ticks = delta_cycles / CYC_PER_TICK;

	k_spin_unlock(&data->lock, key);

	return delta_ticks;
}

static int sys_clock_driver_init(void)
{
	struct ti_dm_timer_data *data;

	systick_timer_dev = DEVICE_DT_GET(DT_NODELABEL(systick_timer));

	data = systick_timer_dev->data;

	data->last_cycle = 0;

	DEVICE_MMIO_NAMED_MAP(systick_timer_dev, reg_base, K_MEM_CACHE_NONE);

	IRQ_CONNECT(TIMER_IRQ_NUM, TIMER_IRQ_PRIO, ti_dmtimer_isr, NULL, TIMER_IRQ_FLAGS);

	/* Disable prescalar */
	TI_DM_TIMER_WRITE(systick_timer_dev, 0, TCLR, PRE);

	/* Select autoreload mode */
	TI_DM_TIMER_WRITE(systick_timer_dev, 1, TCLR, AR);

	/* Enable match interrupt */
	TI_DM_TIMER_WRITE(systick_timer_dev, 1, IRQENABLE_SET, MAT_EN_FLAG);

	/* Load timer counter value */
	TI_DM_TIMER_WRITE(systick_timer_dev, 0, TCRR, TIMER_COUNTER);

	/* Load timer load value */
	TI_DM_TIMER_WRITE(systick_timer_dev, 0, TLDR, LOAD_VALUE);

	/* Load timer compare value */
	TI_DM_TIMER_WRITE(systick_timer_dev, CYC_PER_TICK, TMAR, COMPARE_VALUE);

	/* Enable compare mode */
	TI_DM_TIMER_WRITE(systick_timer_dev, 1, TCLR, CE);

	/* Start the timer */
	TI_DM_TIMER_WRITE(systick_timer_dev, 1, TCLR, ST);

	irq_enable(TIMER_IRQ_NUM);

	return 0;
}

#define TI_DM_TIMER(n)                                                                             \
	static struct ti_dm_timer_data ti_dm_timer_data_##n;                                       \
	static const struct ti_dm_timer_config ti_dm_timer_config_##n = {                          \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),                              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &ti_dm_timer_data_##n, &ti_dm_timer_config_##n,       \
			      PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(TI_DM_TIMER);

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
