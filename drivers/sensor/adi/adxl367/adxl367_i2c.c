/* adxl367_i2c.c - I2C routines for ADXL367 driver
 */

/*
 * Copyright (c) 2023 Analog Devices
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/logging/log.h>

#include "adxl367.h"

#ifdef ADXL367_BUS_I2C

LOG_MODULE_DECLARE(ADXL367, CONFIG_SENSOR_LOG_LEVEL);

static int adxl367_bus_access(const struct device *dev, uint8_t reg,
			      void *data, size_t length)
{
	const struct adxl367_dev_config *config = dev->config;

	if ((reg & ADXL367_READ) != 0) {
		return i2c_burst_read_dt(&config->i2c,
					 ADXL367_TO_REG(reg),
					 (uint8_t *) data, length);
	} else {
		if (length != 1) {
			return -EINVAL;
		}

		return i2c_reg_write_byte_dt(&config->i2c,
					     ADXL367_TO_REG(reg),
					     *(uint8_t *)data);
	}
}

static int adxl367_i2c_reg_read(const struct device *dev, uint8_t reg_addr,
			    uint8_t *reg_data)
{
	return adxl367_bus_access(dev, ADXL367_REG_READ(reg_addr), reg_data, 1);
}

static int adxl367_i2c_reg_read_multiple(const struct device *dev,
					 uint8_t reg_addr,
					 uint8_t *reg_data,
					 uint16_t count)
{
	return adxl367_bus_access(dev, ADXL367_REG_READ(reg_addr),
				  reg_data, count);
}

static int adxl367_i2c_reg_write(const struct device *dev,
				 uint8_t reg_addr,
				 uint8_t reg_data)
{
	return adxl367_bus_access(dev, ADXL367_REG_WRITE(reg_addr),
				  &reg_data, 1);
}


int adxl367_i2c_reg_write_mask(const struct device *dev,
			       uint8_t reg_addr,
			       uint32_t mask,
			       uint8_t data)
{
	int ret;
	uint8_t tmp;

	ret = adxl367_i2c_reg_read(dev, reg_addr, &tmp);
	if (ret != 0) {
		return ret;
	}

	tmp &= ~mask;
	tmp |= data;

	return adxl367_i2c_reg_write(dev, reg_addr, tmp);
}

static const struct adxl367_transfer_function adxl367_i2c_transfer_fn = {
	.read_reg_multiple = adxl367_i2c_reg_read_multiple,
	.write_reg = adxl367_i2c_reg_write,
	.read_reg  = adxl367_i2c_reg_read,
	.write_reg_mask = adxl367_i2c_reg_write_mask,
};

int adxl367_i2c_init(const struct device *dev)
{
	struct adxl367_data *data = dev->data;
	const struct adxl367_dev_config *config = dev->config;

	data->hw_tf = &adxl367_i2c_transfer_fn;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	return 0;
}
#endif /* ADXL367_BUS_I2C */
