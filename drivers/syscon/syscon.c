/*
 * Copyright (c) Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT syscon

#include <errno.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/drivers/syscon.h>

#include "syscon_common.h"

struct syscon_generic_config {
	DEVICE_MMIO_ROM;
	uint8_t reg_width;
};

struct syscon_generic_data {
	DEVICE_MMIO_RAM;
	size_t size;
};

static int syscon_generic_get_base(const struct device *dev, uintptr_t *addr)
{
	if (!dev) {
		return -ENODEV;
	}

	*addr = DEVICE_MMIO_GET(dev);
	return 0;
}

static int syscon_generic_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct syscon_generic_config *config;
	struct syscon_generic_data *data;
	uintptr_t base_address;

	if (!dev) {
		return -ENODEV;
	}

	data = dev->data;
	config = dev->config;

	if (!val) {
		return -EINVAL;
	}

	if (syscon_sanitize_reg(&reg, data->size, config->reg_width)) {
		return -EINVAL;
	}

	base_address = DEVICE_MMIO_GET(dev);

	switch (config->reg_width) {
	case 1:
		*val = sys_read8(base_address + reg);
		break;
	case 2:
		*val = sys_read16(base_address + reg);
		break;
	case 4:
		*val = sys_read32(base_address + reg);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int syscon_generic_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct syscon_generic_config *config;
	struct syscon_generic_data *data;
	uintptr_t base_address;

	if (!dev) {
		return -ENODEV;
	}

	data = dev->data;
	config = dev->config;

	if (syscon_sanitize_reg(&reg, data->size, config->reg_width)) {
		return -EINVAL;
	}

	base_address = DEVICE_MMIO_GET(dev);

	switch (config->reg_width) {
	case 1:
		sys_write8(val, (base_address + reg));
		break;
	case 2:
		sys_write16(val, (base_address + reg));
		break;
	case 4:
		sys_write32(val, (base_address + reg));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int syscon_generic_get_size(const struct device *dev, size_t *size)
{
	struct syscon_generic_data *data = dev->data;

	*size = data->size;
	return 0;
}

static DEVICE_API(syscon, syscon_generic_driver_api) = {
	.read = syscon_generic_read_reg,
	.write = syscon_generic_write_reg,
	.get_base = syscon_generic_get_base,
	.get_size = syscon_generic_get_size,
};

static int syscon_generic_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	return 0;
}

#define SYSCON_INIT(inst)                                                                          \
	static const struct syscon_generic_config syscon_generic_config_##inst = {                 \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),                                           \
		.reg_width = DT_INST_PROP_OR(inst, reg_io_width, 4),                               \
	};                                                                                         \
	static struct syscon_generic_data syscon_generic_data_##inst = {                           \
		.size = DT_INST_REG_SIZE(inst),                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, syscon_generic_init, NULL, &syscon_generic_data_##inst,        \
			      &syscon_generic_config_##inst, PRE_KERNEL_1,                         \
			      CONFIG_SYSCON_INIT_PRIORITY, &syscon_generic_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SYSCON_INIT);
