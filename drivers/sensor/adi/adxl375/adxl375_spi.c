/* adxl375_spi.c - SPI routines for ADXL375 driver
 */

/*
 * Copyright (c) 2022 Analog Devices
 * Copyright (c) 2024 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl375

#include <string.h>
#include <zephyr/logging/log.h>

#include "adxl375.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

LOG_MODULE_DECLARE(adxl375, CONFIG_SENSOR_LOG_LEVEL);

static int adxl375_bus_access(const struct device *dev, uint8_t reg, void *data, size_t length)
{
	const struct adxl375_dev_config *config = dev->config;

	const struct spi_buf buf[2] = {{.buf = &reg, .len = 1}, {.buf = data, .len = length}};

	struct spi_buf_set tx = {
		.buffers = buf,
	};

	if (reg & ADXL375_READ) {
		const struct spi_buf_set rx = {.buffers = buf, .count = 2};

		tx.count = 1;

		return spi_transceive_dt(&config->spi, &tx, &rx);
	}

	tx.count = 2;

	return spi_write_dt(&config->spi, &tx);
}

static int adxl375_spi_reg_read(const struct device *dev, uint8_t reg_addr, uint8_t *reg_data)
{
	return adxl375_bus_access(dev, ADXL375_REG_READ(reg_addr), reg_data, 1);
}

static int adxl375_spi_reg_read_multiple(const struct device *dev, uint8_t reg_addr,
					 uint8_t *reg_data, uint16_t count)
{
	return adxl375_bus_access(dev, ADXL375_REG_READ(reg_addr), reg_data, count);
}

static int adxl375_spi_reg_write(const struct device *dev, uint8_t reg_addr, uint8_t reg_data)
{
	return adxl375_bus_access(dev, ADXL375_REG_WRITE(reg_addr), &reg_data, 1);
}

int adxl375_spi_reg_write_mask(const struct device *dev, uint8_t reg_addr, uint32_t mask,
			       uint8_t data)
{
	int ret;
	uint8_t tmp;

	ret = adxl375_spi_reg_read(dev, reg_addr, &tmp);
	if (ret) {
		return ret;
	}

	tmp &= ~mask;
	tmp |= data;

	return adxl375_spi_reg_write(dev, reg_addr, tmp);
}

static const struct adxl375_transfer_function adxl375_spi_transfer_fn = {
	.read_reg_multiple = adxl375_spi_reg_read_multiple,
	.write_reg = adxl375_spi_reg_write,
	.read_reg = adxl375_spi_reg_read,
	.write_reg_mask = adxl375_spi_reg_write_mask,
};

int adxl375_spi_init(const struct device *dev)
{
	struct adxl375_data *data = dev->data;
	const struct adxl375_dev_config *config = dev->config;

	data->hw_tf = &adxl375_spi_transfer_fn;

	if (!spi_is_ready_dt(&config->spi)) {
		return -ENODEV;
	}

	return 0;
}

#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
