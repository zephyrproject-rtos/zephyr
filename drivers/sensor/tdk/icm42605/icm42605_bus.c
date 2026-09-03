/*
 * Copyright 2026 Ahmed Ashraf NourEldeen <a.programmer55559@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include "icm42605_bus.h"

LOG_MODULE_DECLARE(ICM42605, CONFIG_SENSOR_LOG_LEVEL);

static int icm42605_i2c_single_write(const struct i2c_dt_spec *bus, uint8_t reg, uint8_t *data)
{
	uint8_t buf[2] = {reg, *data};

	return i2c_write_dt(bus, buf, sizeof(buf));
}

static int icm42605_i2c_read(const struct i2c_dt_spec *bus, uint8_t reg, uint8_t *data, size_t len)
{
	return i2c_write_read_dt(bus, &reg, 1, data, len);
}

static int icm42605_spi_single_write(const struct spi_dt_spec *bus, uint8_t reg, uint8_t *data)
{
	const struct spi_buf buf[2] = {{
					    	.buf = &reg,
					    	.len = 1,
						},
						{
					    	.buf = data,
					    	.len = 1,
						}};
	const struct spi_buf_set tx = {
		.buffers = buf,
		.count = 2,
	};

	return spi_write_dt(bus, &tx);
}

static int icm42605_spi_read(const struct spi_dt_spec *bus, uint8_t reg, uint8_t *data, size_t len)
{

	unsigned char tx_buffer[2] = {
		0,
	};

	tx_buffer[0] = 0x80 | reg;

	const struct spi_buf tx_buf = {
		.buf = tx_buffer,
		.len = 1,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	struct spi_buf rx_buf[2] = {{
						.buf = tx_buffer,
						.len = 1,
					},
					{
						.buf = data,
						.len = len,
				    }};

	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 2,
	};

	return spi_transceive_dt(bus, &tx, &rx);
}
int icm42605_reg_read(const struct icm42605_config *cfg, uint8_t reg, uint8_t *data, size_t len)
{
	switch (cfg->bus_type) {
	case ICM42605_BUS_SPI:
		return icm42605_spi_read(&cfg->bus_cfg.spi, reg, data, len);

	case ICM42605_BUS_I2C:
		return icm42605_i2c_read(&cfg->bus_cfg.i2c, reg, data, len);

	default:
		return -EINVAL;
	}
}

int icm42605_reg_write(const struct icm42605_config *cfg, uint8_t reg, uint8_t *data)
{
	switch (cfg->bus_type) {
	case ICM42605_BUS_SPI:
		return icm42605_spi_single_write(&cfg->bus_cfg.spi, reg, data);

	case ICM42605_BUS_I2C:
		return icm42605_i2c_single_write(&cfg->bus_cfg.i2c, reg, data);

	default:
		return -EINVAL;
	}
}
