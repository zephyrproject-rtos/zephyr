/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb1200_bbram

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/drivers/bbram.h>
#include <zephyr/logging/log.h>
#include <reg/vbat.h>

struct bbram_kb1200_config {
	struct vbat_regs *vbat_ptr;
};

struct bbram_kb1200_data {
	uint32_t status;
};

static int bbram_kb1200_check_invalid(const struct device *dev)
{
	const struct bbram_kb1200_config *config = dev->config;
	struct bbram_kb1200_data *data = dev->data;
	struct vbat_regs *vbat = config->vbat_ptr;

	data->status = vbat->BKUPSTS;
	/* Clear the bit(s) */
	vbat->BKUPSTS = BBRAM_STATUS_IBBR;
	return (data->status & BBRAM_STATUS_IBBR);
}

static int bbram_kb1200_check_standby_power(const struct device *dev)
{
	const struct bbram_kb1200_config *config = dev->config;
	struct bbram_kb1200_data *data = dev->data;
	struct vbat_regs *vbat = config->vbat_ptr;

	data->status = vbat->BKUPSTS;
	/* Clear the bit(s) */
	vbat->BKUPSTS = BBRAM_STATUS_VCC0;
	return (data->status & BBRAM_STATUS_VCC0);
}

static int bbram_kb1200_check_power(const struct device *dev)
{
	const struct bbram_kb1200_config *config = dev->config;
	struct bbram_kb1200_data *data = dev->data;
	struct vbat_regs *vbat = config->vbat_ptr;

	data->status = vbat->BKUPSTS;
	/* Clear the bit(s) */
	vbat->BKUPSTS = BBRAM_STATUS_VCC;
	return (data->status & BBRAM_STATUS_VCC);
}

static int bbram_kb1200_get_size(const struct device *dev, size_t *size)
{
	*size = KB1200_BBRAM_SIZE;
	return 0;
}

static int bbram_kb1200_read(const struct device *dev, size_t offset, size_t size, uint8_t *data)
{
	const struct bbram_kb1200_config *config = dev->config;
	struct vbat_regs *vbat = config->vbat_ptr;

	if (bbram_kb1200_check_invalid(dev)) {
		return -EINVAL;
	}

	if (((offset + size) > KB1200_BBRAM_SIZE) || (size < 1)) {
		return -ERANGE;
	}
	memcpy((void *)data, (void *)((uint8_t *)&vbat->PASCR + offset), size);
	return 0;
}

static int bbram_kb1200_write(const struct device *dev, size_t offset, size_t size,
			      const uint8_t *data)
{
	const struct bbram_kb1200_config *config = dev->config;
	struct vbat_regs *vbat = config->vbat_ptr;

	if (bbram_kb1200_check_invalid(dev)) {
		return -EINVAL;
	}

	if (((offset + size) > KB1200_BBRAM_SIZE) || (size < 1)) {
		return -ERANGE;
	}
	memcpy((void *)((uint8_t *)&vbat->PASCR + offset), (void *)data, size);
	return 0;
}

static const struct bbram_driver_api bbram_kb1200_driver_api = {
	.check_invalid = bbram_kb1200_check_invalid,
	.check_standby_power = bbram_kb1200_check_standby_power,
	.check_power = bbram_kb1200_check_power,
	.get_size = bbram_kb1200_get_size,
	.read = bbram_kb1200_read,
	.write = bbram_kb1200_write,
};

#define BBRAM_KB1200_DEVICE(inst)                                                                  \
	static struct bbram_kb1200_data bbram_data_##inst = {};                                    \
	static const struct bbram_kb1200_config bbram_cfg_##inst = {                               \
		.vbat_ptr = (struct vbat_regs *)DT_INST_REG_ADDR(inst),                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &bbram_data_##inst, &bbram_cfg_##inst,             \
			      PRE_KERNEL_1, CONFIG_BBRAM_INIT_PRIORITY, &bbram_kb1200_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BBRAM_KB1200_DEVICE);
