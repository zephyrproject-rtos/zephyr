/*
 * Copyright (c) 2026 Kim Bøndergaard <kim@fam-boendergaard.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT waveshare_ch32v003

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/waveshare_ch32v003.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mfd_ch32v003, CONFIG_MFD_LOG_LEVEL);

struct mfd_ch32v003_config {
	struct i2c_dt_spec bus;
};

struct mfd_ch32v003_data {
	struct k_mutex lock;
};

struct k_mutex *ch32v003_get_lock(const struct device *dev)
{
	struct mfd_ch32v003_data *data = dev->data;

	return &data->lock;
}

int ch32v003_read_reg8(const struct device *dev, enum ch32v003_reg reg, uint8_t *value)
{
	const struct mfd_ch32v003_config *cfg = dev->config;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->bus, reg, value);
	if (ret < 0) {
		return -EIO;
	}

	return 0;
}

int ch32v003_write_reg8(const struct device *dev, enum ch32v003_reg reg, uint8_t val)
{
	const struct mfd_ch32v003_config *cfg = dev->config;
	int ret;

	ret = i2c_reg_write_byte_dt(&cfg->bus, reg, val);
	if (ret < 0) {
		return -EIO;
	}

	return 0;
}

int ch32v003_read_reg16(const struct device *dev, enum ch32v003_reg reg, uint16_t *value)
{
	const struct mfd_ch32v003_config *cfg = dev->config;
	int ret;
	uint8_t raw[2];

	ret = i2c_burst_read_dt(&cfg->bus, reg, raw, sizeof(raw));
	if (ret < 0) {
		return -EIO;
	}

	/* Convert little-endian raw bytes to host format */
	*value = (uint16_t)raw[0] | ((uint16_t)raw[1] << 8);
	return 0;
}

static int mfd_ch32v003_init(const struct device *dev)
{
	struct mfd_ch32v003_data *data = dev->data;
	const struct mfd_ch32v003_config *cfg = dev->config;

	if (k_mutex_init(&data->lock) < 0) {
		return -EINVAL;
	}

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C bus %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	return 0;
}

#define MFD_CH32V003_DEFINE(inst)                                                                  \
	static const struct mfd_ch32v003_config mfd_ch32v003_config_##inst = {                     \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
	static struct mfd_ch32v003_data mfd_ch32v003_data_##inst;                                 \
	DEVICE_DT_INST_DEFINE(inst, mfd_ch32v003_init, NULL, &mfd_ch32v003_data_##inst,            \
			      &mfd_ch32v003_config_##inst, POST_KERNEL,                            \
			      CONFIG_MFD_WAVESHARE_CH32V003_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_CH32V003_DEFINE)
