/*
 * Bosch BMA400 3-axis accelerometer driver
 * SPDX-FileCopyrightText: Copyright 2026 Luca Gessi lucagessi90@gmail.com
 * SPDX-FileCopyrightText: Copyright 2026 The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma400

#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

#include "bma400.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

LOG_MODULE_DECLARE(bma400, CONFIG_SENSOR_LOG_LEVEL);

static int bma400_spi_read_reg(const struct device *dev, uint8_t reg_addr, uint8_t *value)
{
	int ret;
	const struct bma400_config *cfg = dev->config;
	const size_t tx_len = 3;
	uint8_t tx_buf_data[3] = {0};
	uint8_t rx_buf_data[3] = {0};

	const struct spi_buf tx_buf = {.buf = tx_buf_data, .len = tx_len};
	struct spi_buf rx_buf = {.buf = rx_buf_data, .len = tx_len};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1,
	};

	tx_buf_data[0] = reg_addr | 0x80;

	ret = spi_transceive_dt(&cfg->bus_cfg.spi, &tx, &rx);
	if (ret < 0) {
		LOG_DBG("spi_transceive failed %i", ret);
		return ret;
	}

	*value = rx_buf_data[2];

	return 0;
}

static int bma400_spi_write_reg(const struct device *dev, uint8_t reg_addr, uint8_t value)
{
	int ret;
	const struct bma400_config *cfg = dev->config;
	const size_t tx_len = 2;
	uint8_t tx_buf_data[2] = {0};

	tx_buf_data[1] = value;
	const struct spi_buf tx_buf = {
		.buf = tx_buf_data,
		.len = tx_len,
	};

	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	tx_buf_data[0] = reg_addr;

	ret = spi_write_dt(&cfg->bus_cfg.spi, &tx);
	if (ret < 0) {
		LOG_ERR("spi_write_dt failed %i", ret);
		return ret;
	}
	return 0;
}

static const struct bma400_hw_operations spi_ops = {.read_reg = bma400_spi_read_reg,
						    .write_reg = bma400_spi_write_reg};

int bma400_spi_init(const struct device *dev)
{
	struct bma400_data *data = dev->data;
	const struct bma400_config *cfg = dev->config;
	uint8_t tmp;

	if (!device_is_ready(cfg->bus_cfg.spi.bus)) {
		LOG_ERR("SPI bus device is not ready");
		return -ENODEV;
	}

	data->hw_ops = &spi_ops;

	/* Single read of SPI initializes the chip to SPI mode */
	return bma400_spi_read_reg(dev, BMA400_REG_CHIP_ID, &tmp);
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
