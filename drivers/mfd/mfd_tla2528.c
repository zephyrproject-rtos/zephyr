/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mfd/tla2528.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <stdint.h>

#define DT_DRV_COMPAT ti_tla2528

LOG_MODULE_REGISTER(TLA2528, CONFIG_MFD_LOG_LEVEL);

enum tla2528_i2c_opcodes {
	SINGLE_REG_READ = 0x10,
	SINGLE_REG_WRITE = 0x08,
	SET_BIT = 0x18,
	CLEAR_BIT = 0x20,
	BLOCK_READ = 0x30,
	BLOCK_WRITE = 0x28,
};

struct mfd_tla2528_config {
	const struct i2c_dt_spec bus;
};

struct mfd_tla2528_data {
	struct k_mutex lock;
};

static int tla2528_i2c_write(const struct device *dev, enum tla2528_i2c_opcodes opcode,
			     enum tla2528_reg reg, uint8_t data)
{
	const struct mfd_tla2528_config *conf = dev->config;
	const uint8_t buf[] = {opcode, reg, data};

	return i2c_write_dt(&conf->bus, buf, sizeof(buf));
}

static int tla2528_i2c_read(const struct device *dev, enum tla2528_i2c_opcodes opcode,
			    enum tla2528_reg reg, uint8_t *data, size_t len)
{
	const struct mfd_tla2528_config *conf = dev->config;
	const uint8_t write_buf[] = {opcode, reg};

	return i2c_write_read_dt(&conf->bus, write_buf, sizeof(write_buf), data, len);
}

int tla2528_register_write(const struct device *dev, enum tla2528_reg reg, uint8_t data)
{
	return tla2528_i2c_write(dev, SINGLE_REG_WRITE, reg, data);
}

int tla2528_register_read(const struct device *dev, enum tla2528_reg reg, uint8_t *data)
{
	return tla2528_i2c_read(dev, SINGLE_REG_READ, reg, data, 1);
}

int tla2528_register_set_bits(const struct device *dev, enum tla2528_reg reg, uint8_t bitmask)
{
	return tla2528_i2c_write(dev, SET_BIT, reg, bitmask);
}

int tla2528_register_clear_bits(const struct device *dev, enum tla2528_reg reg, uint8_t bitmask)
{
	return tla2528_i2c_write(dev, CLEAR_BIT, reg, bitmask);
}

int tla2528_read_adc_data(const struct device *dev, uint16_t *data)
{
	const struct mfd_tla2528_config *conf = dev->config;
	int ret;
	uint8_t raw[2];

	ret = i2c_read_dt(&conf->bus, raw, sizeof(raw));
	if (ret >= 0) {
		*data = sys_get_be16(raw);
	}

	return ret;
}

struct k_mutex *tla2528_get_lock(const struct device *dev)
{
	struct mfd_tla2528_data *data = dev->data;

	return &data->lock;
}

static int tla2528_init(const struct device *dev)
{
	struct mfd_tla2528_data *data = dev->data;

	return k_mutex_init(&data->lock);
}

#define MFD_TLA2528_INST_DEFINE(n)                                                                 \
	static const struct mfd_tla2528_config config##n = {.bus = I2C_DT_SPEC_INST_GET(n)};       \
	static struct mfd_tla2528_data data##n;                                                    \
	DEVICE_DT_INST_DEFINE(n, tla2528_init, NULL, &data##n, &config##n, POST_KERNEL,            \
			      CONFIG_MFD_TLA2528_INIT_PRIO, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_TLA2528_INST_DEFINE);
