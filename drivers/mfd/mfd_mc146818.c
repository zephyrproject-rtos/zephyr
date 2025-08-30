/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT motorola_mc146818_mfd

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/mfd/mc146818.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/sys_io.h>

#define RTC_STD_INDEX (DT_INST_REG_ADDR_BY_IDX(0, 0))
#define RTC_STD_TARGET (DT_INST_REG_ADDR_BY_IDX(0, 1))
#define RTC_EXT_INDEX (DT_INST_REG_ADDR_BY_IDX(0, 2))
#define RTC_EXT_TARGET (DT_INST_REG_ADDR_BY_IDX(0, 3))

struct mfd_mc146818_data {
	struct k_spinlock lock;
};

uint8_t mfd_mc146818_std_read(const struct device *dev, uint8_t offset)
{
	struct mfd_mc146818_data *data = dev->data;
	uint8_t value;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	sys_out8(offset, RTC_STD_INDEX);
	value = sys_in8(RTC_STD_TARGET);

	k_spin_unlock(&data->lock, key);
	return value;
}

void mfd_mc146818_std_write(const struct device *dev, uint8_t offset, uint8_t value)
{
	struct mfd_mc146818_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	sys_out8(offset, RTC_STD_INDEX);
	sys_out8(value, RTC_STD_TARGET);

	k_spin_unlock(&data->lock, key);
}

uint8_t mfd_mc146818_ext_read(const struct device *dev, uint8_t offset)
{
	struct mfd_mc146818_data *data = dev->data;
	uint8_t value;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	sys_out8(offset, RTC_EXT_INDEX);
	value = sys_in8(RTC_EXT_TARGET);

	k_spin_unlock(&data->lock, key);
	return value;
}

void mfd_mc146818_ext_write(const struct device *dev, uint8_t offset, uint8_t value)
{
	struct mfd_mc146818_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	sys_out8(offset, RTC_EXT_INDEX);
	sys_out8(value, RTC_EXT_TARGET);

	k_spin_unlock(&data->lock, key);
}

#define MFD_MC146818_DEFINE(inst)						\
	static struct mfd_mc146818_data data##inst;				\
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &data##inst, NULL,		\
			      POST_KERNEL,					\
			      CONFIG_MFD_MOTOROLA_MC146818_INIT_PRIORITY,	\
			      NULL);						\

DT_INST_FOREACH_STATUS_OKAY(MFD_MC146818_DEFINE)
