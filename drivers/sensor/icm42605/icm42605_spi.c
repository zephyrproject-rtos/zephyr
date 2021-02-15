/*
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/spi.h>
#include <logging/log.h>
#include <sys/__assert.h>

LOG_MODULE_DECLARE(ICM42605, CONFIG_SENSOR_LOG_LEVEL);

static const struct device *spi_dev;
static const struct spi_config *spi_cfg;

int inv_spi_single_write(uint8_t reg, uint8_t *data)
{
	int result;

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

	result = spi_write(spi_dev, spi_cfg, &tx);

	if (result) {
		return result;
	}

	return 0;
}

int inv_spi_read(uint8_t reg, uint8_t *data, size_t len)
{
	int result;
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

	result = spi_transceive(spi_dev, spi_cfg, &tx, &rx);

	if (result) {
		return result;
	}

	return 0;
}

int icm42605_spi_init(const struct device *spi_device,
		      const struct spi_config *spi_config)
{
	__ASSERT_NO_MSG(spi_device);
	__ASSERT_NO_MSG(spi_config);

	spi_dev = spi_device;
	spi_cfg = spi_config;

	return 0;
}
