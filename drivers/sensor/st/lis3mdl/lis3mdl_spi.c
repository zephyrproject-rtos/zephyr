/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* adapted from  lsm6dsl_spi.c - SPI routines for LSM6DSL driver
 */
#define DT_DRV_COMPAT st_lis3mdl_magn

#include <string.h>
#include <zephyr/logging/log.h>

#include "lis3mdl.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

#define LIS3MDL_SPI_READ		(1 << 7)

LOG_MODULE_DECLARE(LIS3MDL, CONFIG_SENSOR_LOG_LEVEL);

static int lis3mdl_raw_read(const struct device *dev, uint8_t reg_addr,
			    uint8_t *value, uint8_t len)
{
	const struct lis3mdl_config *cfg = dev->config;
	uint8_t buffer_tx[2] = { reg_addr | LIS3MDL_SPI_READ, 0 };
	const struct spi_buf tx_buf = {
			.buf = buffer_tx,
			.len = 2,
	};
	if (len > 1){
		// Set ms bit to auto increment address to be read
		buffer_tx[0] |= BIT(6);
	}
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	const struct spi_buf rx_buf[2] = {
		{
			.buf = NULL,
			.len = 1,
		},
		{
			.buf = value,
			.len = len,
		}
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 2
	};


	if (len > 64) {
		return -EIO;
	}

	if (spi_transceive_dt(&cfg->bus_cfg.spi, &tx, &rx)) {
		return -EIO;
	}

	return 0;
}

static int lis3mdl_raw_write(const struct device *dev, uint8_t reg_addr,
			     uint8_t *value, uint8_t len)
{
	const struct lis3mdl_config *cfg = dev->config;
	uint8_t buffer_tx[1] = { reg_addr & ~LIS3MDL_SPI_READ };
	if (len > 1){
		// Set ms bit to auto increment address to be read
		buffer_tx[0] |= BIT(6);
	}
	const struct spi_buf tx_buf[2] = {
		{
			.buf = buffer_tx,
			.len = 1,
		},
		{
			.buf = value,
			.len = len,
		}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = 2
	};


	if (len > 64) {
		return -EIO;
	}

	if (spi_write_dt(&cfg->bus_cfg.spi, &tx)) {
		return -EIO;
	}

	return 0;
}

static int lis3mdl_spi_read_data(const struct device *dev, uint8_t reg_addr,
				 uint8_t *value, uint8_t len)
{
	return lis3mdl_raw_read(dev, reg_addr, value, len);
}

static int lis3mdl_spi_write_data(const struct device *dev, uint8_t reg_addr,
				  uint8_t *value, uint8_t len)
{
	return lis3mdl_raw_write(dev, reg_addr, value, len);
}

static int lis3mdl_spi_read_reg(const struct device *dev, uint8_t reg_addr,
				uint8_t *value)
{
	return lis3mdl_raw_read(dev, reg_addr, value, 1);
}

static int lis3mdl_spi_update_reg(const struct device *dev, uint8_t reg_addr,
				  uint8_t mask, uint8_t value)
{
	uint8_t tmp_val;

	lis3mdl_raw_read(dev, reg_addr, &tmp_val, 1);
	tmp_val = (tmp_val & ~mask) | (value & mask);

	return lis3mdl_raw_write(dev, reg_addr, &tmp_val, 1);
}

static const struct lis3mdl_transfer_function lis3mdl_spi_transfer_fn = {
	.read_data = lis3mdl_spi_read_data,
	.write_data = lis3mdl_spi_write_data,
	.read_reg  = lis3mdl_spi_read_reg,
	.update_reg = lis3mdl_spi_update_reg,
};

int lis3mdl_spi_init(const struct device *dev)
{
	struct lis3mdl_data *data = dev->data;
	const struct lis3mdl_config *cfg = dev->config;

	data->hw_tf = &lis3mdl_spi_transfer_fn;

	if (!spi_is_ready_dt(&cfg->bus_cfg.spi)) {
		return -ENODEV;
	}

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */