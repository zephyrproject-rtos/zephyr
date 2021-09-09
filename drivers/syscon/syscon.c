/*
 * Copyright (c) Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT syscon

#include <sys/util.h>
#include <device.h>
#include <init.h>

#include <drivers/syscon.h>

static const struct device *syscon_dev;

struct syscon_generic_config {
	DEVICE_MMIO_ROM;
};

static struct syscon_generic_config syscon_generic_config_0 = {
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(0)),
};

struct syscon_generic_data {
	DEVICE_MMIO_RAM;
	size_t size;
};

static struct syscon_generic_data syscon_generic_data_0;

uintptr_t syscon_generic_get_base(void)
{
	if (!syscon_dev) {
		return -ENODEV;
	}

	return DEVICE_MMIO_GET(syscon_dev);
}

static int sanitize_reg(uint16_t *reg, size_t reg_size)
{
	/* Avoid unaligned readings */
	*reg = ROUND_DOWN(*reg, sizeof(uint32_t));

	/* Check for out-of-bounds readings */
	if (*reg >= reg_size) {
		return -EINVAL;
	}

	return 0;
}

int syscon_generic_read_reg(uint16_t reg, uint32_t *val)
{
	struct syscon_generic_data *data;
	uintptr_t base_address;

	if (!syscon_dev) {
		return -ENODEV;
	}

	data = syscon_dev->data;

	if (!val) {
		return -EINVAL;
	}

	if (sanitize_reg(&reg, data->size)) {
		return -EINVAL;
	}

	base_address = DEVICE_MMIO_GET(syscon_dev);

	*val = sys_read32(base_address + reg);

	return 0;
}

int syscon_generic_write_reg(uint16_t reg, uint32_t val)
{
	struct syscon_generic_data *data;
	uintptr_t base_address;

	if (!syscon_dev) {
		return -ENODEV;
	}

	data = syscon_dev->data;

	if (sanitize_reg(&reg, data->size)) {
		return -EINVAL;
	}

	base_address = DEVICE_MMIO_GET(syscon_dev);

	sys_write32(val, (base_address + reg));

	return 0;
}

int syscon_generic_init(const struct device *dev)
{
	struct syscon_generic_data *data = dev->data;

	syscon_dev = dev;

	DEVICE_MMIO_MAP(syscon_dev, K_MEM_CACHE_NONE);

	data->size = DT_REG_SIZE(DT_DRV_INST(0));

	return 0;
}

DEVICE_DT_INST_DEFINE(0, syscon_generic_init, NULL, &syscon_generic_data_0,
		&syscon_generic_config_0, PRE_KERNEL_1,
		CONFIG_SYSCON_GENERIC_INIT_PRIORITY_DEVICE, NULL);
