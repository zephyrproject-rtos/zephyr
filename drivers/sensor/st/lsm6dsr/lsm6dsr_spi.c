/* lsm6dsr_spi.c - SPI routines for LSM6DSR driver
 */

/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lsm6dsr

#include <string.h>
#include <zephyr/logging/log.h>

#include "lsm6dsr.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

#define LSM6DSR_SPI_READ		(1 << 7)

LOG_MODULE_DECLARE(LSM6DSR, CONFIG_SENSOR_LOG_LEVEL);

static int lsm6dsr_raw_read(const struct device *dev, uint8_t reg_addr,
			    uint8_t *value, uint8_t len)
{
	const struct lsm6dsr_config *cfg = dev->config;
	uint8_t buffer_tx[2] = { reg_addr | LSM6DSR_SPI_READ, 0 };
	const struct spi_buf tx_buf = {
			.buf = buffer_tx,
			.len = 2,
	};
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

static int lsm6dsr_raw_write(const struct device *dev, uint8_t reg_addr,
			     uint8_t *value, uint8_t len)
{
	const struct lsm6dsr_config *cfg = dev->config;
	uint8_t buffer_tx[1] = { reg_addr & ~LSM6DSR_SPI_READ };
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

static int lsm6dsr_spi_read_data(const struct device *dev, uint8_t reg_addr,
				 uint8_t *value, uint8_t len)
{
	return lsm6dsr_raw_read(dev, reg_addr, value, len);
}

static int lsm6dsr_spi_write_data(const struct device *dev, uint8_t reg_addr,
				  uint8_t *value, uint8_t len)
{
	return lsm6dsr_raw_write(dev, reg_addr, value, len);
}

static int lsm6dsr_spi_read_reg(const struct device *dev, uint8_t reg_addr,
				uint8_t *value)
{
	return lsm6dsr_raw_read(dev, reg_addr, value, 1);
}

static int lsm6dsr_spi_update_reg(const struct device *dev, uint8_t reg_addr,
				  uint8_t mask, uint8_t value)
{
	uint8_t tmp_val;

	lsm6dsr_raw_read(dev, reg_addr, &tmp_val, 1);
	tmp_val = (tmp_val & ~mask) | (value & mask);

	return lsm6dsr_raw_write(dev, reg_addr, &tmp_val, 1);
}

static const struct lsm6dsr_transfer_function lsm6dsr_spi_transfer_fn = {
	.read_data = lsm6dsr_spi_read_data,
	.write_data = lsm6dsr_spi_write_data,
	.read_reg  = lsm6dsr_spi_read_reg,
	.update_reg = lsm6dsr_spi_update_reg,
};

int lsm6dsr_spi_init(const struct device *dev)
{
	struct lsm6dsr_data *data = dev->data;
	const struct lsm6dsr_config *cfg = dev->config;

	data->hw_tf = &lsm6dsr_spi_transfer_fn;

	if (!spi_is_ready_dt(&cfg->bus_cfg.spi)) {
		return -ENODEV;
	}

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
