/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for BMI260s accessed via SPI.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "bmi260.h"

LOG_MODULE_DECLARE(bmi260, CONFIG_SENSOR_LOG_LEVEL);

static int bmi260_bus_check_spi(const union bmi260_bus *bus)
{
	return spi_is_ready_dt(&bus->spi) ? 0 : -ENODEV;
}

static int bmi260_reg_read_spi(const union bmi260_bus *bus,
			       uint8_t start, uint8_t *data, uint16_t len)
{
	int ret;
	uint8_t addr;
	uint8_t tmp[2];
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

	/* First byte we read should be discarded. */
	rx_buf[0].buf = &tmp;
	rx_buf[0].len = 2;
	rx_buf[1].len = len;
	rx_buf[1].buf = data;

	addr = start | 0x80;

	ret = spi_transceive_dt(&bus->spi, &tx, &rx);
	if (ret < 0) {
		LOG_DBG("spi_transceive failed %i", ret);
		return ret;
	}

	k_usleep(BMI260_SPI_ACC_DELAY_US);
	return 0;
}

static int bmi260_reg_write_spi(const union bmi260_bus *bus, uint8_t start,
				const uint8_t *data, uint16_t len)
{
	int ret;
	uint8_t addr;
	const struct spi_buf tx_buf[2] = {
		{.buf = &addr, .len = sizeof(addr)},
		{.buf = (const uint8_t *)data, .len = len}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf)
	};

	addr = start & BMI260_REG_MASK;

	ret = spi_write_dt(&bus->spi, &tx);
	if (ret < 0) {
		LOG_ERR("spi_write_dt failed %i", ret);
		return ret;
	}

	k_usleep(BMI260_SPI_ACC_DELAY_US);
	return 0;
}

static int bmi260_bus_init_spi(const union bmi260_bus *bus)
{
	uint8_t tmp;

	/* Single read of SPI initializes the chip to SPI mode
	 */
	return bmi260_reg_read_spi(bus, BMI260_REG_CHIP_ID, &tmp, 1);
}

const struct bmi260_bus_io bmi260_bus_io_spi = {
	.check = bmi260_bus_check_spi,
	.read = bmi260_reg_read_spi,
	.write = bmi260_reg_write_spi,
	.init = bmi260_bus_init_spi,
};
