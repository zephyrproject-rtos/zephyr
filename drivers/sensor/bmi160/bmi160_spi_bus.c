/* Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bmi160.h"

#ifdef BMI160_BUS_SPI
static int bmi160_transceive(const struct device *dev, uint8_t reg, bool write, void *buf,
			     size_t length)
{
	const struct bmi160_cfg *cfg = dev->config;
	const struct spi_buf tx_buf[2] = { { .buf = &reg, .len = 1 },
					   { .buf = buf, .len = length } };
	const struct spi_buf_set tx = { .buffers = tx_buf, .count = buf ? 2 : 1 };

	if (!write) {
		const struct spi_buf_set rx = { .buffers = tx_buf, .count = 2 };

		return spi_transceive_dt(&cfg->bus.spi, &tx, &rx);
	}

	return spi_write_dt(&cfg->bus.spi, &tx);
}

static bool bmi160_bus_ready_spi(const struct device *dev)
{
	const struct bmi160_cfg *cfg = dev->config;

	return spi_is_ready(&cfg->bus.spi);
}

static int bmi160_read_spi(const struct device *dev, uint8_t reg_addr, void *buf, uint8_t len)
{
	return bmi160_transceive(dev, reg_addr | BMI160_REG_READ, false, buf, len);
}

static int bmi160_write_spi(const struct device *dev, uint8_t reg_addr, void *buf, uint8_t len)
{
	return bmi160_transceive(dev, reg_addr & BMI160_REG_MASK, true, buf, len);
}

const struct bmi160_bus_io bmi160_bus_spi_io = {
	.ready = bmi160_bus_ready_spi,
	.read = bmi160_read_spi,
	.write = bmi160_write_spi,
};
#endif /* BMI160_BUS_SPI */
