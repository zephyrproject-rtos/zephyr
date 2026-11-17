/*
 * Copyright (c) 2026 Deere & Company
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/logging/log.h>

#include "adxl382.h"

LOG_MODULE_DECLARE(adxl382, CONFIG_SENSOR_LOG_LEVEL);

static int adxl382_spi_read_reg_multiple(const struct device *dev, uint8_t reg,
					 uint8_t *values, uint16_t count)
{
	const struct adxl382_dev_config *config = dev->config;
	uint8_t tx[ADXL382_MAX_READ_SIZE + 1] = {ADXL382_REG_READ(reg)};
	uint8_t rx[sizeof(tx)];
	struct spi_buf tx_buf = {.buf = tx, .len = count + 1U};
	struct spi_buf rx_buf = {.buf = rx, .len = count + 1U};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};
	int ret;

	if (count == 0U || count > ADXL382_MAX_READ_SIZE) {
		LOG_ERR("Invalid SPI read length: %u", count);
		return -EINVAL;
	}
	ret = spi_transceive_dt(&config->spi, &tx_set, &rx_set);
	if (ret == 0) {
		memcpy(values, rx + 1, count);
	} else {
		LOG_ERR("SPI read 0x%02x failed: %d", reg, ret);
	}
	return ret;
}

static int adxl382_spi_read_reg(const struct device *dev, uint8_t reg, uint8_t *value)
{
	return adxl382_spi_read_reg_multiple(dev, reg, value, 1);
}

static int adxl382_spi_write_reg(const struct device *dev, uint8_t reg, uint8_t value)
{
	const struct adxl382_dev_config *config = dev->config;
	uint8_t tx[] = {ADXL382_REG_WRITE(reg), value};
	uint8_t rx[sizeof(tx)];
	struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	struct spi_buf rx_buf = {.buf = rx, .len = sizeof(rx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};
	/* QSPI TX-only completion can precede the final clock; drain RX too. */
	int ret = spi_transceive_dt(&config->spi, &tx_set, &rx_set);

	if (ret != 0) {
		LOG_ERR("SPI write 0x%02x failed: %d", reg, ret);
	}
	return ret;
}

static const struct adxl382_transfer_function adxl382_spi_transfer_fn = {
	.read_reg_multiple = adxl382_spi_read_reg_multiple,
	.read_reg = adxl382_spi_read_reg,
	.write_reg = adxl382_spi_write_reg,
};

int adxl382_spi_init(const struct device *dev)
{
	const struct adxl382_dev_config *config = dev->config;
	struct adxl382_data *data = dev->data;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI controller not ready");
		return -ENODEV;
	}
	data->hw_tf = &adxl382_spi_transfer_fn;
	return 0;
}
