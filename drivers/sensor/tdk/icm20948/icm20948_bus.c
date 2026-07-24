/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "icm20948.h"
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>

static int icm20948_bus_read(const struct device *dev, uint8_t reg, uint8_t *val, uint8_t len)
{
	const struct icm20948_config *cfg = dev->config;

#if defined(CONFIG_I2C)
	if (cfg->bus_type == ICM20948_BUS_I2C) {
		return i2c_burst_read_dt(&cfg->i2c, reg, val, len);
	}
#endif
#if defined(CONFIG_SPI)
	if (cfg->bus_type == ICM20948_BUS_SPI) {
		uint8_t addr = reg | 0x80; /* Set read bit */
		const struct spi_buf tx_buf = {.buf = &addr, .len = 1};
		const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
		const struct spi_buf rx_buf[2] = {
			{.buf = NULL, .len = 1}, /* Skip byte during TX */
			{.buf = val, .len = len},
		};
		const struct spi_buf_set rx = {.buffers = rx_buf, .count = 2};

		return spi_transceive_dt(&cfg->spi, &tx, &rx);
	}
#endif
	return -ENOTSUP;
}

static int icm20948_bus_write(const struct device *dev, uint8_t reg, uint8_t *val, uint8_t len)
{
	const struct icm20948_config *cfg = dev->config;

#if defined(CONFIG_I2C)
	if (cfg->bus_type == ICM20948_BUS_I2C) {
		return i2c_burst_write_dt(&cfg->i2c, reg, val, len);
	}
#endif
#if defined(CONFIG_SPI)
	if (cfg->bus_type == ICM20948_BUS_SPI) {
		uint8_t addr = reg & 0x7F; /* Clear read bit */
		const struct spi_buf tx_buf[2] = {
			{.buf = &addr, .len = 1},
			{.buf = val, .len = len},
		};
		const struct spi_buf_set tx = {.buffers = tx_buf, .count = 2};

		return spi_write_dt(&cfg->spi, &tx);
	}
#endif
	return -ENOTSUP;
}

int icm20948_set_bank(const struct device *dev, uint16_t reg)
{
	/*
	 * REG_BANK_SEL expects bank number in bits [5:4]:
	 * Bank 0 = 0x00, Bank 1 = 0x10, Bank 2 = 0x20, Bank 3 = 0x30
	 * Our reg encoding has bank in upper byte (0x0300 = Bank 3, reg 0x00)
	 * So shift right by 4 to get proper format: 0x0300 >> 4 = 0x30
	 */
	uint8_t bank = (reg & 0xff00) >> 4;

	return icm20948_bus_write(dev, REG_BANK_SEL, &bank, 1);
}

int icm20948_read_reg(const struct device *dev, uint16_t reg, uint8_t *val)
{
	int ret;

	ret = icm20948_set_bank(dev, reg);
	if (ret < 0) {
		return ret;
	}

	return icm20948_bus_read(dev, reg & 0xFF, val, 1);
}

int icm20948_read_block(const struct device *dev, uint16_t reg, uint8_t *buf, uint8_t len)
{
	int ret;

	ret = icm20948_set_bank(dev, reg);
	if (ret < 0) {
		return ret;
	}

	return icm20948_bus_read(dev, reg & 0xFF, buf, len);
}

int icm20948_write_reg(const struct device *dev, uint16_t reg, uint8_t val)
{
	int ret;

	ret = icm20948_set_bank(dev, reg);
	if (ret < 0) {
		return ret;
	}

	return icm20948_bus_write(dev, reg & 0xFF, &val, 1);
}

int icm20948_read_field(const struct device *dev, uint16_t reg, uint8_t mask, uint8_t *val)
{
	int ret;

	ret = icm20948_read_reg(dev, reg, val);
	if (ret < 0) {
		return ret;
	}

	*val = FIELD_GET(mask, *val);

	return 0;
}

int icm20948_write_field(const struct device *dev, uint16_t reg, uint8_t mask, uint8_t val)
{
	uint8_t tmp = 0;
	int ret;

	ret = icm20948_read_reg(dev, reg, &tmp);
	if (ret < 0) {
		return ret;
	}

	tmp &= ~mask;
	tmp |= FIELD_PREP(mask, val);

	return icm20948_write_reg(dev, reg, tmp);
}
