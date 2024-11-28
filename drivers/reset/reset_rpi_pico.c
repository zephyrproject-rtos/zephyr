/*
 * Copyright (c) 2022 Andrei-Edward Popa
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_reset

#include <limits.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/reset.h>

struct reset_rpi_config {
	DEVICE_MMIO_ROM;
	uint8_t reg_width;
	uint8_t active_low;
	uintptr_t base_address;
};

static int reset_rpi_read_register(const struct device *dev, uint16_t offset, uint32_t *value)
{
	const struct reset_rpi_config *config = dev->config;
	uint32_t base_address = config->base_address;

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

static int reset_rpi_write_register(const struct device *dev, uint16_t offset, uint32_t value)
{
	const struct reset_rpi_config *config = dev->config;
	uint32_t base_address = config->base_address;

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

static int reset_rpi_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const struct reset_rpi_config *config = dev->config;
	uint16_t offset;
	uint32_t value;
	uint8_t regbit;
	int ret;

	offset = id / (config->reg_width * CHAR_BIT);
	regbit = id % (config->reg_width * CHAR_BIT);

	ret = reset_rpi_read_register(dev, offset, &value);
	if (ret) {
		return ret;
	}

	*status = !(value & BIT(regbit)) ^ !config->active_low;

	return ret;
}

static int reset_rpi_update(const struct device *dev, uint32_t id, uint8_t assert)
{
	const struct reset_rpi_config *config = dev->config;
	uint16_t offset;
	uint32_t value;
	uint8_t regbit;
	int ret;

	offset = id / (config->reg_width * CHAR_BIT);
	regbit = id % (config->reg_width * CHAR_BIT);

	ret = reset_rpi_read_register(dev, offset, &value);
	if (ret) {
		return ret;
	}

	if (assert ^ config->active_low) {
		value |= BIT(regbit);
	} else {
		value &= ~BIT(regbit);
	}

	return reset_rpi_write_register(dev, offset, value);
}

static int reset_rpi_line_assert(const struct device *dev, uint32_t id)
{
	return reset_rpi_update(dev, id, 1);
}

static int reset_rpi_line_deassert(const struct device *dev, uint32_t id)
{
	return reset_rpi_update(dev, id, 0);
}

static int reset_rpi_line_toggle(const struct device *dev, uint32_t id)
{
	int ret;

	ret = reset_rpi_line_assert(dev, id);
	if (ret) {
		return ret;
	}

	return reset_rpi_line_deassert(dev, id);
}

static int reset_rpi_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	return 0;
}

static DEVICE_API(reset, reset_rpi_driver_api) = {
	.status = reset_rpi_status,
	.line_assert = reset_rpi_line_assert,
	.line_deassert = reset_rpi_line_deassert,
	.line_toggle = reset_rpi_line_toggle,
};

#define RPI_RESET_INIT(idx)							\
	static const struct reset_rpi_config reset_rpi_config_##idx = {		\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(idx)),				\
		.reg_width = DT_INST_PROP_OR(idx, reg_width, 4),		\
		.active_low = DT_INST_PROP_OR(idx, active_low, 0),		\
		.base_address = DT_INST_REG_ADDR(idx),				\
	};									\
										\
	DEVICE_DT_INST_DEFINE(idx, reset_rpi_init,				\
			      NULL, NULL,					\
			      &reset_rpi_config_##idx, PRE_KERNEL_1,		\
			      CONFIG_RESET_INIT_PRIORITY,			\
			      &reset_rpi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RPI_RESET_INIT);
