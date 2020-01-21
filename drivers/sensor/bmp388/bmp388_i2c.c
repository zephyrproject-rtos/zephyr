/* Driver for Bosch BMP388 Digital pressure sensor */

/*
 * Copyright (c) 2020 Ioannis Konstantelias
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <drivers/i2c.h>
#include <logging/log.h>

#include "bmp388.h"

#ifdef DT_BOSCH_BMP388_BUS_I2C

LOG_MODULE_DECLARE(BMP388, CONFIG_SENSOR_LOG_LEVEL);

static int bmp388_i2c_read(struct device *dev, u8_t reg_addr,
			   u8_t *value, u16_t len)
{
	struct bmp388_data *data = dev->driver_data;
	const struct bmp388_config *cfg = dev->config->config_info;

	return i2c_burst_read(data->bus, cfg->i2c_slave_addr,
			      reg_addr, value, len);
}

static int bmp388_i2c_write(struct device *dev, u8_t reg_addr, u8_t value)
{
	struct bmp388_data *data = dev->driver_data;
	const struct bmp388_config *cfg = dev->config->config_info;

	return i2c_reg_write_byte(data->bus, cfg->i2c_slave_addr,
				  reg_addr, value);
}

int bmp388_i2c_init(struct device *dev)
{
	struct bmp388_data *data = dev->driver_data;

	data->reg_read = bmp388_i2c_read;
	data->reg_write = bmp388_i2c_write;

	return 0;
}
#endif /* DT_BOSCH_BMP388_BUS_I2C */
