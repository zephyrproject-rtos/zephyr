/*
 * Copyright (c) 2024, Vitrolife A/S
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>

#include <zephyr/drivers/mfd/ad559x.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util_macro.h>

#include "mfd_ad559x.h"

static int mfd_ad559x_i2c_read_raw(const struct device *dev, uint8_t *val, size_t len)
{
	const struct mfd_ad559x_config *config = dev->config;

	return i2c_read_dt(&config->i2c, val, len);
}

static int mfd_ad559x_i2c_write_raw(const struct device *dev, uint8_t *val, size_t len)
{
	const struct mfd_ad559x_config *config = dev->config;

	return i2c_write_dt(&config->i2c, val, len);
}

static int mfd_ad559x_i2c_read_reg(const struct device *dev, uint8_t reg, uint8_t reg_data,
				   uint16_t *val)
{
	const struct mfd_ad559x_config *config = dev->config;
	uint8_t buf[sizeof(*val)];
	int ret;

	ARG_UNUSED(reg_data);

	__ASSERT((reg & 0xf0) == 0, "reg bits [7:4] should be 0: 0x%x", reg);

	ret = i2c_write_read_dt(&config->i2c, &reg, sizeof(reg), buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	*val = sys_get_be16(buf);

	return 0;
}

static int mfd_ad559x_i2c_write_reg(const struct device *dev, uint8_t reg, uint16_t val)
{
	uint8_t buf[sizeof(reg) + sizeof(val)];

	buf[0] = reg;
	sys_put_be16(val, &buf[1]);

	return mfd_ad559x_i2c_write_raw(dev, buf, sizeof(buf));
}

static const struct mfd_ad559x_transfer_function mfd_ad559x_i2c_transfer_function = {
	.read_raw = mfd_ad559x_i2c_read_raw,
	.write_raw = mfd_ad559x_i2c_write_raw,
	.read_reg = mfd_ad559x_i2c_read_reg,
	.write_reg = mfd_ad559x_i2c_write_reg,
};

int mfd_ad559x_i2c_init(const struct device *dev)
{
	const struct mfd_ad559x_config *config = dev->config;
	struct mfd_ad559x_data *data = dev->data;

	data->transfer_function = &mfd_ad559x_i2c_transfer_function;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	return 0;
}
