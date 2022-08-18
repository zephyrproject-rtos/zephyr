/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for BMI270s accessed via SPI.
 */

#include <zephyr/logging/log.h>
#include "bmi270.h"

LOG_MODULE_DECLARE(bmi270, CONFIG_SENSOR_LOG_LEVEL);

static int bmi270_bus_check_spi(const union bmi270_bus *bus)
{
	return spi_is_ready(&bus->spi) ? 0 : -ENODEV;
}

static int bmi270_reg_read_spi(const union bmi270_bus *bus,
			       uint8_t start, uint8_t *data, uint16_t len)
{
	int ret;
	uint8_t addr;
	uint8_t tmp[2];
	int i;
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

	rx_buf[0].buf = &tmp[0];
	rx_buf[0].len = 2;
	rx_buf[1].len = 1;

	for (i = 0; i < len; i++) {
		addr = (start + i) | 0x80;
		rx_buf[1].buf = &data[i];

		ret = spi_transceive_dt(&bus->spi, &tx, &rx);
		if (ret < 0) {
			LOG_DBG("spi_transceive failed %i", ret);
			return ret;
		}
	}

	k_usleep(BMI270_SPI_ACC_DELAY_US);
	return 0;
}

static int bmi270_reg_write_spi(const union bmi270_bus *bus, uint8_t start,
				const uint8_t *data, uint16_t len)
{
	int ret;
	uint8_t addr;
	const struct spi_buf tx_buf[2] = {
		{.buf = &addr, .len = sizeof(addr)},
		{.buf = (uint8_t *)data, .len = len}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf)
	};

	addr = start & BMI270_REG_MASK;

	ret = spi_write_dt(&bus->spi, &tx);
	if (ret < 0) {
		LOG_ERR("spi_write_dt failed %i", ret);
		return ret;
	}

	k_usleep(BMI270_SPI_ACC_DELAY_US);
	return 0;
}

static int bmi270_bus_init_spi(const union bmi270_bus *bus)
{
	uint8_t tmp;

	/* Single read of SPI initializes the chip to SPI mode
	 */
	return bmi270_reg_read_spi(bus, BMI270_REG_CHIP_ID, &tmp, 1);
}

const struct bmi270_bus_io bmi270_bus_io_spi = {
	.check = bmi270_bus_check_spi,
	.read = bmi270_reg_read_spi,
	.write = bmi270_reg_write_spi,
	.init = bmi270_bus_init_spi,
};
