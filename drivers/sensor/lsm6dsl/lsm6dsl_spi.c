/* lsm6dsl_spi.c - SPI routines for LSM6DSL driver
 */

/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <drivers/spi.h>
#include <logging/log.h>

#include "lsm6dsl.h"

#define LSM6DSL_SPI_READ		(1 << 7)

LOG_MODULE_DECLARE(LSM6DSL, CONFIG_SENSOR_LOG_LEVEL);

static int lsm6dsl_raw_read(struct device *dev, uint8_t reg_addr,
			    uint8_t *value, uint8_t len)
{
	struct lsm6dsl_data *data = dev->driver_data;
	const struct lsm6dsl_config *config = dev->config_info;
	const struct spi_config *spi_cfg = config->spi.spi_conf;
	uint8_t buffer_tx[2] = { reg_addr | LSM6DSL_SPI_READ, 0 };
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

	if (spi_transceive(data->comm_master, spi_cfg, &tx, &rx)) {
		return -EIO;
	}

	return 0;
}

static int lsm6dsl_raw_write(struct device *dev, uint8_t reg_addr,
			     uint8_t *value, uint8_t len)
{
	struct lsm6dsl_data *data = dev->driver_data;
	const struct lsm6dsl_config *config = dev->config_info;
	const struct spi_config *spi_cfg = config->spi.spi_conf;
	uint8_t buffer_tx[1] = { reg_addr & ~LSM6DSL_SPI_READ };
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

	if (spi_write(data->comm_master, spi_cfg, &tx)) {
		return -EIO;
	}

	return 0;
}

static int lsm6dsl_spi_read_data(struct device *dev, uint8_t reg_addr,
				 uint8_t *value, uint8_t len)
{
	return lsm6dsl_raw_read(dev, reg_addr, value, len);
}

static int lsm6dsl_spi_write_data(struct device *dev, uint8_t reg_addr,
				  uint8_t *value, uint8_t len)
{
	return lsm6dsl_raw_write(dev, reg_addr, value, len);
}

static int lsm6dsl_spi_read_reg(struct device *dev, uint8_t reg_addr,
				uint8_t *value)
{
	return lsm6dsl_raw_read(dev, reg_addr, value, 1);
}

static int lsm6dsl_spi_update_reg(struct device *dev, uint8_t reg_addr,
				  uint8_t mask, uint8_t value)
{
	uint8_t tmp_val;

	lsm6dsl_raw_read(dev, reg_addr, &tmp_val, 1);
	tmp_val = (tmp_val & ~mask) | (value & mask);

	return lsm6dsl_raw_write(dev, reg_addr, &tmp_val, 1);
}

static const struct lsm6dsl_transfer_function lsm6dsl_spi_transfer_fn = {
	.read_data = lsm6dsl_spi_read_data,
	.write_data = lsm6dsl_spi_write_data,
	.read_reg  = lsm6dsl_spi_read_reg,
	.update_reg = lsm6dsl_spi_update_reg,
};

int lsm6dsl_spi_init(struct device *dev)
{
	struct lsm6dsl_data *data = dev->driver_data;
	const struct lsm6dsl_config *config = dev->config_info;

	data->hw_tf = &lsm6dsl_spi_transfer_fn;

	if (config->spi.gpio_name != NULL) {
		/* The config is placed in RAM by LSM6DSL_SPI_CS_CONFIG */
		struct spi_cs_control *cs =
			(struct spi_cs_control *)config->spi.spi_conf->cs;
		cs->gpio_dev = device_get_binding(config->spi.gpio_name);
		if (!cs->gpio_dev) {
			LOG_ERR("Unable to get GPIO SPI CS device");
			return -ENODEV;
		}
	}

	return 0;
}
