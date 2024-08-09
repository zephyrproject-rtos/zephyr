/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <strings.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/adxl34x.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

#include "adxl34x_private.h"
#include "adxl34x_spi.h"
#include "adxl34x_reg.h"

LOG_MODULE_DECLARE(adxl34x, CONFIG_SENSOR_LOG_LEVEL);

/**
 * Initialise the SPI device
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_spi_init(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("Device not ready");
		return -ENODEV;
	}
	return 0;
}

/**
 * Function called when a write to the device is initiated
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] reg_addr Address of the register to write to
 * @param[in] reg_data Value to write
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_spi_write(const struct device *dev, uint8_t reg_addr, uint8_t reg_data)
{
	const struct adxl34x_dev_config *config = dev->config;
	uint8_t address = reg_addr & ~ADXL34X_SPI_MSG_READ;
	int rc;

	const struct spi_buf buf[2] = {
		/* clang-format off */
		{
			.buf = &address,
			.len = 1,
		},
		{
			.buf = &reg_data,
			.len = 1,
		}
		/* clang-format on */
	};
	const struct spi_buf_set tx = {
		.buffers = buf,
		.count = 2,
	};

	rc = spi_write_dt(&config->spi, &tx);
	return rc;
}

/**
 * Function called when a read of a single register from the device is initiated
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] reg_addr Address of the register to read from
 * @param[out] reg_data Pointer to store the value read
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_spi_read(const struct device *dev, uint8_t reg_addr, uint8_t *reg_data)
{
	int rc;

	rc = adxl34x_spi_read_buf(dev, reg_addr, reg_data, sizeof(*reg_data));
	return rc;
}

/**
 * Function called when a read of multiple registers from the device is initiated
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] reg_addr Address of the register to read from
 * @param[out] rx_buf Pointer to store the data read
 * @param[in] size Size of the @p rx_buf buffer in bytes
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_spi_read_buf(const struct device *dev, uint8_t reg_addr, uint8_t *rx_buffer,
			 uint8_t size)
{
	const struct adxl34x_dev_config *config = dev->config;
	int rc;
	uint8_t address = reg_addr | ADXL34X_SPI_MSG_READ;

	if (size > 1) {
		address |= ADXL34X_SPI_MULTI_BYTE;
	}
	bzero(rx_buffer, size);

	const struct spi_buf buf[2] = {
		/* clang-format off */
		{
			.buf = &address,
			.len = 1,
		},
		{
			.buf = rx_buffer,
			.len = size,
		}
		/* clang-format on */
	};
	const struct spi_buf_set tx = {
		.buffers = buf,
		.count = 2,
	};
	const struct spi_buf_set rx = {
		.buffers = buf,
		.count = 2,
	};

	rc = spi_transceive_dt(&config->spi, &tx, &rx);
	return rc;
}
