/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/counter.h>
#include <soc.h>
#include <counter/counter_ace_v1x_art_regs.h>

static struct k_spinlock lock;

static void counter_ace_v1x_art_ionte_set(bool new_timestamp_enable)
{
	uint32_t val;

	val = sys_read32(ACE_TSCTRL);
	val &= ~ACE_TSCTRL_IONTE_MASK;
	val |= FIELD_PREP(ACE_TSCTRL_IONTE_MASK, new_timestamp_enable);
	sys_write32(val, ACE_TSCTRL);
}

static void counter_ace_v1x_art_cdmas_set(uint32_t cdmas)
{
	uint32_t val;

	val = sys_read32(ACE_TSCTRL);
	val &= ~ACE_TSCTRL_CDMAS_MASK;
	val |= FIELD_PREP(ACE_TSCTRL_CDMAS_MASK, cdmas);
	sys_write32(val, ACE_TSCTRL);
}

static void counter_ace_v1x_art_ntk_set(bool new_timestamp_taken)
{
	uint32_t val;

	val = sys_read32(ACE_TSCTRL);
	val &= ~ACE_TSCTRL_NTK_MASK;
	val |= FIELD_PREP(ACE_TSCTRL_NTK_MASK, new_timestamp_taken);
	sys_write32(val, ACE_TSCTRL);
}

static uint32_t counter_ace_v1x_art_ntk_get(void)
{
	return FIELD_GET(ACE_TSCTRL_NTK_MASK, sys_read32(ACE_TSCTRL));
}

static void counter_ace_v1x_art_hhtse_set(bool enable)
{
	uint32_t val;

	val = sys_read32(ACE_TSCTRL);
	val &= ~ACE_TSCTRL_HHTSE_MASK;
	val |= FIELD_PREP(ACE_TSCTRL_HHTSE_MASK, enable);
	sys_write32(val, ACE_TSCTRL);
}

static uint64_t counter_ace_v1x_art_counter_get(void)
{
	uint32_t hi0, lo, hi1;

	do {
		hi0 = sys_read32(ACE_ARTCS_HI);
		lo = sys_read32(ACE_ARTCS_LO);
		hi1 = sys_read32(ACE_ARTCS_HI);
	} while (hi0 != hi1);

	return (((uint64_t)hi1) << 32) | lo;
}

int counter_ace_v1x_art_get_value(const struct device *dev, uint64_t *value)
{
	ARG_UNUSED(dev);

	k_spinlock_key_t key = k_spin_lock(&lock);

	counter_ace_v1x_art_ionte_set(1);
	counter_ace_v1x_art_cdmas_set(1);

	if (counter_ace_v1x_art_ntk_get()) {
		counter_ace_v1x_art_ntk_set(1);
		while (counter_ace_v1x_art_ntk_get()) {
			k_busy_wait(10);
		}
	}

	counter_ace_v1x_art_hhtse_set(1);

	while (!counter_ace_v1x_art_ntk_get()) {
		k_busy_wait(10);
	}

	*value = counter_ace_v1x_art_counter_get();

	counter_ace_v1x_art_ntk_set(1);
	k_spin_unlock(&lock, key);

	return 0;
}

int counter_ace_v1x_art_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static const struct counter_driver_api ace_v1x_art_counter_apis = {
	.get_value_64 = counter_ace_v1x_art_get_value
};

DEVICE_DT_DEFINE(DT_NODELABEL(ace_art_counter), counter_ace_v1x_art_init, NULL, NULL, NULL,
		 PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		 &ace_v1x_art_counter_apis);
