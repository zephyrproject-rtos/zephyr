/*
 * Copyright (c) 2025 Croxel
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT allegro_als31300

#include "als31300.h"

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/i2c.h>

LOG_MODULE_REGISTER(als31300, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Convert 12-bit two's complement value to signed 16-bit
 * @param value 12-bit value to convert (bits 11:0)
 * @return Signed 16-bit value
 */
int16_t als31300_convert_12bit_to_signed(uint16_t value)
{
	return (int16_t)sign_extend(value, ALS31300_12BIT_SIGN_BIT_INDEX);
}

/**
 * @brief Parse raw register data from 8-byte buffer
 * @param buf 8-byte buffer containing register 0x28 and 0x29 data
 * @param readings Pointer to readings structure to store parsed data
 */
void als31300_parse_registers(const uint8_t *buf, struct als31300_readings *readings)
{
	uint32_t reg28_data, reg29_data;
	uint16_t x_msb, y_msb, z_msb;
	uint8_t x_lsb, y_lsb, z_lsb;
	uint8_t temp_msb, temp_lsb;

	/* Convert 8 bytes to two 32-bit values (MSB first) */
	reg28_data = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) |
		     ((uint32_t)buf[3]);
	reg29_data = ((uint32_t)buf[4] << 24) | ((uint32_t)buf[5] << 16) | ((uint32_t)buf[6] << 8) |
		     ((uint32_t)buf[7]);

	/* Extract fields from register 0x28 */
	temp_msb = (reg28_data & ALS31300_REG28_TEMP_MSB_MASK) >> ALS31300_REG28_TEMP_MSB_SHIFT;
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
	readings->x = als31300_convert_12bit_to_signed(x_raw);
	readings->y = als31300_convert_12bit_to_signed(y_raw);
	readings->z = als31300_convert_12bit_to_signed(z_raw);
	readings->temp = temp_raw;
}

/**
 * @brief Convert raw magnetic field value to microgauss
 * This function converts the 12-bit signed raw magnetic field value to
 * microgauss units
 * Formula: microgauss = (raw_value * 500 * 1000000) / 4096
 * @param raw_value Signed 12-bit magnetic field value
 * @return Magnetic field in microgauss
 */
int32_t als31300_convert_to_gauss(int16_t raw_value)
{
	/* Convert to microgauss
	 * For 500G full scale: (raw_value * 500 * 1000000) / 4096
	 * This gives us the fractional part in microgauss
	 */
	return ((int64_t)raw_value * ALS31300_FULL_SCALE_RANGE_GAUSS * 1000000) /
	       ALS31300_12BIT_RESOLUTION;
}

/**
 * @brief Convert raw temperature value to celsius
 * Based on datasheet formula: T(°C) = 302 * (raw_temp - 1708) / 4096
 * @param raw_temp 12-bit raw temperature value
 * @return Temperature in microcelsius
 */
int32_t als31300_convert_temperature(uint16_t raw_temp)
{
	/* Convert to microcelsius
	 * Formula: microcelsius = (302 * (raw_temp - 1708) * 1000000) / 4096
	 */
	return ((int64_t)ALS31300_TEMP_SCALE_FACTOR * ((int32_t)raw_temp - ALS31300_TEMP_OFFSET) *
		1000000) /
	       ALS31300_TEMP_DIVISOR;
}

/**
 * @brief Read and parse sensor data from ALS31300
 * This function performs an 8-byte I2C burst read from registers 0x28 and 0x29
 * to get magnetic field and temperature data. The data is parsed according to
 * the datasheet bit field layout and stored in the provided readings structure.
 * @param dev Pointer to the device structure
 * @param readings Pointer to readings structure to store data
 * @return 0 on success, negative error code on failure
 */
static int als31300_read_sensor_data(const struct device *dev, enum sensor_channel chan,
				     struct als31300_readings *readings)
{
	const struct als31300_config *cfg = dev->config;
	struct als31300_data *data = dev->data;
	uint8_t buf[8];
	int ret;

	/* Read both data registers in a single 8-byte transaction for consistency */
	ret = i2c_burst_read_dt(&cfg->i2c, ALS31300_REG_DATA_28, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to read sensor data: %d", ret);
		return ret;
	}

	/* Parse the register data using common helper */
	als31300_parse_registers(buf, readings);

	/* Also update local data structure for compatibility */
	data->x_raw = readings->x;
	data->y_raw = readings->y;
	data->z_raw = readings->z;
	data->temp_raw = readings->temp;

	return 0;
}

static int als31300_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct als31300_readings readings;

	return als31300_read_sensor_data(dev, chan, &readings);
}

static int als31300_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct als31300_data *data = dev->data;
	int32_t raw_val;
	int32_t converted_val;

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
		/* Temperature conversion */
		converted_val = als31300_convert_temperature(data->temp_raw);
		sensor_value_from_micro(val, converted_val);
		return 0;
	default:
		return -ENOTSUP;
	}

	/* Convert raw magnetic data to gauss */
	converted_val = als31300_convert_to_gauss(raw_val);
	sensor_value_from_micro(val, converted_val);

	return 0;
}

static DEVICE_API(sensor, als31300_api) = {
	.sample_fetch = als31300_sample_fetch,
	.channel_get = als31300_channel_get,
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = als31300_submit,
	.get_decoder = als31300_get_decoder,
#endif
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
	ret = i2c_burst_write_dt(&cfg->i2c, ALS31300_REG_VOLATILE_27, (uint8_t *)&reg27_value,
				 sizeof(reg27_value));
	if (ret < 0) {
		LOG_ERR("Failed to write to register 0x27: %d", ret);
		return ret;
	}

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
	uint8_t test_val;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, ALS31300_REG_VOLATILE_27, &test_val);
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

#define ALS31300_INIT(inst)                                                                        \
	RTIO_DEFINE(als31300_rtio_ctx_##inst, 16, 16);                                             \
	I2C_DT_IODEV_DEFINE(als31300_bus_##inst, DT_DRV_INST(inst));                               \
                                                                                                   \
	static struct als31300_data als31300_data_##inst;                                          \
                                                                                                   \
	static const struct als31300_config als31300_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.bus =                                                                             \
			{                                                                          \
				.ctx = &als31300_rtio_ctx_##inst,                                  \
				.iodev = &als31300_bus_##inst,                                     \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, als31300_init, NULL, &als31300_data_##inst,             \
				     &als31300_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &als31300_api)

DT_INST_FOREACH_STATUS_OKAY(ALS31300_INIT);
