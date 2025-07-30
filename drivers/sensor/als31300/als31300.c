/*
 * Copyright (c) 2025 Croxel
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT allegro_als31300

#include "als31300.h"

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(als31300, CONFIG_SENSOR_LOG_LEVEL);

struct als31300_data {
	int16_t x_raw;
	int16_t y_raw;
	int16_t z_raw;
	int16_t temp_raw;
	bool new_data;
};

struct als31300_config {
	struct i2c_dt_spec i2c;
	uint16_t full_scale_range;
};

/**
 * @brief Get sensitivity value based on full-scale range
 * This function returns the sensitivity in LSB/gauss for the configured
 * full-scale range according to the ALS31300 datasheet specifications.
 * @param full_scale_range Full-scale range in gauss (500, 1000, or 2000)
 * @return Sensitivity value in LSB/gauss (4 for 500G, 2 for 1000G, 1 for 2000G)
 */
static float als31300_get_sensitivity(uint16_t full_scale_range)
{
	switch (full_scale_range) {
	case 500:
		return ALS31300_SENS_500G;
	case 1000:
		return ALS31300_SENS_1000G;
	case 2000:
		return ALS31300_SENS_2000G;
	default:
		return ALS31300_SENS_500G; /* Default to 500G */
	}
}

/**
 * @brief Convert 12-bit two's complement value to signed 16-bit
 * This function properly handles 12-bit two's complement conversion where:
 * - Bit 11 is the sign bit
 * - If bit 11 = 1, the number is negative
 * - If bit 11 = 0, the number is positive
 * @param value 12-bit value to convert (bits 11:0)
 * @return Signed 16-bit value
 */
static int16_t als31300_convert_12bit_to_signed(uint16_t value)
{
	/* Mask to ensure we only have 12 bits */
	value &= 0x0FFF;
	/* Check if bit 11 (sign bit) is set - negative number */
	if (value & 0x800) {
		/* For negative numbers in 12-bit two's complement:
		 * Sign extend by setting bits 15:12 to 1
		 */
		return (int16_t)(value | 0xF000);
	}

	/* Positive number, just cast */
	return (int16_t)value;
}

/**
 * @brief Convert raw magnetic field value to gauss
 * This function converts the 12-bit signed raw magnetic field value to
 * gauss units using the device's configured sensitivity range.
 * Formula: gauss = raw_value / 4096 * sensitivity_range
 * @param dev Pointer to the device structure (for configuration access)
 * @param raw_value Signed 12-bit magnetic field value
 * @return Magnetic field in gauss
 */
static float als31300_convert_to_gauss(const struct device *dev, int16_t raw_value)
{
	const struct als31300_config *cfg = dev->config;

	const float ALS31300_SENSITIVITY = als31300_get_sensitivity(cfg->full_scale_range);

	return ((float)raw_value / ALS31300_12BIT_RESOLUTION) * ALS31300_SENSITIVITY;
}

/**
 * @brief Convert raw temperature value to celsius
 * Based on datasheet formula: T(°C) = 302 * (raw_temp - 1708) / 4096
 * @param raw_temp 12-bit raw temperature value
 * @return Temperature in degrees Celsius
 */
static float als31300_convert_temperature(uint16_t raw_temp)
{
	return 302.0f * ((float)raw_temp - 1708.0f) / 4096.0f;
}

/**
 * @brief Read and parse sensor data from ALS31300
 * This function performs an 8-byte I2C burst read from registers 0x28 and 0x29
 * to get magnetic field and temperature data. The data is parsed according to
 * the datasheet bit field layout and stored in the device data structure.
 * @param dev Pointer to the device structure
 * @return 0 on success, negative error code on failure
 */
static int als31300_read_sensor_data(const struct device *dev)
{
	const struct als31300_config *cfg = dev->config;
	struct als31300_data *data = dev->data;
	uint8_t buf[8];
	uint32_t reg28_data, reg29_data;
	uint16_t x_msb, y_msb, z_msb;
	uint8_t x_lsb, y_lsb, z_lsb;
	uint8_t temp_msb, temp_lsb;
	int ret;

	/* Read both data registers in a single 8-byte transaction for consistency */
	ret = i2c_burst_read_dt(&cfg->i2c, ALS31300_REG_DATA_28, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to read sensor data: %d", ret);
		return ret;
	}

	/* Convert 8 bytes to two 32-bit values (MSB first) */
	reg28_data = ((uint32_t)buf[0] << 24) |
		((uint32_t)buf[1] << 16) |
		((uint32_t)buf[2] << 8) |
		((uint32_t)buf[3]);
	reg29_data = ((uint32_t)buf[4] << 24) |
		((uint32_t)buf[5] << 16) |
		((uint32_t)buf[6] << 8) |
		((uint32_t)buf[7]);
	/* Extract fields from register 0x28 */
	temp_msb = (reg28_data & ALS31300_REG28_TEMP_MSB_MASK) >> ALS31300_REG28_TEMP_MSB_SHIFT;
	data->new_data = !!(reg28_data & ALS31300_REG28_NEW_DATA_MASK);
	z_msb = (reg28_data & ALS31300_REG28_Z_AXIS_MSB_MASK) >> ALS31300_REG28_Z_AXIS_MSB_SHIFT;
	y_msb = (reg28_data & ALS31300_REG28_Y_AXIS_MSB_MASK) >> ALS31300_REG28_Y_AXIS_MSB_SHIFT;
	x_msb = (reg28_data & ALS31300_REG28_X_AXIS_MSB_MASK) >> ALS31300_REG28_X_AXIS_MSB_SHIFT;

	/* Extract fields from register 0x29 */
	temp_lsb = (reg29_data & ALS31300_REG29_TEMP_LSB_MASK) >> ALS31300_REG29_TEMP_LSB_SHIFT;
	z_lsb = (reg29_data & ALS31300_REG29_Z_AXIS_LSB_MASK) >> ALS31300_REG29_Z_AXIS_LSB_SHIFT;
	y_lsb = (reg29_data & ALS31300_REG29_Y_AXIS_LSB_MASK) >> ALS31300_REG29_Y_AXIS_LSB_SHIFT;
	x_lsb = (reg29_data & ALS31300_REG29_X_AXIS_LSB_MASK) >> ALS31300_REG29_X_AXIS_LSB_SHIFT;

	/* Combine MSB and LSB to form 12-bit values */
	const uint16_t x_raw = (x_msb << 4) | x_lsb;
	const uint16_t y_raw = (y_msb << 4) | y_lsb;
	const uint16_t z_raw = (z_msb << 4) | z_lsb;
	const uint16_t temp_raw = (temp_msb << 6) | temp_lsb;

	/* Convert to signed values (proper 12-bit two's complement) */
	data->x_raw = als31300_convert_12bit_to_signed(x_raw);
	data->y_raw = als31300_convert_12bit_to_signed(y_raw);
	data->z_raw = als31300_convert_12bit_to_signed(z_raw);

	/* Temperature processing */
	data->temp_raw = temp_raw;

	return 0;
}

static int als31300_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	ARG_UNUSED(chan);

	return als31300_read_sensor_data(dev);
}

static int als31300_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct als31300_data *data = dev->data;
	int32_t raw_val;
	float gauss_val;
	float temp_val;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		raw_val = data->x_raw;
		break;
	case SENSOR_CHAN_MAGN_Y:
		raw_val = data->y_raw;
		break;
	case SENSOR_CHAN_MAGN_Z:
		raw_val = data->z_raw;
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* Temperature conversion: temp(°C) = 302(value - 1708)/4096 */
		temp_val = als31300_convert_temperature(data->temp_raw);
		val->val1 = (int32_t)temp_val;
		val->val2 = (int32_t)((temp_val - val->val1) * 1000000);
		return 0;
	default:
		return -ENOTSUP;
	}

	/* Convert raw magnetic data to Gauss using proper conversion */
	gauss_val = als31300_convert_to_gauss(dev, raw_val);

	/* Convert float to sensor_value structure */
	val->val1 = (int32_t)gauss_val;
	val->val2 = (int32_t)((gauss_val - val->val1) * 1000000);

	return 0;
}

static const struct sensor_driver_api als31300_api = {
	.sample_fetch = als31300_sample_fetch,
	.channel_get = als31300_channel_get,
};

/**
 * @brief Configure ALS31300 to Active Mode
 * This function sets the device to Active Mode by writing to the volatile
 * register 0x27. This register can be written without customer access mode.
 * @param dev Pointer to the device structure
 * @return 0 on success, negative error code on failure
 */
static int als31300_configure_device(const struct device *dev)
{
	const struct als31300_config *cfg = dev->config;
	uint32_t reg27_value = 0x00000000; /* All bits to 0 = Active Mode */
	int ret;

	LOG_INF("Configuring ALS31300 to Active Mode...");

	/* Write 0x00000000 to register 0x27 to set Active Mode
	 * Bits [1:0] = 0 → Active Mode
	 * Bits [3:2] = 0 → Single read mode (default I2C mode)
	 * Bits [6:4] = 0 → Low-power count = 0.5ms (doesn't matter in Active Mode)
	 * Bits [31:7] = 0 → Reserved (should be 0)
	 */
	ret = i2c_reg_write_byte_dt(&cfg->i2c, ALS31300_REG_VOLATILE_27, reg27_value);
	if (ret < 0) {
		LOG_ERR("Failed to write to register 0x27: %d", ret);
		return ret;
	}

	/* Small delay to ensure the mode change takes effect */
	k_msleep(10);

	return 0;
}

/**
 * @brief Initialize ALS31300 device
 */
static int als31300_init(const struct device *dev)
{
	const struct als31300_config *cfg = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	/* Wait for power-on delay as specified in datasheet */
	k_usleep(ALS31300_POWER_ON_DELAY_US);

	/* Test communication by reading a register (can be done without customer access) */
	uint8_t test_val[4];

	ret = i2c_reg_read_byte_dt(&cfg->i2c, ALS31300_REG_VOLATILE_27, test_val);
	if (ret < 0) {
		LOG_ERR("Failed to communicate with sensor: %d", ret);
		return ret;
	}

	/* Configure device to Active Mode */
	ret = als31300_configure_device(dev);
	if (ret < 0) {
		LOG_ERR("Failed to configure device: %d", ret);
		return ret;
	}

	/* Wait a bit more for the sensor to be fully ready in Active Mode */
	k_msleep(ALS31300_REG_WRITE_DELAY_MS);

	return 0;
}

#define ALS31300_INIT(inst) \
	static struct als31300_data als31300_data_##inst; \
	 \
	static const struct als31300_config als31300_config_##inst = { \
		.i2c = I2C_DT_SPEC_INST_GET(inst), \
		.full_scale_range = DT_INST_PROP(inst, full_scale_range), \
	}; \
	 \
	DEVICE_DT_INST_DEFINE(inst, als31300_init, NULL, \
			      &als31300_data_##inst, &als31300_config_##inst, \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
			      &als31300_api);

DT_INST_FOREACH_STATUS_OKAY(ALS31300_INIT)
