/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2022, Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for BME680s accessed via SPI.
 */

#include <zephyr/logging/log.h>
#include "bme680.h"

#if BME680_BUS_SPI

LOG_MODULE_DECLARE(bme680, CONFIG_SENSOR_LOG_LEVEL);

static int bme680_bus_check_spi(const union bme680_bus *bus)
{
	return spi_is_ready(&bus->spi) ? 0 : -ENODEV;
}

static inline int bme680_set_mem_page(const struct device *dev, uint8_t addr)
{
	const struct bme680_config *config = dev->config;
	struct bme680_data *data = dev->data;
	uint8_t page;
	int err = 0;

	if (addr > 0x7f) {
		page = BME680_MEM_PAGE1;
	} else {
		page = BME680_MEM_PAGE0;
	}

	if (data->mem_page != page) {

		data->mem_page = page;
		uint8_t cmd[] = { BME680_REG_MEM_PAGE & BME680_SPI_WRITE_MSK, data->mem_page };
		const struct spi_buf tx_buf = {
			.buf = cmd,
			.len = sizeof(cmd)
		};
		const struct spi_buf_set tx = {
			.buffers = &tx_buf,
			.count = 1
		};

		err = spi_write_dt(&config->bus.spi, &tx);
	}
	return err;
}

static int bme680_reg_write_spi(const struct device *dev,
				uint8_t reg, uint8_t val)
{
	const struct bme680_config *config = dev->config;
	int err;
	uint8_t cmd[] = { reg & BME680_SPI_WRITE_MSK, val };
	const struct spi_buf tx_buf = {
		.buf = cmd,
		.len = sizeof(cmd)
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	err = bme680_set_mem_page(dev, reg);
	if (err) {
		return err;
	}

	err = spi_write_dt(&config->bus.spi, &tx);

	return err;
}

static int bme680_reg_read_spi(const struct device *dev,
			       uint8_t start, uint8_t *buf, int size)
{
	const struct bme680_config *config = dev->config;
	int err;
	uint8_t addr;
	const struct spi_buf tx_buf = {
		.buf = &addr,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	struct spi_buf rx_buf[2];
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf)
	};

	rx_buf[0].buf = NULL;
	rx_buf[0].len = 1;

	addr = start | BME680_SPI_READ_BIT;
	rx_buf[1].buf = buf;
	rx_buf[1].len = size;

	err = bme680_set_mem_page(dev, start);
	if (err) {
		return err;
	}

	err = spi_transceive_dt(&config->bus.spi, &tx, &rx);

	return err;
}

const struct bme680_bus_io bme680_bus_io_spi = {
	.check = bme680_bus_check_spi,
	.read = bme680_reg_read_spi,
	.write = bme680_reg_write_spi,
};
#endif /* BME680_BUS_SPI */
