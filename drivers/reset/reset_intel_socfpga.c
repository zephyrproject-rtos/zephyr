/*
 * Copyright (c) 2022, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_socfpga_reset

#include <zephyr/device.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/kernel.h>

struct reset_intel_config {
	DEVICE_MMIO_ROM;
	uint8_t reg_width;
	uint8_t active_low;
	uintptr_t base_address;
};

struct reset_intel_soc_data {
	DEVICE_MMIO_RAM;
};

static int reset_intel_soc_read_register(const struct device *dev, uint16_t offset, uint32_t *value)
{
	const struct reset_intel_config *config = (const struct reset_intel_config *)dev->config;
	mem_addr_t base_address = config->base_address;

	switch (config->reg_width) {
	case 1:
		*value = sys_read8(base_address + offset);
		break;
	case 2:
		*value = sys_read16(base_address + offset);
		break;
	case 4:
		*value = sys_read32(base_address + offset);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int reset_intel_soc_write_register(const struct device *dev, uint16_t offset, uint32_t value)
{
	const struct reset_intel_config *config = (const struct reset_intel_config *)dev->config;
	mem_addr_t base_address = config->base_address;

	switch (config->reg_width) {
	case 1:
		sys_write8(value, base_address + offset);
		break;
	case 2:
		sys_write16(value, base_address + offset);
		break;
	case 4:
		sys_write32(value, base_address + offset);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int reset_intel_soc_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const struct reset_intel_config *config = (const struct reset_intel_config *)dev->config;
	uint16_t offset;
	uint32_t value;
	uint8_t regbit;
	int ret;

	offset = id / (config->reg_width * CHAR_BIT);
	offset = offset * (config->reg_width);
	regbit = id % (config->reg_width * CHAR_BIT);
	ret = reset_intel_soc_read_register(dev, offset, &value);
	if (ret) {
		return ret;
	}

	*status = !(value & BIT(regbit)) ^ !config->active_low;

	return ret;
}

static int reset_intel_soc_update(const struct device *dev, uint32_t id, uint8_t assert)
{
	const struct reset_intel_config *config = (const struct reset_intel_config *)dev->config;
	uint16_t offset;
	uint32_t value;
	uint8_t regbit;
	int ret;

	offset = id / (config->reg_width * CHAR_BIT);
	regbit = id % (config->reg_width * CHAR_BIT);
	offset = offset * config->reg_width;
	ret = reset_intel_soc_read_register(dev, offset, &value);
	if (ret) {
		return ret;
	}

	if (assert ^ config->active_low) {
		value |= BIT(regbit);
	} else {
		value &= ~BIT(regbit);
	}
	return reset_intel_soc_write_register(dev, offset, value);
}

static int reset_intel_soc_line_assert(const struct device *dev, uint32_t id)
{
	return reset_intel_soc_update(dev, id, 1);
}

static int reset_intel_soc_line_deassert(const struct device *dev, uint32_t id)
{
	return reset_intel_soc_update(dev, id, 0);
}

static int reset_intel_soc_line_toggle(const struct device *dev, uint32_t id)
{
	int ret;

	ret = reset_intel_soc_line_assert(dev, id);
	if (ret) {
		return ret;
	}

	return reset_intel_soc_line_deassert(dev, id);
}

static int reset_intel_soc_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	return 0;
}

static const struct reset_driver_api reset_intel_soc_driver_api = {
	.status = reset_intel_soc_status,
	.line_assert = reset_intel_soc_line_assert,
	.line_deassert = reset_intel_soc_line_deassert,
	.line_toggle = reset_intel_soc_line_toggle,
};

#define INTEL_SOC_RESET_INIT(_inst)						\
	static struct reset_intel_soc_data reset_intel_soc_data_##_inst;	\
	static const struct reset_intel_config reset_intel_config_##_inst = {	\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(_inst)),			\
		.reg_width = DT_INST_PROP_OR(_inst, reg_width, 4),		\
		.active_low = DT_INST_PROP_OR(_inst, active_low, 0),		\
		.base_address = DT_INST_REG_ADDR(_inst),			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(_inst,						\
			reset_intel_soc_init,					\
			NULL,							\
			&reset_intel_soc_data_##_inst,				\
			&reset_intel_config_##_inst,				\
			PRE_KERNEL_1,						\
			CONFIG_RESET_INIT_PRIORITY,				\
			&reset_intel_soc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INTEL_SOC_RESET_INIT);
