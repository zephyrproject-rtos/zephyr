/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Dotcom IoT LLP
 * Author: Mahendra Sondagar <mahendra@dotcom.co.in>
 * Author: Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 */
/**
 * @file ams_as7341.c
 * @brief AS7341 Spectral Sensor Module driver.
 *
 * Implements the Zephyr sensor driver API for the AS7341.
 * Two-pass SMUX configuration is used to read all 8 spectral filters
 * plus the clear and NIR channels over I2C.
 */

#include "ams_as7341.h"

#define DT_DRV_COMPAT ams_as7341

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ams_as7341, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Poll the AS7341 STATUS2 register until spectral data is valid.
 *
 * Reads the AVALID flag (STATUS2 bit 6) in a polling loop, sleeping 2 ms
 * between each attempt.  The loop runs at most 250 iterations (~500 ms
 * total) before giving up.
 *
 * @param dev Pointer to the sensor device structure.
 *
 * @retval 0          Data is ready (AVALID set).
 * @retval -EIO       An I2C read error occurred.
 * @retval -ETIMEDOUT AVALID was not set within the polling timeout.
 */
static int as7341_wait_data_ready(const struct device *dev)
{
	const struct as7341_config *cfg = dev->config;
	uint8_t status;
	int timeout = 250;

	while (timeout--) {
		if (i2c_reg_read_byte_dt(&cfg->i2c, AS7341_REG_STATUS2, &status) < 0) {
			LOG_ERR("Failed to set time");
			return -EIO;
		}

		if (status & BIT(6)) {
			return 0;
		}

		k_msleep(2);
	}

	return -ETIMEDOUT;
}

/**
 * @brief Enable or disable the AS7341 spectral measurement engine.
 *
 * Writes the SP_EN bit (ENABLE register bit 1).  Spectral measurement
 * must be disabled before reconfiguring the SMUX and re-enabled
 * afterwards to start a new integration cycle.
 *
 * @param dev Pointer to the sensor device structure.
 * @param en  @c true  – enable spectral measurement.
 *            @c false – disable spectral measurement.
 *
 * @return 0 on success, or a negative errno code on I2C failure.
 */
static int as7341_enable_spectral(const struct device *dev, bool en)
{
	const struct as7341_config *cfg = dev->config;

	return i2c_reg_update_byte_dt(&cfg->i2c, AS7341_REG_ENABLE, BIT(1), en ? BIT(1) : 0);
}

/**
 * @brief Set a runtime attribute on the AS7341.
 *
 * Maps Zephyr sensor attributes to AS7341 hardware registers:
 *
 * | Zephyr attribute              | AS7341 register | Description              |
 * |-------------------------------|-----------------|--------------------------|
 * | SENSOR_ATTR_GAIN              | CFG1            | Spectral gain (0–10)     |
 * | SENSOR_ATTR_SAMPLING_FREQUENCY| ATIME           | Integration time steps   |
 * | SENSOR_ATTR_RESOLUTION        | ASTEP_L/ASTEP_H | Integration step size    |
 *
 * The integer part of @p val (@c val->val1) is used for all attributes.
 * The channel parameter @p chan is ignored; the setting applies globally.
 *
 * @param dev  Pointer to the sensor device structure.
 * @param chan Sensor channel (ignored – applies to all channels).
 * @param attr Attribute to set; see table above.
 * @param val  Pointer to the sensor_value carrying the new setting.
 *
 * @retval 0        Attribute written successfully.
 * @retval -ENOTSUP Attribute is not supported by this driver.
 * @retval <0       Negative errno code on I2C write failure.
 */
static int as7341_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct as7341_config *cfg = dev->config;
	struct as7341_data *data = dev->data;
	int ret;

	switch (attr) {
	case SENSOR_ATTR_GAIN:
		data->gain = (uint8_t)val->val1;
		ret = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_CFG1, data->gain);
		if (ret < 0) {
			LOG_ERR("Failed to set gain");
			return ret;
		}
		return 0;

	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		data->atime = (uint16_t)val->val1;
		ret = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_ATIME, (uint8_t)data->atime);
		if (ret < 0) {
			LOG_ERR("Failed to set ATIME");
			return ret;
		}
		return 0;

	case SENSOR_ATTR_RESOLUTION:
		data->astep = (uint16_t)val->val1;
		ret = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_ASTEP_L,
					    (uint8_t)(data->astep & 0xFF));
		if (ret < 0) {
			LOG_ERR("Failed to set ASTEP LSB");
			return ret;
		}
		ret = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_ASTEP_H,
					    (uint8_t)(data->astep >> 8));
		if (ret < 0) {
			LOG_ERR("Failed to set ASTEP MSB");
			return ret;
		}
		return 0;

	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Read a runtime attribute from the AS7341 driver cache.
 *
 * Returns the value most recently written via as7341_attr_set() or set
 * during initialisation.  No I2C transaction is performed.
 *
 * Supported attributes: SENSOR_ATTR_GAIN, SENSOR_ATTR_SAMPLING_FREQUENCY,
 * SENSOR_ATTR_RESOLUTION.  @c val->val2 is always set to 0.
 *
 * @param dev  Pointer to the sensor device structure.
 * @param chan Sensor channel (ignored).
 * @param attr Attribute to read.
 * @param val  Pointer to the sensor_value to populate.
 *
 * @retval 0 Attribute read successfully.
 * @retval -ENOTSUP Attribute is not supported by this driver.
 */
static int as7341_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
{
	struct as7341_data *data = dev->data;

	switch (attr) {
	case SENSOR_ATTR_GAIN:
		val->val1 = data->gain;
		val->val2 = 0;
		return 0;

	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		val->val1 = data->atime;
		val->val2 = 0;
		return 0;

	case SENSOR_ATTR_RESOLUTION:
		val->val1 = data->astep;
		val->val2 = 0;
		return 0;

	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Write a single byte to an AS7341 SMUX configuration register.
 *
 * Helper used by as7341_smux_config_f1f4() and as7341_smux_config_f5f8()
 * to program the SMUX routing table one byte at a time.
 *
 * @param dev Pointer to the sensor device structure.
 * @param reg SMUX register address to write.
 * @param val Byte value to write.
 *
 * @return 0 on success, or a negative errno code on I2C failure.
 */
static int as7341_smux_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct as7341_config *cfg = dev->config;

	return i2c_reg_write_byte_dt(&cfg->i2c, reg, val);
}

/**
 * @brief Write the SMUX routing table for the low spectral channels (F1–F4).
 *
 * Programs the 20-byte SMUX configuration that routes the F1 (415 nm),
 * F2 (445 nm), F3 (480 nm), and F4 (515 nm) filters, together with the
 * first clear and NIR channels, to the six ADC inputs.
 *
 * Must be called after setting the SMUX command to WRITE and before
 * calling as7341_enable_smux().
 *
 * @param dev Pointer to the sensor device structure.
 *
 * @return 0 on success, or a negative errno code if any I2C write fails.
 */
static int as7341_smux_config_f1f4(const struct device *dev)
{
	int err;

	err = as7341_smux_write(dev, 0x00, 0x30);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x01, 0x01);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x02, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x03, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x04, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x05, 0x42);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x06, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x07, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x08, 0x50);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x09, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x0A, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x0B, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x0C, 0x20);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x0D, 0x04);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x0E, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x0F, 0x30);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x10, 0x01);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x11, 0x50);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x12, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x13, 0x06);
	if (err < 0) {
		return err;
	}

	return 0;
}

/**
 * @brief Write the SMUX routing table for the high spectral channels (F5–F8).
 *
 * Programs the 20-byte SMUX configuration that routes the F5 (555 nm),
 * F6 (590 nm), F7 (630 nm), and F8 (680 nm) filters, together with the
 * second clear and NIR channels, to the six ADC inputs.
 *
 * Must be called after setting the SMUX command to WRITE and before
 * calling as7341_enable_smux().
 *
 * @param dev Pointer to the sensor device structure.
 *
 * @return 0 on success, or a negative errno code if any I2C write fails.
 */
static int as7341_smux_config_f5f8(const struct device *dev)
{
	int err;

	err = as7341_smux_write(dev, 0x00, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x01, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x02, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x03, 0x40);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x04, 0x02);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x05, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x06, 0x10);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x07, 0x03);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x08, 0x50);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x09, 0x10);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x0A, 0x03);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x0B, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x0C, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x0D, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x0E, 0x24);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x0F, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x10, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x11, 0x50);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x12, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x13, 0x06);
	if (err < 0) {
		return err;
	}

	return 0;
}

/**
 * @brief Configure SMUX for flicker detection mode.
 *
 * Programs the AS7341 SMUX registers to route the internal flicker
 * detection signal to ADC channel 5. This configuration disables all
 * spectral channel mappings and enables only the flicker detection path.
 *
 * The SMUX must be reconfigured before enabling flicker detection
 * measurements. This function writes a fixed register sequence required
 * by the device datasheet.
 *
 * @param dev Pointer to the device structure.
 *
 * @retval 0 If the configuration was successful.
 * @retval -EIO If an I2C communication error occurs during SMUX write.
 */
static int as7341_smux_config_flicker(const struct device *dev)
{
	int err;

	/* Flicker Detection SMUX config (Flicker -> ADC5) */

	err = as7341_smux_write(dev, 0x00, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x01, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x02, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x03, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x04, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x05, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x06, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x07, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x08, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x09, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x0A, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x0B, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x0C, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x0D, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x0E, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x0F, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x10, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x11, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x12, 0x00);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_write(dev, 0x13, 0x60);
	if (err < 0) {
		return err;
	}

	return 0;
}

/**
 * @brief Set the SMUX command field in the CFG6 register.
 *
 * Writes bits [4:3] of register 0xAF (CFG6) with the requested SMUX
 * command code.  The command must be set before enabling the SMUX so
 * that the hardware knows whether to load (WRITE) or execute the
 * routing table.
 *
 * @param dev Pointer to the sensor device structure.
 * @param cmd SMUX command code (e.g. AS7341_SMUX_CMD_WRITE).
 *            The value is shifted left by 3 to align with bits [4:3].
 *
 * @return 0 on success, or a negative errno code on I2C failure.
 */
static int as7341_set_smux_command(const struct device *dev, uint8_t cmd)
{
	const struct as7341_config *cfg = dev->config;

	return i2c_reg_update_byte_dt(&cfg->i2c, AS7341_REG_CFG6, AS7341_CFG6_SMUX_CMD_MASK,
				      (cmd & AS7341_SMUX_CMD_MASK) << AS7341_SMUX_CMD_SHIFT);
}

/**
 * @brief Activate the SMUX and wait for the operation to complete.
 *
 * Sets bit 4 (SMUXEN) in the ENABLE register to start the SMUX
 * operation, then polls the same bit until the hardware clears it,
 * indicating that the SMUX routing table has been loaded into the
 * ADC input multiplexers.
 *
 * The poll loop runs at most 100 iterations with a 1 ms sleep between
 * each attempt (~100 ms total timeout).
 *
 * @param dev Pointer to the sensor device structure.
 *
 * @retval 0          SMUX operation completed successfully.
 * @retval -EIO       An I2C read or write error occurred.
 * @retval -ETIMEDOUT SMUXEN bit did not clear within the polling timeout.
 */
static int as7341_enable_smux(const struct device *dev)
{
	const struct as7341_config *cfg = dev->config;
	int ret = i2c_reg_update_byte_dt(&cfg->i2c, AS7341_REG_ENABLE, BIT(4), BIT(4));

	if (ret < 0) {
		return ret;
	}

	/* Wait for SMUX operation to complete (bit 4 clears when done) */
	uint8_t reg;
	int timeout = 1000;

	while (timeout--) {
		ret = i2c_reg_read_byte_dt(&cfg->i2c, AS7341_REG_ENABLE, &reg);
		if (ret < 0) {
			return ret;
		}
		if (!(reg & BIT(4))) {
			return 0;
		}
		k_msleep(1);
	}
	return -ETIMEDOUT;
}

/**
 * @brief Fetch spectral samples from all AS7341 channels.
 *
 * Implements the Zephyr sensor_sample_fetch() API for the AS7341.
 * Because the sensor has more spectral channels than ADC inputs, two
 * sequential measurement passes are required:
 *
 * - Pass 1: Routes F1–F4 + clear + NIR (low channels) through the SMUX
 *   and reads ADC results into ch_data[0..5].
 * - Pass 2: Routes F5–F8 + clear + NIR (high channels) through the SMUX
 *   and reads ADC results into ch_data[6..11].
 *
 * Each pass follows the sequence:
 *   1. Disable spectral measurement (SP_EN = 0).
 *   2. Set SMUX command to WRITE.
 *   3. Program the SMUX routing table.
 *   4. Enable the SMUX (SMUXEN = 1) and wait for completion.
 *   5. Enable spectral measurement (SP_EN = 1).
 *   6. Wait for AVALID (data ready).
 *   7. Burst-read the six 16-bit ADC results.
 *
 * Only @c SENSOR_CHAN_ALL is accepted; per-channel fetch is not supported.
 *
 * @param dev  Pointer to the sensor device structure.
 * @param chan Must be SENSOR_CHAN_ALL.
 *
 * @retval 0        All 12 channel values fetched successfully.
 * @retval -ENOTSUP @p chan is not SENSOR_CHAN_ALL.
 * @retval <0       Negative errno code on any I2C or timeout error.
 */
static int as7341_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct as7341_data *data = dev->data;
	const struct as7341_config *cfg = dev->config;
	uint8_t buf[2];
	int err;

	if (chan == SENSOR_CHAN_AS7341_FLICKER) {
		return as7341_detect_flicker(dev, &data->flicker_hz);
	}

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	LOG_DBG("AS7341 sample fetch started");

	/* Low Channels (F1-F4 + CLEAR_0 + NIR_0) */

	/* Disable measurement and configure SMUX for low channels */
	err = as7341_enable_spectral(dev, false);
	if (err < 0) {
		return err;
	}

	err = as7341_set_smux_command(dev, AS7341_SMUX_CMD_WRITE);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_config_f1f4(dev);
	if (err < 0) {
		LOG_ERR("SMUX F1-F4 config failed");
		return err;
	}

	/* Enable SMUX */
	err = as7341_enable_smux(dev);
	if (err < 0) {
		return err;
	}

	/* Start spectral measurement */
	err = as7341_enable_spectral(dev, true);
	if (err < 0) {
		return err;
	}

	/* Wait for data ready */
	err = as7341_wait_data_ready(dev);
	if (err < 0) {
		LOG_ERR("Data ready timeout - Bank 1 (F1-F4)");
		return err;
	}

	/* Read first 6 channels */
	for (int i = 0; i < 6; i++) {
		err = i2c_burst_read_dt(&cfg->i2c, AS7341_REG_CH0_DATA_L_1 + (i * 2), buf, 2);
		if (err < 0) {
			return err;
		}
		data->ch_data[i] = ((uint16_t)buf[1] << 8) | buf[0];
	}

	/* High Channels (F5-F8 + CLEAR + NIR) */

	/* Disable measurement and configure SMUX for high channels */
	err = as7341_enable_spectral(dev, false);
	if (err < 0) {
		return err;
	}

	err = as7341_set_smux_command(dev, AS7341_SMUX_CMD_WRITE);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_config_f5f8(dev);
	if (err < 0) {
		LOG_ERR("SMUX F5-F8 config failed");
		return err;
	}

	/* Enable SMUX */
	err = as7341_enable_smux(dev);
	if (err < 0) {
		return err;
	}

	/* Start spectral measurement */
	err = as7341_enable_spectral(dev, true);
	if (err < 0) {
		return err;
	}

	/* Wait for data ready */
	err = as7341_wait_data_ready(dev);
	if (err < 0) {
		LOG_ERR("Data ready timeout - Bank 2 (F5-F8)");
		return err;
	}

	/* Read next 6 channels */
	for (int i = 0; i < 6; i++) {
		err = i2c_burst_read_dt(&cfg->i2c, AS7341_REG_CH0_DATA_L_1 + (i * 2), buf, 2);
		if (err < 0) {
			return err;
		}

		data->ch_data[i + 6] = ((uint16_t)buf[1] << 8) | buf[0];
	}

	LOG_DBG("AS7341 sample fetch completed successfully");
	return 0;
}

/**
 * @brief Get a spectral channel reading from the AS7341 driver buffer.
 *
 * Implements the Zephyr sensor_channel_get() API for the AS7341.
 * Returns the raw 16-bit ADC count most recently fetched by
 * as7341_sample_fetch() for the requested spectral channel.
 *
 * The result is stored in val->val1 as an integer count.
 * val->val2 is always set to 0 (no fractional part).
 *
 * This function must be called after sensor_sample_fetch() has
 * successfully populated the internal driver buffer. It has no
 * side effects on driver state.
 *
 * Channel-to-buffer index mapping:
 *
 * | Channel                    | Wavelength | Pass | ch_data index |
 * |----------------------------|------------|------|---------------|
 * | SENSOR_CHAN_AS7341_415NM_F1 | 415 nm     | 1    | 0             |
 * | SENSOR_CHAN_AS7341_445NM_F2 | 445 nm     | 1    | 1             |
 * | SENSOR_CHAN_AS7341_480NM_F3 | 480 nm     | 1    | 2             |
 * | SENSOR_CHAN_AS7341_515NM_F4 | 515 nm     | 1    | 3             |
 * | SENSOR_CHAN_AS7341_CLEAR_0  | Clear      | 1    | 4             |
 * | SENSOR_CHAN_AS7341_NIR_0    | NIR        | 1    | 5             |
 * | SENSOR_CHAN_AS7341_555NM_F5 | 555 nm     | 2    | 6             |
 * | SENSOR_CHAN_AS7341_590NM_F6 | 590 nm     | 2    | 7             |
 * | SENSOR_CHAN_AS7341_630NM_F7 | 630 nm     | 2    | 8             |
 * | SENSOR_CHAN_AS7341_680NM_F8 | 680 nm     | 2    | 9             |
 * | SENSOR_CHAN_AS7341_CLEAR    | Clear      | 2    | 10            |
 * | SENSOR_CHAN_AS7341_NIR      | NIR        | 2    | 11            |
 *
 * @param dev  Pointer to the sensor device structure.
 * @param chan Spectral channel to read; see table above.
 * @param val  Pointer to the sensor_value structure to populate.
 *             val->val1 receives the raw ADC count; val->val2 is set to 0.
 *
 * @retval 0        Channel value retrieved successfully.
 * @retval -ENOTSUP The requested channel is not supported by this driver.
 */
static int as7341_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct as7341_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AS7341_415NM_F1:
		val->val1 = data->ch_data[0];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_445NM_F2:
		val->val1 = data->ch_data[1];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_480NM_F3:
		val->val1 = data->ch_data[2];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_515NM_F4:
		val->val1 = data->ch_data[3];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_CLEAR_0:
		val->val1 = data->ch_data[4];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_NIR_0:
		val->val1 = data->ch_data[5];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_555NM_F5:
		val->val1 = data->ch_data[6];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_590NM_F6:
		val->val1 = data->ch_data[7];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_630NM_F7:
		val->val1 = data->ch_data[8];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_680NM_F8:
		val->val1 = data->ch_data[9];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_CLEAR:
		val->val1 = data->ch_data[10];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_NIR:
		val->val1 = data->ch_data[11];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_FLICKER:
		val->val1 = data->flicker_hz;
		val->val2 = 0;
		break;

	default:
		return -ENOTSUP;
	}
	return 0;
}

/**
 * @brief Detect flicker frequency using AS7341 spectral sensor.
 *
 * This function configures the AS7341 sensor for flicker detection,
 * starts a measurement, reads the FD_STATUS register, and decodes the
 * result into a flicker frequency in Hz.
 *
 * The function uses a hard‑coded SMUX configuration for flicker detection
 * and enables only PON and FDEN (ENABLE = 0x41). No custom timing or AGC
 * settings are applied, so the sensor uses its default measurement window,
 * which is optimised for 100 Hz flicker detection (50 Hz mains).
 *
 * The measurement is followed by a 600 ms delay (empirically determined).
 * After obtaining the status, the function maps specific FD_STATUS values
 * to flicker frequencies:
 * - 0x2E (46) → 120 Hz
 * - 0x2D (45) → 100 Hz
 * - 0x2C (44) → unknown / weak flicker (reported as 1 Hz)
 * - any other value → no flicker detected (0 Hz)
 *
 * The function cleans up by disabling FDEN and SP_EN before returning.
 *
 * @param dev       Pointer to the AS7341 device structure.
 * @param flicker_hz Pointer to where the result should be stored.
 *                   When called from sample_fetch(), this points to
 *                   the driver's internal `flicker_hz` buffer.
 *                   - 0  : no flicker detected
 *                   - 1  : unknown / weak flicker
 *                   - 100: 100 Hz flicker detected
 *                   - 120: 120 Hz flicker detected
 *
 * @return 0 on success, negative errno code on failure.
 * @retval -EINVAL if @p flicker_hz is NULL.
 * @retval <0     I2C communication or configuration error.
 *
 * @note This function assumes the sensor has already been initialised
 *       (chip ID verified) and that the I2C bus is ready.
 * @note The 600 ms delay is a practical requirement; the datasheet
 *       recommends at least 300 ms, but longer improves reliability.
 * @note The FD_STATUS decoding is based on empirical values observed
 *       with the default configuration. If custom timing (FD_TIME1/2)
 *       is used, the status mapping may need adjustment.
 */
int as7341_detect_flicker(const struct device *dev, uint16_t *flicker_hz)
{
	const struct as7341_config *cfg = dev->config;
	uint8_t status;
	int err;

	if (!flicker_hz) {
		return -EINVAL;
	}
	*flicker_hz = 0;

	LOG_DBG("Starting AS7341 Flicker Detection");

	/* Power on only */
	err = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_ENABLE, AS7341_ENABLE_PON);
	if (err < 0) {
		return err;
	}

	k_msleep(10);

	/* SMUX config */
	err = as7341_set_smux_command(dev, AS7341_SMUX_CMD_WRITE);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_config_flicker(dev);
	if (err < 0) {
		return err;
	}

	err = as7341_enable_smux(dev);
	if (err < 0) {
		return err;
	}

	err = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_ENABLE,
				    AS7341_ENABLE_PON | AS7341_ENABLE_FDEN);
	if (err < 0) {
		return err;
	}

	/* Wait for measurement */
	k_msleep(600);

	/* Read FD_STATUS (0xDB) */
	err = i2c_reg_read_byte_dt(&cfg->i2c, AS7341_REG_FD_STATUS, &status);
	if (err < 0) {
		return err;
	}

	LOG_DBG("FD_STATUS = 0x%02X", status);

	switch (status) {
	case AS7341_FD_STATUS_120HZ:
		*flicker_hz = 120;
		LOG_DBG("120 Hz flicker detected");
		break;

	case AS7341_FD_STATUS_UNKNOWN:
		*flicker_hz = 1;
		LOG_DBG("Unknown flicker detected");
		break;

	case AS7341_FD_STATUS_100HZ:
		*flicker_hz = 100;
		LOG_DBG("100 Hz flicker detected");
		break;

	default:
		*flicker_hz = 0;
		LOG_DBG("No flicker detected");
		break;
	}

	/* Clean shutdown */
	i2c_reg_update_byte_dt(&cfg->i2c, AS7341_REG_ENABLE,
			       AS7341_ENABLE_FDEN | AS7341_ENABLE_SP_EN, 0);

	return 0;
}

/**
 * @brief AS7341 sensor driver API.
 *
 * Registers the AS7341 driver callbacks with the Zephyr sensor subsystem.
 * sample_fetch and channel_get are required; attr_set, attr_get
 *
 * See sensor_driver_api for the full list of available callbacks.
 */
static const struct sensor_driver_api as7341_api = {
	.attr_set = as7341_attr_set,
	.attr_get = as7341_attr_get,
	.sample_fetch = as7341_sample_fetch,
	.channel_get = as7341_channel_get,
};

/**
 * @brief Initialise the AS7341 11-channel spectral sensor.
 *
 * Called once by the Zephyr kernel at POST_KERNEL initialisation priority.
 * Performs the following steps in order:
 *
 *  1. Verifies the I2C bus is ready.
 *  2. Reads and validates the chip ID register (expected 0x24 or 0x09).
 *  3. Powers on the sensor by writing AS7341_ENABLE_PON to the ENABLE register.
 *  4. Waits 10 ms for the power-on sequence to complete.
 *  5. Enables the internal LED driver (register 0x70, bit 3).
 *  6. Sets the LED current limit (register 0x74 = 0x80).
 *  7. Sets ATIME = 100 (integration time steps).
 *  8. Sets ASTEP = 999 (0x03E7) via ASTEP_L and ASTEP_H registers.
 *  9. Sets spectral gain to 256x (CFG1 = 0x09).
 * 10. Sets CFG0 = 0x20 (bank 0, normal operating mode).
 *
 * Integration time in microseconds is calculated as:
 *   T_int = (ATIME + 1) * (ASTEP + 1) * 2.78 µs
 * With ATIME=100 and ASTEP=999: T_int ≈ 278 ms.
 *
 * @param dev Pointer to the sensor device structure.
 *
 * @retval 0        Initialisation succeeded.
 * @retval -ENODEV  I2C bus not ready, or chip ID does not match.
 * @retval <0       Negative errno code on any I2C write failure
 */
int as7341_init(const struct device *dev)
{
	const struct as7341_config *cfg = dev->config;
	uint8_t chip_id;
	int err;

	LOG_DBG("as7341_init");

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	/* Verify chip ID */
	err = i2c_reg_read_byte_dt(&cfg->i2c, AS7341_REG_ID, &chip_id);
	if (err < 0) {
		LOG_ERR("Failed to read chip ID");
		return err;
	}

	LOG_DBG("Chip ID read: 0x%02X", chip_id);

	if (chip_id != AS7341_CHIP_ID_VAL && chip_id != AS7341_CHIP_ID_ALT) {
		LOG_ERR("Unexpected chip ID: 0x%02X", chip_id);
		return -ENODEV;
	}

	/* Power ON sensor */
	err = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_ENABLE, AS7341_ENABLE_PON);
	if (err < 0) {
		LOG_ERR("Failed to power on");
		return err;
	}

	k_msleep(10);

	err = i2c_reg_update_byte_dt(&cfg->i2c, AS7341_REG_CONFIG, AS7341_CONFIG_FLICKER_MODE,
				     AS7341_CONFIG_FLICKER_MODE);
	if (err < 0) {
		return err;
	}

	err = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_LED, AS7341_LED_BRIGHTNESS_DEFAULT);
	if (err < 0) {
		LOG_ERR("Failed to set LED brightness");
		return err;
	}

	/* Set ATIME */
	err = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_ATIME, AS7341_ATIME_DEFAULT);
	if (err < 0) {
		LOG_ERR("Error setting ATIME");
		return err;
	}

	/* Set ASTEP = 999 */
	err = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_ASTEP_L, AS7341_ASTEP_999_LSB);
	if (err < 0) {
		LOG_ERR("Error setting ASTEP LSB");
		return err;
	}
	err = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_ASTEP_H, AS7341_ASTEP_999_MSB);
	if (err < 0) {
		LOG_ERR("Error setting ASTEP MSB");
		return err;
	}

	/* Set gain - AS7341_GAIN_256X */
	err = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_CFG1, AS7341_GAIN_256X);
	if (err < 0) {
		LOG_ERR("Error setting gain");
		return err;
	}

	/* Configure CFG0 for normal operation, bank 0 */
	err = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_CFG0, AS7341_CFG0_NORMAL_BANK0);
	if (err < 0) {
		LOG_ERR("Error setting CFG0");
		return err;
	}

	LOG_DBG("AS7341 initialized successfully");

	return 0;
}

#define AS7341_DEFINE(inst)                                                    \
	static struct as7341_data as7341_data_##inst;                              \
	static const struct as7341_config as7341_config_##inst = {                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                     \
	};                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, as7341_init, NULL, &as7341_data_##inst, \
								 &as7341_config_##inst, POST_KERNEL,           \
								 CONFIG_SENSOR_INIT_PRIORITY, &as7341_api);

DT_INST_FOREACH_STATUS_OKAY(AS7341_DEFINE)
