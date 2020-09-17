/*
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/spi.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(ICM42605, CONFIG_SENSOR_LOG_LEVEL);

struct device *spi_dev;
static struct spi_config *spi_cfg;

int inv_spi_single_write(uint8_t reg, uint8_t *data)
{
	int err;

	const struct spi_buf buf[2] = {
		{
			.buf = &reg,
			.len = 1,
		},
		{
			.buf = data,
			.len = 1,
		}
	};
	const struct spi_buf_set tx = {
		.buffers = buf,
		.count = 2,
	};

	err = spi_write(spi_dev, spi_cfg, &tx);

	if (err) {
		LOG_ERR("SPI error: %d", err);
		return -EIO;
	}

	return 0;
}

int inv_spi_read(uint8_t reg, uint8_t *data, size_t len)
{
	int err;
	unsigned char tx_buffer[2] = { 0, };

	tx_buffer[0] = 0x80 | reg;

	const struct spi_buf tx_buf = {
		.buf = tx_buffer,
		.len = 1,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	struct spi_buf rx_buf[2] = {
		{
			.buf = tx_buffer,
			.len = 1,
		},
		{
			.buf = data,
			.len = len,
		}
	};

	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 2,
	};

	err = spi_transceive(spi_dev, spi_cfg, &tx, &rx);

	if (err) {
		LOG_ERR("SPI error: %d", err);
		return -EIO;
	}

	return 0;
}

int icm42605_spi_init(struct device *spi_device, struct spi_config *spi_config)
{
	spi_dev = spi_device;

	if (!spi_dev) {
		LOG_ERR("Cannot get SPI bus information");
		return -EINVAL;
	}

	spi_cfg = spi_config;

	return 0;
}
