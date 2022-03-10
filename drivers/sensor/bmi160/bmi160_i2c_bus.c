/* Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bmi160.h"

#ifdef BMI160_BUS_I2C
static bool bmi160_bus_ready_i2c(const struct device *dev)
{
	const struct bmi160_cfg *cfg = dev->config;

	return device_is_ready(cfg->bus.i2c.bus);
}

static int bmi160_read_i2c(const struct device *dev, uint8_t reg_addr, void *buf, uint8_t len)
{
	const struct bmi160_cfg *cfg = dev->config;

	return i2c_burst_read_dt(&cfg->bus.i2c, reg_addr, buf, len);
}

static int bmi160_write_i2c(const struct device *dev, uint8_t reg_addr, void *buf, uint8_t len)
{
	const struct bmi160_cfg *cfg = dev->config;

	return i2c_burst_write_dt(&cfg->bus.i2c, reg_addr, buf, len);
}

const struct bmi160_bus_io bmi160_bus_i2c_io = {
	.ready = bmi160_bus_ready_i2c,
	.read = bmi160_read_i2c,
	.write = bmi160_write_i2c,
};
#endif /* BMI160_BUS_I2C */
