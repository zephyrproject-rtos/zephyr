/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT puya_py32_rcc_rctl

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/irq.h>

#define PY32_RESET_SET_OFFSET(id) (((id) >> 5U) & 0xFFFU)
#define PY32_RESET_REG_BIT(id)    ((id) & 0x1FU)

struct reset_py32_config {
	uintptr_t base;
};

static bool reset_py32_valid(uint32_t id)
{
	uint32_t offset = id >> 5U;

	return (offset >= 0x24U) && (offset <= 0x30U) && ((offset % 4U) == 0U);
}

static int reset_py32_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const struct reset_py32_config *config = dev->config;

	if (!reset_py32_valid(id)) {
		return -EINVAL;
	}

	*status = !!sys_test_bit(config->base + PY32_RESET_SET_OFFSET(id), PY32_RESET_REG_BIT(id));

	return 0;
}

static int reset_py32_line_assert(const struct device *dev, uint32_t id)
{
	const struct reset_py32_config *config = dev->config;

	unsigned int key;

	if (!reset_py32_valid(id)) {
		return -EINVAL;
	}
	key = irq_lock();
	sys_set_bit(config->base + PY32_RESET_SET_OFFSET(id), PY32_RESET_REG_BIT(id));
	/* Complete the RCC write before the peripheral can be accessed. */
	(void)sys_read32(config->base + PY32_RESET_SET_OFFSET(id));
	irq_unlock(key);

	return 0;
}

static int reset_py32_line_deassert(const struct device *dev, uint32_t id)
{
	const struct reset_py32_config *config = dev->config;

	unsigned int key;

	if (!reset_py32_valid(id)) {
		return -EINVAL;
	}
	key = irq_lock();
	sys_clear_bit(config->base + PY32_RESET_SET_OFFSET(id), PY32_RESET_REG_BIT(id));
	/* Complete the RCC write before the peripheral can be accessed. */
	(void)sys_read32(config->base + PY32_RESET_SET_OFFSET(id));
	irq_unlock(key);

	return 0;
}

static int reset_py32_line_toggle(const struct device *dev, uint32_t id)
{
	int ret = reset_py32_line_assert(dev, id);

	if (ret < 0) {
		return ret;
	}
	return reset_py32_line_deassert(dev, id);
}

static DEVICE_API(reset, reset_py32_driver_api) = {
	.status = reset_py32_status,
	.line_assert = reset_py32_line_assert,
	.line_deassert = reset_py32_line_deassert,
	.line_toggle = reset_py32_line_toggle,
};

static const struct reset_py32_config reset_py32_config = {
	.base = DT_REG_ADDR(DT_INST_PARENT(0)),
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &reset_py32_config, PRE_KERNEL_1,
		      CONFIG_RESET_INIT_PRIORITY, &reset_py32_driver_api);
