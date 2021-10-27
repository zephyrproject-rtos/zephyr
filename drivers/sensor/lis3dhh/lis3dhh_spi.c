/*
 * Copyright (C) NEXPLORE
 * https://www.nexplore.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis3dhh

#include <string.h>
#include "lis3dhh.h"
#include <logging/log.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

LOG_MODULE_DECLARE(lis3dhh, CONFIG_SENSOR_LOG_LEVEL);

#define LIS3DHH_SPI_READ_BIT BIT(7)

static int lis3dhh_raw_read(const struct device *dev, uint8_t reg_addr,
			    uint8_t *value, uint8_t len)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	const struct lis3dhh_config *cfg = dev->config;
	const struct spi_config *spi_cfg = &cfg->bus_cfg.spi_cfg->spi_conf;
	uint8_t buffer_tx[2] = { reg_addr | LIS3DHH_SPI_READ_BIT, 0 };

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

	if (spi_transceive(lis3dhh_drv_data->bus, spi_cfg, &tx, &rx)) {
		return -EIO;
	}

	return 0;
}

static int lis3dhh_raw_write(const struct device *dev, uint8_t reg_addr,
			     uint8_t *value, uint8_t len)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	const struct lis3dhh_config *cfg = dev->config;
	const struct spi_config *spi_cfg = &cfg->bus_cfg.spi_cfg->spi_conf;
	uint8_t buffer_tx[2] = { reg_addr & ~LIS3DHH_SPI_READ_BIT };

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

	if (spi_write(lis3dhh_drv_data->bus, spi_cfg, &tx)) {
		return -EIO;
	}

	return 0;
}

static int lis3dhh_spi_read_data(const struct device *dev, uint8_t reg_addr,
				 uint8_t *value, uint8_t len)
{
	return lis3dhh_raw_read(dev, reg_addr, value, len);
}

static int lis3dhh_spi_write_data(const struct device *dev, uint8_t reg_addr,
				  uint8_t *value, uint8_t len)
{
	return lis3dhh_raw_write(dev, reg_addr, value, len);
}

static int lis3dhh_spi_read_reg(const struct device *dev, uint8_t reg_addr,
				uint8_t *value)
{
	return lis3dhh_raw_read(dev, reg_addr, value, 1);
}

static int lis3dhh_spi_write_reg(const struct device *dev, uint8_t reg_addr,
				 uint8_t value)
{
	uint8_t tmp_val = value;

	return lis3dhh_raw_write(dev, reg_addr, &tmp_val, 1);
}

static int lis3dhh_spi_update_reg(const struct device *dev, uint8_t reg_addr,
				  uint8_t mask, uint8_t value)
{
	uint8_t tmp_val;

	lis3dhh_raw_read(dev, reg_addr, &tmp_val, 1);
	tmp_val = (tmp_val & ~mask) | (value & mask);
	return lis3dhh_raw_write(dev, reg_addr, &tmp_val, 1);
}

/**
 * @brief translate between functions
 *
 */
static const struct lis3dhh_transfer_function lis3dhh_spi_transfter_fn = {
	.read_data = lis3dhh_spi_read_data,
	.write_data = lis3dhh_spi_write_data,
	.read_reg = lis3dhh_spi_read_reg,
	.write_reg = lis3dhh_spi_write_reg,
	.update_reg = lis3dhh_spi_update_reg,
};

int lis3dhh_spi_init(const struct device *dev)
{
	struct lis3dhh_data *lis3dhh_drv_data = dev->data;
	const struct lis3dhh_config *cfg = dev->config;
	const struct lis3dhh_spi_cfg *spi_cfg = cfg->bus_cfg.spi_cfg;

	lis3dhh_drv_data->hw_tf = &lis3dhh_spi_transfter_fn;

	if (spi_cfg->cs_gpios_label != NULL) {
		lis3dhh_drv_data->cs_ctrl.gpio_dev = device_get_binding(spi_cfg->
									cs_gpios_label);
		if (!lis3dhh_drv_data->cs_ctrl.gpio_dev) {
			LOG_ERR("Unable to get chip-select GPIO.");
			return -ENODEV;
		}
		LOG_DBG("SPI chip-select GPIO configured on %s: %u", spi_cfg->
			cs_gpios_label, lis3dhh_drv_data->cs_ctrl.gpio_pin);
	} else {
		LOG_ERR("Unable to find chip-select GPIO.");
	}

	return 0;
}

#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
