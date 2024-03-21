/*
 * Copyright (c) 2024 Gustavo Silva
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sciosense_ens160

#include <zephyr/drivers/spi.h>

#include "ens160.h"

LOG_MODULE_DECLARE(ENS160, CONFIG_SENSOR_LOG_LEVEL);

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

#define ENS160_SPI_READ_BIT BIT(0)

static int ens160_read_reg_spi(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct ens160_config *config = dev->config;

	uint8_t tx_buffer = (reg << 1) | ENS160_SPI_READ_BIT;

	const struct spi_buf tx_buf = {
		.buf = &tx_buffer,
		.len = 1,
	};

	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	struct spi_buf rx_buf[2] = {
		{
			.buf = NULL,
			.len = 1,
		},
		{
			.buf = val,
			.len = 1,
		}
	};

	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 2,
	};

	return spi_transceive_dt(&config->spi, &tx, &rx);
}

static int ens160_read_data_spi(const struct device *dev, uint8_t start, uint8_t *data, size_t len)
{
	const struct ens160_config *config = dev->config;

	uint8_t tx_buffer = (start << 1) | ENS160_SPI_READ_BIT;

	const struct spi_buf tx_buf = {
		.buf = &tx_buffer,
		.len = 1,
	};

	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	struct spi_buf rx_buf[2] = {
		{
			.buf = NULL,
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

	return spi_transceive_dt(&config->spi, &tx, &rx);
}

static int ens160_write_reg_spi(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct ens160_config *config = dev->config;

	uint8_t tx_buffer = reg << 1;

	const struct spi_buf buf[2] = {
		{
			.buf = &tx_buffer,
			.len = 1,
		},
		{
			.buf = &val,
			.len = 1,
		}
	};

	const struct spi_buf_set tx = {
		.buffers = buf,
		.count = 2,
	};

	return spi_write_dt(&config->spi, &tx);
}

static int ens160_write_data_spi(const struct device *dev, uint8_t reg, uint8_t *data, size_t len)
{
	const struct ens160_config *config = dev->config;

	uint8_t tx_buffer = reg << 1;

	const struct spi_buf buf[2] = {
		{
			.buf = &tx_buffer,
			.len = 1,
		},
		{
			.buf = &data,
			.len = len,
		}
	};

	const struct spi_buf_set tx = {
		.buffers = buf,
		.count = 2,
	};

	return spi_write_dt(&config->spi, &tx);
}

const struct ens160_transfer_function ens160_spi_transfer_function = {
	.read_reg = ens160_read_reg_spi,
	.read_data = ens160_read_data_spi,
	.write_reg = ens160_write_reg_spi,
	.write_data = ens160_write_data_spi,
};

int ens160_spi_init(const struct device *dev)
{
	const struct ens160_config *config = dev->config;
	struct ens160_data *data = dev->data;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_DBG("SPI bus not ready");
		return -ENODEV;
	}

	data->tf = &ens160_spi_transfer_function;

	return 0;
}

#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
