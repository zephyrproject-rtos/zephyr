/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "adis1647x.h"

LOG_MODULE_DECLARE(adis1647x, CONFIG_SENSOR_LOG_LEVEL);

static int adis1647x_reg_access(const struct device *dev, uint8_t cmd, uint8_t addr, uint8_t data,
				uint8_t *rx)
{
	const struct adis1647x_config *cfg = dev->config;
	uint8_t addr_byte = addr | cmd;
	uint8_t data_byte = data;
	uint8_t rx_buf[2] = {0};
	int ret;

	const struct spi_buf tx_bufs[2] = {{.buf = &addr_byte, .len = 1},
					   {.buf = &data_byte, .len = 1}};
	const struct spi_buf rx_bufs[2] = {{.buf = &rx_buf[0], .len = 1},
					   {.buf = &rx_buf[1], .len = 1}};

	struct spi_buf_set tx_set = {.buffers = tx_bufs, .count = 2};
	struct spi_buf_set rx_set = {.buffers = rx_bufs, .count = 2};

	LOG_DBG("SPI TX: %02X %02X", addr_byte, data_byte);

	ret = spi_transceive_dt(&cfg->spi, &tx_set, &rx_set);
	if (ret) {
		LOG_ERR("spi_transceive_dt failed: %d", ret);
		return ret;
	}

	if (rx != NULL) {
		rx[0] = rx_buf[0];
		rx[1] = rx_buf[1];
	}

	return 0;
}

int adis1647x_reg_read(const struct device *dev, uint8_t addr, uint16_t *val)
{
	uint8_t rx[2];
	int ret;

	ret = adis1647x_reg_access(dev, ADIS1647X_READ_CMD, addr, 0x00, NULL);
	if (ret) {
		return ret;
	}

	k_busy_wait(ADIS1647X_SPI_STALL_US);

	ret = adis1647x_reg_access(dev, ADIS1647X_READ_CMD, 0x00, 0x00, rx);
	if (ret) {
		return ret;
	}

	*val = ((uint16_t)rx[0] << 8) | rx[1];
	return 0;
}

int adis1647x_reg_write_no_verify(const struct device *dev, uint8_t addr, uint16_t val)
{
	const struct adis1647x_config *cfg = dev->config;
	int ret;

	/* Ensure CS is high before starting write sequence */
	spi_release_dt(&cfg->spi);
	k_busy_wait(ADIS1647X_SPI_STALL_US);

	/* Write low byte */
	ret = adis1647x_reg_access(dev, ADIS1647X_WRITE_CMD, addr, val & 0xFF, NULL);
	if (ret) {
		return ret;
	}

	/* Force CS high between writes */
	spi_release_dt(&cfg->spi);
	k_busy_wait(ADIS1647X_SPI_STALL_US);

	/* Write high byte */
	ret = adis1647x_reg_access(dev, ADIS1647X_WRITE_CMD, addr + 1, (val >> 8) & 0xFF, NULL);
	if (ret) {
		return ret;
	}

	spi_release_dt(&cfg->spi);
	k_busy_wait(ADIS1647X_SPI_STALL_US);

	return 0;
}

int adis1647x_reg_write(const struct device *dev, uint8_t addr, uint16_t val)
{
	const int retries = 16;
	int ret;
	uint16_t readback;

	LOG_DBG("reg_write: addr=0x%02X val=0x%04X (low=0x%02X high=0x%02X)", addr, val, val & 0xFF,
		(val >> 8) & 0xFF);

	for (int i = 0; i < retries; i++) {
		ret = adis1647x_reg_write_no_verify(dev, addr, val);
		if (ret) {
			continue;
		}

		/* Verify write */
		ret = adis1647x_reg_read(dev, addr, &readback);
		if (ret == 0 && readback == val) {
			return 0;
		}

		LOG_DBG("reg_write retry %d: wrote=0x%04X read=0x%04X", i, val, readback);
	}

	LOG_ERR("reg_write failed after %d retries: addr=0x%02X val=0x%04X", retries, addr, val);
	return -EIO;
}

int adis1647x_burst_read(const struct device *dev, struct adis1647x_burst_data *data)
{
	const struct adis1647x_config *cfg = dev->config;
	int ret;

	uint8_t tx_data[sizeof(struct adis1647x_burst_data)] = {0};

	tx_data[0] = ADIS1647X_BURST_CMD;
	tx_data[1] = 0x00;

	struct spi_buf tx_buf = {.buf = tx_data, .len = sizeof(tx_data)};
	struct spi_buf rx_buf = {.buf = data, .len = sizeof(struct adis1647x_burst_data)};
	struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};

	ret = spi_transceive_dt(&cfg->spi, &tx_set, &rx_set);
	if (ret) {
		LOG_ERR("Burst read SPI error: %d", ret);
		return ret;
	}

	return 0;
}
