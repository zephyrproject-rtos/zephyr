/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/adxl34x.h>
#include <zephyr/logging/log.h>

#include "adxl34x_private.h"
#include "adxl34x_i2c.h"

LOG_MODULE_DECLARE(adxl34x, CONFIG_SENSOR_LOG_LEVEL);

/**
 * Initialise the I2C device
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_i2c_init(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c)) {
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
int adxl34x_i2c_write(const struct device *dev, uint8_t reg_addr, uint8_t reg_data)
{
	const struct adxl34x_dev_config *config = dev->config;
	int rc;

	rc = i2c_reg_write_byte_dt(&config->i2c, reg_addr, reg_data);
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
int adxl34x_i2c_read(const struct device *dev, uint8_t reg_addr, uint8_t *reg_data)
{
	const struct adxl34x_dev_config *config = dev->config;
	int rc;

	rc = i2c_write_read_dt(&config->i2c, &reg_addr, sizeof(reg_addr), reg_data,
			       sizeof(*reg_data));
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
int adxl34x_i2c_read_buf(const struct device *dev, uint8_t reg_addr, uint8_t *rx_buf, uint8_t size)
{
	const struct adxl34x_dev_config *config = dev->config;
	int rc;

	rc = i2c_write_read_dt(&config->i2c, &reg_addr, sizeof(reg_addr), rx_buf, size);
	return rc;
}
