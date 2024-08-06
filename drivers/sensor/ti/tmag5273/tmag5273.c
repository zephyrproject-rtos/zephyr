/*
 * Copyright (c) 2023 deveritec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmag5273

#include "tmag5273.h"

#include <stdint.h>
#include <stdlib.h>

#include <zephyr/drivers/sensor/tmag5273.h>
#include <zephyr/dt-bindings/sensor/tmag5273.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(TMAG5273, CONFIG_SENSOR_LOG_LEVEL);

#define CONV_FACTOR_MT_TO_GS 10

#define TMAG5273_CRC_DATA_BYTES 4
#define TMAG5273_CRC_I2C_SIZE   COND_CODE_1(CONFIG_CRC, (1), (0))

/**
 * @brief size of the buffer to read out all result data from the sensor
 *
 * Since the register counting is zero-based, one byte needs to be added to get the correct size.
 * Also takes into account if CRC is enabled, which adds an additional byte for the CRC always
 * located after the last read result byte.
 */
#define TMAG5273_I2C_BUFFER_SIZE                                                                   \
	(TMAG5273_REG_RESULT_END - TMAG5273_REG_RESULT_BEGIN + 1 + TMAG5273_CRC_I2C_SIZE)

/** static configuration data */
struct tmag5273_config {
	struct i2c_dt_spec i2c;

	uint8_t mag_channel;
	uint8_t axis;
	bool temperature;

	uint8_t meas_range;
	uint8_t temperature_coefficient;
	uint8_t angle_magnitude_axis;
	uint8_t ch_mag_gain_correction;

	uint8_t operation_mode;
	uint8_t averaging;

	bool trigger_conv_via_int;
	bool low_noise_mode;
	bool ignore_diag_fail;

	struct gpio_dt_spec int_gpio;

#ifdef CONFIG_CRC
	bool crc_enabled;
#endif
};

struct tmag5273_data {
	uint8_t version;             /** version as given by the sensor */
	uint16_t conversion_time_us; /** time for one conversion */

	int16_t x_sample;           /** measured B-field @x-axis */
	int16_t y_sample;           /** measured B-field @y-axis */
	int16_t z_sample;           /** measured B-field @z-axis */
	int16_t temperature_sample; /** measured temperature data */

	uint16_t xyz_range; /** magnetic range for x/y/z-axis in mT */

	int16_t angle_sample;     /** measured angle in degree, if activated */
	uint8_t magnitude_sample; /** Positive vector magnitude (can be >7 bit). */
};

/**
 * @brief resets the DEVICE_STATUS register
 *
 * @param dev driver handle
 * @retval see @ref i2c_reg_write_byte
 */
static int tmag5273_reset_device_status(const struct device *dev)
{
	const struct tmag5273_config *drv_cfg = dev->config;

	return i2c_reg_write_byte_dt(&drv_cfg->i2c, TMAG5273_REG_DEVICE_STATUS,
				     TMAG5273_RESET_DEVICE_STATUS);
}

/**
 * @brief checks for DIAG_FAIL errors and reads out the DEVICE_STATUS register if necessary
 *
 * @param[in] drv_cfg driver instance configuration
 * @param[out] device_status DEVICE_STATUS register if DIAG_FAIL is set
 *
 * @retval 0 on success
 * @retval "!= 0" on error
 *                  - \c -EIO on any set error device status bit
 *                  - see @ref i2c_reg_read_byte for error codes
 *
 * @note
 * If tmag5273_config.ignore_diag_fail is set
 *   - \a device_status will be always set to \c 0,
 *   - the function always returns \c 0.
 */
static int tmag5273_check_device_status(const struct tmag5273_config *drv_cfg,
					uint8_t *device_status)
{
	int retval;

	if (drv_cfg->ignore_diag_fail) {
		*device_status = 0;
		return 0;
	}

	retval = i2c_reg_read_byte_dt(&drv_cfg->i2c, TMAG5273_REG_CONV_STATUS, device_status);
	if (retval < 0) {
		LOG_ERR("error reading CONV_STATUS %d", retval);
		return retval;
	}

	if ((*device_status & TMAG5273_DIAG_STATUS_MSK) != TMAG5273_DIAG_FAIL) {
		/* no error */
		*device_status = 0;
		return 0;
	}

	retval = i2c_reg_read_byte_dt(&drv_cfg->i2c, TMAG5273_REG_DEVICE_STATUS, device_status);
	if (retval < 0) {
		LOG_ERR("error reading DEVICE_STATUS %d", retval);
		return retval;
	}

	if ((*device_status & TMAG5273_VCC_UV_ER_MSK) == TMAG5273_VCC_UV_ERR) {
		LOG_ERR("VCC under voltage detected");
	}
#ifdef CONFIG_CRC
	if (drv_cfg->crc_enabled &&
	    ((*device_status & TMAG5273_OTP_CRC_ER_MSK) == TMAG5273_OTP_CRC_ERR)) {
		LOG_ERR("OTP CRC error detected");
	}
#endif
	if ((*device_status & TMAG5273_INT_ER_MSK) == TMAG5273_INT_ERR) {
		LOG_ERR("INT pin error detected");
	}

	if ((*device_status & TMAG5273_OSC_ER_MSK) == TMAG5273_OSC_ERR) {
		LOG_ERR("Oscillator error detected");
	}

	return -EIO;
}

/**
 * @brief performs a trigger through the INT-pin
 *
 * @param drv_cfg driver instance configuration
 *
 * @retval 0 on success
 * @retval see @ref gpio_pin_set_dt
 */
static inline int tmag5273_dev_int_trigger(const struct tmag5273_config *drv_cfg)
{
	int retval;

	retval = gpio_pin_configure_dt(&drv_cfg->int_gpio, GPIO_OUTPUT);
	if (retval < 0) {
		return retval;
	}

	retval = gpio_pin_set_dt(&drv_cfg->int_gpio, 1);
	if (retval < 0) {
		return retval;
	}

	retval = gpio_pin_set_dt(&drv_cfg->int_gpio, 0);
	if (retval < 0) {
		return retval;
	}

	retval = gpio_pin_configure_dt(&drv_cfg->int_gpio, GPIO_INPUT);
	if (retval < 0) {
		return retval;
	}

	return 0;
}

/** @brief returns the high measurement range based on the chip version */
static inline uint16_t tmag5273_range_high(uint8_t version)
{
	switch (version) {
	case TMAG5273_VER_TMAG5273X1:
		return TMAG5273_MEAS_RANGE_HIGH_MT_VER1;
	case TMAG5273_VER_TMAG5273X2:
		return TMAG5273_MEAS_RANGE_HIGH_MT_VER2;
	case TMAG5273_VER_TMAG3001X1:
		return TMAG3001_MEAS_RANGE_HIGH_MT_VER1;
	case TMAG5273_VER_TMAG3001X2:
		return TMAG3001_MEAS_RANGE_HIGH_MT_VER2;
	default:
		return -ENODEV;
	}
}

/** @brief returns the low measurement range based on the chip version */
static inline uint16_t tmag5273_range_low(uint8_t version)
{
	switch (version) {
	case TMAG5273_VER_TMAG5273X1:
		return TMAG5273_MEAS_RANGE_LOW_MT_VER1;
	case TMAG5273_VER_TMAG5273X2:
		return TMAG5273_MEAS_RANGE_LOW_MT_VER2;
	case TMAG5273_VER_TMAG3001X1:
		return TMAG3001_MEAS_RANGE_LOW_MT_VER1;
	case TMAG5273_VER_TMAG3001X2:
		return TMAG3001_MEAS_RANGE_LOW_MT_VER2;
	default:
		return -ENODEV;
	}
}

/**
 * @brief update the measurement range of the X/Y/Z-axis
 *
 * @param dev handle to the sensor
 * @param val value to be set
 *
 * @return see @ref i2c_reg_update_byte_dt
 */
static inline int tmag5273_attr_set_xyz_meas_range(const struct device *dev,
						   const struct sensor_value *val)
{
	const struct tmag5273_config *drv_cfg = dev->config;
	struct tmag5273_data *drv_data = dev->data;

	const uint16_t range_high = tmag5273_range_high(drv_data->version);
	const uint16_t range_low = tmag5273_range_low(drv_data->version);

	int retval;
	uint8_t regdata;
	uint16_t range;

	if (val->val1 >= range_high) {
		regdata = TMAG5273_XYZ_MEAS_RANGE_HIGH;
		range = range_high;
	} else {
		regdata = TMAG5273_XYZ_MEAS_RANGE_LOW;
		range = range_low;
	}

	retval = i2c_reg_update_byte_dt(&drv_cfg->i2c, TMAG5273_REG_SENSOR_CONFIG_2,
					TMAG5273_MEAS_RANGE_XYZ_MSK, regdata);
	if (retval < 0) {
		return retval;
	}

	drv_data->xyz_range = range;

	return 0;
}

/**
 * @brief returns the used measurement range of the X/Y/Z-axis
 *
 * @param dev handle to the sensor
 * @param val return value
 *
 * @return \c 0 on success
 * @return see @ref i2c_reg_read_byte_dt
 */
static inline int tmag5273_attr_get_xyz_meas_range(const struct device *dev,
						   struct sensor_value *val)
{
	const struct tmag5273_config *drv_cfg = dev->config;
	struct tmag5273_data *drv_data = dev->data;

	uint8_t regdata;
	int retval;

	retval = i2c_reg_read_byte_dt(&drv_cfg->i2c, TMAG5273_REG_SENSOR_CONFIG_2, &regdata);
	if (retval < 0) {
		return retval;
	}

	if ((regdata & TMAG5273_MEAS_RANGE_XYZ_MSK) == TMAG5273_XYZ_MEAS_RANGE_HIGH) {
		val->val1 = tmag5273_range_high(drv_data->version);
	} else {
		val->val1 = tmag5273_range_low(drv_data->version);
	}

	val->val2 = 0;

	return 0;
}

/**
 * set the X/Y/Z angle & magnitude calculation mode
 *
 * @param dev handle to the sensor
 * @param val value to be set
 *
 * @return \c -ENOTSUP if unknown value
 * @return see @ref i2c_reg_update_byte_dt
 */
static inline int tmag5273_attr_set_xyz_calc(const struct device *dev,
					     const struct sensor_value *val)
{
	const struct tmag5273_config *drv_cfg = dev->config;
	uint8_t regdata;
	int retval;

	switch (val->val1) {
	case TMAG5273_ANGLE_CALC_NONE:
		regdata = TMAG5273_ANGLE_EN_NONE;
		break;
	case TMAG5273_ANGLE_CALC_XY:
		if (!(drv_cfg->axis & TMAG5273_MAG_CH_EN_X) ||
		    !(drv_cfg->axis & TMAG5273_MAG_CH_EN_Y)) {
			return -ENOTSUP;
		}
		regdata = TMAG5273_ANGLE_EN_XY;
		break;
	case TMAG5273_ANGLE_CALC_YZ:
		if (!(drv_cfg->axis & TMAG5273_MAG_CH_EN_Y) ||
		    !(drv_cfg->axis & TMAG5273_MAG_CH_EN_Z)) {
			return -ENOTSUP;
		}
		regdata = TMAG5273_ANGLE_EN_YZ;
		break;
	case TMAG5273_ANGLE_CALC_XZ:
		if (!(drv_cfg->axis & TMAG5273_MAG_CH_EN_X) ||
		    !(drv_cfg->axis & TMAG5273_MAG_CH_EN_Z)) {
			return -ENOTSUP;
		}
		regdata = TMAG5273_ANGLE_EN_XZ;
		break;
	default:
		LOG_ERR("unknown attribute value %d", val->val1);
		return -ENOTSUP;
	}

	retval = i2c_reg_update_byte_dt(&drv_cfg->i2c, TMAG5273_REG_SENSOR_CONFIG_2,
					TMAG5273_ANGLE_EN_MSK, regdata);
	if (retval < 0) {
		return retval;
	}

	return 0;
}

/**
 * returns the X/Y/Z angle & magnitude calculation mode
 *
 * @param dev handle to the sensor
 * @param val return value
 *
 * @return \c 0 on success
 * @return see @ref i2c_reg_read_byte_dt
 */
static inline int tmag5273_attr_get_xyz_calc(const struct device *dev, struct sensor_value *val)
{
	const struct tmag5273_config *drv_cfg = dev->config;

	uint8_t regdata;
	int retval;

	retval = i2c_reg_read_byte_dt(&drv_cfg->i2c, TMAG5273_REG_SENSOR_CONFIG_2, &regdata);
	if (retval < 0) {
		return retval;
	}

	switch (regdata & TMAG5273_ANGLE_EN_MSK) {
	case TMAG5273_ANGLE_EN_XY:
		val->val1 = TMAG5273_ANGLE_CALC_XY;
		break;
	case TMAG5273_ANGLE_EN_YZ:
		val->val1 = TMAG5273_ANGLE_CALC_YZ;
		break;
	case TMAG5273_ANGLE_EN_XZ:
		val->val1 = TMAG5273_ANGLE_CALC_XZ;
		break;
	case TMAG5273_ANGLE_EN_NONE:
		__fallthrough;
	default:
		val->val1 = TMAG5273_ANGLE_CALC_NONE;
	}

	val->val2 = 0;

	return 0;
}

/** @brief returns the number of bytes readable per block for i2c burst reads */
static inline uint8_t tmag5273_get_fetch_block_size(const struct tmag5273_config *drv_cfg,
						    uint8_t remaining_bytes)
{
#ifdef CONFIG_CRC
	if (drv_cfg->crc_enabled && (remaining_bytes > TMAG5273_CRC_DATA_BYTES)) {
		return TMAG5273_CRC_DATA_BYTES;
	}
#endif
	return remaining_bytes;
}

/** @brief returns the size of the CRC field if active */
static inline uint8_t tmag5273_get_crc_size(const struct tmag5273_config *drv_cfg)
{
#ifdef CONFIG_CRC
	if (drv_cfg->crc_enabled) {
		return TMAG5273_CRC_I2C_SIZE;
	}
#else
	ARG_UNUSED(drv_cfg);
#endif
	return 0;
}

static int tmag5273_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	CHECKIF(dev == NULL) {
		LOG_ERR("dev: NULL");
		return -EINVAL;
	}

	CHECKIF(val == NULL) {
		LOG_ERR("val: NULL");
		return -EINVAL;
	}

	if (chan != SENSOR_CHAN_MAGN_XYZ) {
		return -ENOTSUP;
	}

	const struct tmag5273_config *drv_cfg = dev->config;

	int retval;

	switch ((uint16_t)attr) {
	case SENSOR_ATTR_FULL_SCALE:
		if (drv_cfg->meas_range != TMAG5273_DT_AXIS_RANGE_RUNTIME) {
			return -ENOTSUP;
		}

		retval = tmag5273_attr_set_xyz_meas_range(dev, val);
		if (retval < 0) {
			return retval;
		}
		break;
	case TMAG5273_ATTR_ANGLE_MAG_AXIS:
		if (drv_cfg->angle_magnitude_axis != TMAG5273_DT_ANGLE_MAG_RUNTIME) {
			return -ENOTSUP;
		}

		retval = tmag5273_attr_set_xyz_calc(dev, val);
		if (retval < 0) {
			return retval;
		}
		break;
	default:
		LOG_ERR("unknown attribute %d", attr);
		return -ENOTSUP;
	}

	return 0;
}

static int tmag5273_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	CHECKIF(dev == NULL) {
		LOG_ERR("dev: NULL");
		return -EINVAL;
	}

	CHECKIF(val == NULL) {
		LOG_ERR("val: NULL");
		return -EINVAL;
	}

	if (chan != SENSOR_CHAN_MAGN_XYZ) {
		return -ENOTSUP;
	}

	const struct tmag5273_config *drv_cfg = dev->config;

	int retval;

	switch ((uint16_t)attr) {
	case SENSOR_ATTR_FULL_SCALE:
		if (drv_cfg->meas_range != TMAG5273_DT_AXIS_RANGE_RUNTIME) {
			return -ENOTSUP;
		}

		retval = tmag5273_attr_get_xyz_meas_range(dev, val);
		if (retval < 0) {
			return retval;
		}
		break;
	case TMAG5273_ATTR_ANGLE_MAG_AXIS:
		if (drv_cfg->angle_magnitude_axis != TMAG5273_DT_ANGLE_MAG_RUNTIME) {
			return -ENOTSUP;
		}

		retval = tmag5273_attr_get_xyz_calc(dev, val);
		if (retval < 0) {
			return retval;
		}
		break;
	default:
		LOG_ERR("unknown attribute %d", attr);
		return -ENOTSUP;
	}

	return 0;
}

static int tmag5273_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct tmag5273_config *drv_cfg = dev->config;
	struct tmag5273_data *drv_data = dev->data;

	int retval;

	uint8_t i2c_buffer[TMAG5273_I2C_BUFFER_SIZE] = {0};

	/* trigger a conversion and wait until done if in standby mode */
	if (drv_cfg->operation_mode == TMAG5273_DT_OPER_MODE_STANDBY) {
		if (drv_cfg->trigger_conv_via_int) {
			retval = tmag5273_dev_int_trigger(drv_cfg);
			if (retval < 0) {
				return retval;
			}
		}

		uint8_t conv_bit = TMAG5273_CONVERSION_START_BIT;

		while ((i2c_buffer[0] & TMAG5273_RESULT_STATUS_MSK) !=
		       TMAG5273_CONVERSION_COMPLETE) {
			retval = i2c_reg_read_byte_dt(
				&drv_cfg->i2c, TMAG5273_REG_CONV_STATUS | conv_bit, &i2c_buffer[0]);

			if (retval < 0) {
				LOG_ERR("error reading conversion state %d", retval);
				return retval;
			}

			conv_bit = 0;

			k_usleep(drv_data->conversion_time_us);
		}
	}

	/* read data */
	uint8_t start_address, end_address;

	switch ((int)chan) {
	case SENSOR_CHAN_MAGN_X:
		if (!(drv_cfg->axis & TMAG5273_MAG_CH_EN_X)) {
			LOG_ERR("x-axis measurement deactivated");
			return -ENOTSUP;
		}
		start_address = TMAG5273_REG_X_MSB_RESULT;
		end_address = TMAG5273_REG_X_LSB_RESULT;
		break;
	case SENSOR_CHAN_MAGN_Y:
		if (!(drv_cfg->axis & TMAG5273_MAG_CH_EN_Y)) {
			LOG_ERR("y-axis measurement deactivated");
			return -ENOTSUP;
		}
		start_address = TMAG5273_REG_Y_MSB_RESULT;
		end_address = TMAG5273_REG_Y_LSB_RESULT;
		break;
	case SENSOR_CHAN_MAGN_Z:
		if (!(drv_cfg->axis & TMAG5273_MAG_CH_EN_Z)) {
			LOG_ERR("x-axis measurement deactivated");
			return -ENOTSUP;
		}
		start_address = TMAG5273_REG_Z_MSB_RESULT;
		end_address = TMAG5273_REG_Z_LSB_RESULT;
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		if (drv_cfg->axis == TMAG5273_MAG_CH_EN_NONE) {
			LOG_ERR("xyz-axis measurement deactivated");
			return -ENOTSUP;
		}
		start_address = TMAG5273_REG_X_MSB_RESULT;
		end_address = TMAG5273_REG_Z_LSB_RESULT;
		break;
	case SENSOR_CHAN_DIE_TEMP:
		if (!drv_cfg->temperature) {
			LOG_ERR("temperature measurement deactivated");
			return -ENOTSUP;
		}
		start_address = TMAG5273_REG_T_MSB_RESULT;
		end_address = TMAG5273_REG_T_LSB_RESULT;
		break;
	case SENSOR_CHAN_ROTATION:
		if (drv_cfg->angle_magnitude_axis == TMAG5273_ANGLE_CALC_NONE) {
			LOG_ERR("axis measurement deactivated");
			return -ENOTSUP;
		}
		start_address = TMAG5273_REG_ANGLE_MSB_RESULT;
		end_address = TMAG5273_REG_ANGLE_LSB_RESULT;
		break;
	case TMAG5273_CHAN_MAGNITUDE:
	case TMAG5273_CHAN_MAGNITUDE_MSB:
		if (drv_cfg->angle_magnitude_axis == TMAG5273_ANGLE_CALC_NONE) {
			LOG_ERR("axis measurement deactivated");
			return -ENOTSUP;
		}
		start_address = end_address = TMAG5273_REG_MAGNITUDE_RESULT;
		break;
	case TMAG5273_CHAN_ANGLE_MAGNITUDE:
		if (drv_cfg->angle_magnitude_axis == TMAG5273_ANGLE_CALC_NONE) {
			LOG_ERR("axis measurement deactivated");
			return -ENOTSUP;
		}
		start_address = TMAG5273_REG_ANGLE_MSB_RESULT;
		end_address = TMAG5273_REG_MAGNITUDE_RESULT;
		break;
	case SENSOR_CHAN_ALL:
		start_address = TMAG5273_REG_RESULT_BEGIN;
		end_address = TMAG5273_REG_RESULT_END;
		break;
	default:
		LOG_ERR("unknown sensor channel %d", chan);
		return -EINVAL;
	}

	__ASSERT_NO_MSG(start_address >= TMAG5273_REG_RESULT_BEGIN);
	__ASSERT_NO_MSG(end_address <= TMAG5273_REG_RESULT_END);
	__ASSERT_NO_MSG(start_address <= end_address);

	uint32_t nb_bytes = end_address - start_address + 1;

#ifdef CONFIG_CRC
	/* if CRC is enabled multiples of TMAG5273_CRC_DATA_BYTES need to be read */
	if (drv_cfg->crc_enabled && ((nb_bytes % TMAG5273_CRC_DATA_BYTES) != 0)) {
		const uint8_t diff = TMAG5273_CRC_DATA_BYTES - (nb_bytes % TMAG5273_CRC_DATA_BYTES);

		if ((start_address - diff) >= TMAG5273_REG_RESULT_BEGIN) {
			start_address -= diff;
		}

		nb_bytes = (nb_bytes / TMAG5273_CRC_DATA_BYTES + 1) * TMAG5273_CRC_DATA_BYTES;
	}

	__ASSERT_NO_MSG((start_address + nb_bytes) <= (TMAG5273_REG_RESULT_END + 1));
#endif

	uint8_t offset = start_address - TMAG5273_REG_RESULT_BEGIN;
	const uint8_t crc_size = tmag5273_get_crc_size(drv_cfg);

	while (nb_bytes) {
		const uint8_t block_size = tmag5273_get_fetch_block_size(drv_cfg, nb_bytes);

		__ASSERT((offset + block_size + crc_size) <= TMAG5273_I2C_BUFFER_SIZE,
			 "block_size would exceed available i2c buffer capacity");
		__ASSERT(start_address <= end_address,
			 "start_address for reading after end address");

		/* Note: crc_size needs to be read additionally, since it is appended on the end */
		retval = i2c_burst_read_dt(&drv_cfg->i2c, start_address, &i2c_buffer[offset],
					   block_size + crc_size);

		if (retval < 0) {
			LOG_ERR("could not read result data %d", retval);
			return -EIO;
		}

#ifdef CONFIG_CRC
		/* check data validity, if activated */
		if (drv_cfg->crc_enabled) {
			const uint8_t crc = crc8_ccitt(0xFF, &i2c_buffer[offset], block_size);

			if (i2c_buffer[offset + block_size] != crc) {
				LOG_ERR("invalid CRC value: 0x%X (expected: 0x%X)",
					i2c_buffer[offset + block_size], crc);
				return -EIO;
			}
		}
#endif

		__ASSERT(nb_bytes >= block_size, "overflow on nb_bytes");

		nb_bytes -= block_size;

		offset += block_size;
		start_address += block_size;
	}

	retval = tmag5273_check_device_status(
		drv_cfg, &i2c_buffer[TMAG5273_REG_CONV_STATUS - TMAG5273_REG_RESULT_BEGIN]);
	if (retval < 0) {
		return retval;
	}

	bool all_channels = (chan == SENSOR_CHAN_ALL);
	bool all_xyz = all_channels || (chan == SENSOR_CHAN_MAGN_XYZ);
	bool all_angle_magnitude = all_channels || ((int)chan == TMAG5273_CHAN_ANGLE_MAGNITUDE);

	if (all_xyz || (chan == SENSOR_CHAN_MAGN_X)) {
		drv_data->x_sample = sys_get_be16(
			&i2c_buffer[TMAG5273_REG_X_MSB_RESULT - TMAG5273_REG_RESULT_BEGIN]);
	}

	if (all_xyz || (chan == SENSOR_CHAN_MAGN_Y)) {
		drv_data->y_sample = sys_get_be16(
			&i2c_buffer[TMAG5273_REG_Y_MSB_RESULT - TMAG5273_REG_RESULT_BEGIN]);
	}

	if (all_xyz || (chan == SENSOR_CHAN_MAGN_Z)) {
		drv_data->z_sample = sys_get_be16(
			&i2c_buffer[TMAG5273_REG_Z_MSB_RESULT - TMAG5273_REG_RESULT_BEGIN]);
	}

	if (all_channels || (chan == SENSOR_CHAN_DIE_TEMP)) {
		drv_data->temperature_sample = sys_get_be16(
			&i2c_buffer[TMAG5273_REG_T_MSB_RESULT - TMAG5273_REG_RESULT_BEGIN]);
	}

	if (all_angle_magnitude || (chan == SENSOR_CHAN_ROTATION)) {
		drv_data->angle_sample = sys_get_be16(
			&i2c_buffer[TMAG5273_REG_ANGLE_MSB_RESULT - TMAG5273_REG_RESULT_BEGIN]);
	}
	if (all_angle_magnitude || ((int)chan == TMAG5273_CHAN_MAGNITUDE) ||
	    ((int)chan == TMAG5273_CHAN_MAGNITUDE_MSB)) {
		drv_data->magnitude_sample =
			i2c_buffer[TMAG5273_REG_MAGNITUDE_RESULT - TMAG5273_REG_RESULT_BEGIN];
	}

	return 0;
}

/**
 * @brief calculates the b-field value in G based on the sensor value
 *
 * The calculation follows the formula
 * @f[ B=\frac{-(D_{15} \cdot 2^{15}) + \sum_{i=0}^{14} D_i \cdot 2^i}{2^{16}} \cdot 2|B_R| @f]
 * where
 *	- \em D denotes the bit of the input data,
 *	- \em Br represents the magnetic range in mT
 *
 * After the calculation, the value is scaled to Gauss (1 G == 0.1 mT).
 *
 * @param[in] raw_value data read from the device
 * @param[in] range magnetic range of the selected axis (in mT)
 * @param[out] b_field holds the result data after the operation
 */
static inline void tmag5273_channel_b_field_convert(int64_t raw_value, const uint16_t range,
						    struct sensor_value *b_field)
{
	raw_value *= (range << 1) * CONV_FACTOR_MT_TO_GS;

	/* calc integer part in mT and scale to G */
	b_field->val1 = raw_value / (1 << 16);

	/* calc remaining part (first mT digit + fractal part) and scale according to Zephyr.
	 * Ensure that always positive.
	 */
	const int64_t raw_dec_part = (int64_t)b_field->val1 * (1 << 16);

	b_field->val2 = ((raw_value - raw_dec_part) * 1000000) / (1 << 16);
}

/**
 * @brief calculates the temperature value
 *
 * @param[in] raw_value data read from the device
 * @param[out] temperature holds the result data after the operation
 */
static inline void tmag5273_temperature_convert(int64_t raw_value, struct sensor_value *temperature)
{
	const int64_t value =
		(TMAG5273_TEMPERATURE_T_SENS_T0 +
		 ((raw_value - TMAG5273_TEMPERATURE_T_ADC_T0) / TMAG5273_TEMPERATURE_T_ADC_RES)) *
		1000000;

	temperature->val1 = value / 1000000;
	temperature->val2 = value % 1000000;
}

/**
 * @brief calculates the angle value between two axis
 *
 * @param[in] raw_value data read from the device
 * @param[out] angle holds the result data after the operation
 */
static inline void tmag5273_angle_convert(int16_t raw_value, struct sensor_value *angle)
{
	angle->val1 = (raw_value >> 4) & 0x1FF;
	angle->val2 = ((raw_value & 0xF) * 1000000) >> 1;
}

/**
 * @brief calculates the magnitude value in G between two axis
 *
 * Note that \c MAGNITUDE_RESULT represents the MSB of the calculation,
 * therefore it needs to be shifted.
 *
 * @param[in] raw_value data read from the device
 * @param[out] magnitude holds the result data after the operation
 */
static inline void tmag5273_magnitude_convert(uint8_t raw_value, const uint16_t range,
					      struct sensor_value *magnitude)
{
	tmag5273_channel_b_field_convert(raw_value << 8, range, magnitude);
}

static int tmag5273_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	CHECKIF(val == NULL) {
		LOG_ERR("val: NULL");
		return -EINVAL;
	}

	const struct tmag5273_config *drv_cfg = dev->config;
	struct tmag5273_data *drv_data = dev->data;

	int8_t val_offset = 0;

	const bool all_mag_axis = (chan == SENSOR_CHAN_MAGN_XYZ) || (chan == SENSOR_CHAN_ALL);

	if ((drv_cfg->axis & TMAG5273_MAG_CH_EN_X) &&
	    (all_mag_axis || (chan == SENSOR_CHAN_MAGN_X))) {
		tmag5273_channel_b_field_convert(drv_data->x_sample, drv_data->xyz_range,
						 val + val_offset);
		val_offset++;
	}

	if ((drv_cfg->axis & TMAG5273_MAG_CH_EN_Y) &&
	    (all_mag_axis || (chan == SENSOR_CHAN_MAGN_Y))) {
		tmag5273_channel_b_field_convert(drv_data->y_sample, drv_data->xyz_range,
						 val + val_offset);
		val_offset++;
	}

	if ((drv_cfg->axis & TMAG5273_MAG_CH_EN_Z) &&
	    (all_mag_axis || (chan == SENSOR_CHAN_MAGN_Z))) {
		tmag5273_channel_b_field_convert(drv_data->z_sample, drv_data->xyz_range,
						 val + val_offset);
		val_offset++;
	}

	if (drv_cfg->temperature && (chan == SENSOR_CHAN_DIE_TEMP)) {
		tmag5273_temperature_convert(drv_data->temperature_sample, val + val_offset);
		val_offset++;
	}

	if (drv_cfg->angle_magnitude_axis != TMAG5273_ANGLE_CALC_NONE) {
		const bool all_calc_ch = (TMAG5273_CHAN_ANGLE_MAGNITUDE == (uint16_t)chan);

		if (all_calc_ch || ((uint16_t)chan == SENSOR_CHAN_ROTATION)) {
			tmag5273_angle_convert(drv_data->angle_sample, val + val_offset);
			val_offset++;
		}

		if (all_calc_ch || ((uint16_t)chan == TMAG5273_CHAN_MAGNITUDE)) {
			tmag5273_magnitude_convert(drv_data->magnitude_sample, drv_data->xyz_range,
						   val + val_offset);
			val_offset++;
		}

		if (all_calc_ch || (uint16_t)chan == TMAG5273_CHAN_MAGNITUDE_MSB) {
			val[val_offset].val1 = drv_data->magnitude_sample;
			val[val_offset].val2 = 0;

			val_offset++;
		}
	}

	if (val_offset == 0) {
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief sets the \c DEVICE_CONFIG_1 and \c DEVICE_CONFIG_2 registers
 *
 * @param dev handle to the current device instance
 *
 * @retval 0 if everything was okay
 * @retval -EIO on communication errors
 */
static inline int tmag5273_init_device_config(const struct device *dev)
{
	const struct tmag5273_config *drv_cfg = dev->config;
	struct tmag5273_data *drv_data = dev->data;
	int retval;
	uint8_t regdata;

	/* REG_DEVICE_CONFIG_1 */
	regdata = 0;

#ifdef CONFIG_CRC
	if (drv_cfg->crc_enabled) {
		regdata |= TMAG5273_CRC_ENABLE;
	}
#endif

	switch (drv_cfg->temperature_coefficient) {
	case TMAG5273_DT_TEMP_COEFF_NDBFE:
		regdata |= TMAG5273_MAGNET_TEMP_COEFF_NDBFE;
		break;
	case TMAG5273_DT_TEMP_COEFF_CERAMIC:
		regdata |= TMAG5273_MAGNET_TEMP_COEFF_CERAMIC;
		break;
	case TMAG5273_DT_TEMP_COEFF_NONE:
		__fallthrough;
	default:
		regdata |= TMAG5273_MAGNET_TEMP_COEFF_NONE;
		break;
	}

	switch (drv_cfg->averaging) {
	case TMAG5273_DT_AVERAGING_2X:
		regdata |= TMAG5273_CONV_AVG_2;
		break;
	case TMAG5273_DT_AVERAGING_4X:
		regdata |= TMAG5273_CONV_AVG_4;
		break;
	case TMAG5273_DT_AVERAGING_8X:
		regdata |= TMAG5273_CONV_AVG_8;
		break;
	case TMAG5273_DT_AVERAGING_16X:
		regdata |= TMAG5273_CONV_AVG_16;
		break;
	case TMAG5273_DT_AVERAGING_32X:
		regdata |= TMAG5273_CONV_AVG_32;
		break;
	case TMAG5273_DT_AVERAGING_NONE:
		__fallthrough;
	default:
		regdata |= TMAG5273_CONV_AVG_1;
		break;
	}

	const int nb_captured_channels =
		((drv_cfg->mag_channel >= TMAG5273_DT_AXIS_XYZ)
			 ? 3
			 : POPCOUNT((drv_cfg->mag_channel & TMAG5273_DT_AXIS_XYZ))) +
		(int)drv_cfg->temperature;

	drv_data->conversion_time_us = TMAG5273_T_CONVERSION_US(
		(FIELD_GET(TMAG5273_CONV_AVB_MSK, regdata)), (nb_captured_channels));

	regdata |= TMAG5273_I2C_READ_MODE_STANDARD;

	retval = i2c_reg_write_byte_dt(&drv_cfg->i2c, TMAG5273_REG_DEVICE_CONFIG_1, regdata);
	if (retval < 0) {
		LOG_ERR("error setting DEVICE_CONFIG_1 %d", retval);
		return -EIO;
	}

	/* REG_DEVICE_CONFIG_2 */
	regdata = 0;

	if (drv_cfg->low_noise_mode) {
		regdata |= TMAG5273_LP_LOWNOISE;
	}

	if (drv_cfg->trigger_conv_via_int) {
		regdata |= TMAG5273_TRIGGER_MODE_INT;
	}

	if (drv_cfg->operation_mode == TMAG5273_DT_OPER_MODE_CONTINUOUS) {
		regdata |= TMAG5273_OPERATING_MODE_CONTINUOUS;
	}

	/* Note: I2C glitch filter enabled by default */

	retval = i2c_reg_write_byte_dt(&drv_cfg->i2c, TMAG5273_REG_DEVICE_CONFIG_2, regdata);
	if (retval < 0) {
		LOG_ERR("error setting DEVICE_CONFIG_2 %d", retval);
		return -EIO;
	}

	return 0;
}

/**
 * @brief sets the \c SENSOR_CONFIG_1 and \c SENSOR_CONFIG_2 registers
 *
 * @param drv_cfg configuration of the TMAG5273 instance
 *
 * @retval 0 if everything was okay
 * @retval -EIO on communication errors
 */
static inline int tmag5273_init_sensor_settings(const struct tmag5273_config *drv_cfg,
						uint8_t version)
{
	int retval;
	uint8_t regdata;

	/* REG_SENSOR_CONFIG_1 */
	regdata = drv_cfg->mag_channel << TMAG5273_MAG_CH_EN_POS;

	retval = i2c_reg_write_byte_dt(&drv_cfg->i2c, TMAG5273_REG_SENSOR_CONFIG_1, regdata);
	if (retval < 0) {
		LOG_ERR("error setting SENSOR_CONFIG_1 %d", retval);
		return -EIO;
	}

	/* REG_SENSOR_CONFIG_2 */
	regdata = 0;

	if (drv_cfg->ch_mag_gain_correction == TMAG5273_DT_CORRECTION_CH_2) {
		regdata |= TMAG5273_MAG_GAIN_CORRECTION_CH_2;
	}

	switch (drv_cfg->angle_magnitude_axis) {
	case TMAG5273_DT_ANGLE_MAG_XY:
		regdata |= TMAG5273_ANGLE_EN_XY;
		break;
	case TMAG5273_DT_ANGLE_MAG_YZ:
		regdata |= TMAG5273_ANGLE_EN_YZ;
		break;
	case TMAG5273_DT_ANGLE_MAG_XZ:
		regdata |= TMAG5273_ANGLE_EN_XZ;
		break;
	case TMAG5273_DT_ANGLE_MAG_RUNTIME:
	case TMAG5273_DT_ANGLE_MAG_NONE:
		__fallthrough;
	default:
		regdata |= TMAG5273_ANGLE_EN_POS;
		break;
	}

	if (drv_cfg->meas_range == TMAG5273_DT_AXIS_RANGE_LOW) {
		regdata |= TMAG5273_XYZ_MEAS_RANGE_LOW;
	} else {
		regdata |= TMAG5273_XYZ_MEAS_RANGE_HIGH;
	}

	retval = i2c_reg_write_byte_dt(&drv_cfg->i2c, TMAG5273_REG_SENSOR_CONFIG_2, regdata);
	if (retval < 0) {
		LOG_ERR("error setting SENSOR_CONFIG_2 %d", retval);
		return -EIO;
	}

	/* the 3001 Variant has REG_CONFIG_3 instead of REG_T_CONFIG. No need for temp enable. */
	if (version == TMAG5273_VER_TMAG3001X1 || version == TMAG5273_VER_TMAG3001X2) {
		return 0;
	}
	/* REG_T_CONFIG */
	regdata = 0;

	if (drv_cfg->temperature) {
		regdata |= TMAG5273_T_CH_EN_ENABLED;
	}

	retval = i2c_reg_write_byte_dt(&drv_cfg->i2c, TMAG5273_REG_T_CONFIG, regdata);
	if (retval < 0) {
		LOG_ERR("error setting SENSOR_CONFIG_2 %d", retval);
		return -EIO;
	}

	return 0;
}

/**
 * @brief initialize a TMAG5273 sensor
 *
 * @param dev handle to the device
 *
 * @retval 0 on success
 * @retval -EINVAL if bus label is invalid
 * @retval -EIO on communication errors
 */
static int tmag5273_init(const struct device *dev)
{
	const struct tmag5273_config *drv_cfg = dev->config;
	struct tmag5273_data *drv_data = dev->data;
	int retval;
	uint8_t regdata;

	if (!i2c_is_ready_dt(&drv_cfg->i2c)) {
		LOG_ERR("could not get pointer to TMAG5273 I2C device");
		return -ENODEV;
	}

	if (drv_cfg->trigger_conv_via_int) {
		if (!gpio_is_ready_dt(&drv_cfg->int_gpio)) {
			LOG_ERR("invalid int-gpio configuration");
			return -ENODEV;
		}

		retval = gpio_pin_configure_dt(&drv_cfg->int_gpio, GPIO_INPUT);
		if (retval < 0) {
			LOG_ERR("cannot configure GPIO %d", retval);
			return -EINVAL;
		}
	}

	retval = i2c_reg_read_byte_dt(&drv_cfg->i2c, TMAG5273_REG_DEVICE_CONFIG_2, &regdata);
	if (retval < 0) {
		LOG_ERR("could not read device config 2 register %d", retval);
		return -EIO;
	}

	LOG_DBG("operation mode: %d", (int)FIELD_GET(TMAG5273_OPERATING_MODE_MSK, regdata));

	retval = i2c_reg_read_byte_dt(&drv_cfg->i2c, TMAG5273_REG_MANUFACTURER_ID_LSB, &regdata);
	if (retval < 0) {
		return -EIO;
	}

	if (regdata != TMAG5273_MANUFACTURER_ID_LSB) {
		LOG_ERR("unexpected manufacturer id LSB 0x%X", regdata);
		return -EINVAL;
	}

	retval = i2c_reg_read_byte_dt(&drv_cfg->i2c, TMAG5273_REG_MANUFACTURER_ID_MSB, &regdata);
	if (retval < 0) {
		LOG_ERR("could not read MSB of manufacturer id %d", retval);
		return -EIO;
	}

	if (regdata != TMAG5273_MANUFACTURER_ID_MSB) {
		LOG_ERR("unexpected manufacturer id MSB 0x%X", regdata);
		return -EINVAL;
	}

	(void)tmag5273_check_device_status(drv_cfg, &regdata);

	retval = tmag5273_reset_device_status(dev);
	if (retval < 0) {
		LOG_ERR("could not reset DEVICE_STATUS register %d", retval);
		return -EIO;
	}

	retval = i2c_reg_read_byte_dt(&drv_cfg->i2c, TMAG5273_REG_DEVICE_ID, &regdata);
	if (retval < 0) {
		LOG_ERR("could not read DEVICE_ID register %d", retval);
		return -EIO;
	}

	drv_data->version = regdata & TMAG5273_VER_MSK;

	/* magnetic measurement range based on version, apply correct one */
	if (drv_cfg->meas_range == TMAG5273_DT_AXIS_RANGE_LOW) {
		drv_data->xyz_range = tmag5273_range_low(drv_data->version);
	} else {
		drv_data->xyz_range = tmag5273_range_high(drv_data->version);
	}

	regdata = TMAG5273_INT_MODE_NONE;

	if (!drv_cfg->trigger_conv_via_int) {
		regdata |= TMAG5273_INT_MASK_INTB_PIN_MASKED;
	}

	retval = i2c_reg_write_byte_dt(&drv_cfg->i2c, TMAG5273_REG_INT_CONFIG_1, regdata);
	if (retval < 0) {
		LOG_ERR("error deactivating interrupts %d", retval);
		return -EIO;
	}

	/* set settings */
	retval = tmag5273_init_sensor_settings(drv_cfg, drv_data->version);
	if (retval < 0) {
		LOG_ERR("error setting sensor configuration %d", retval);
		return retval;
	}

	retval = tmag5273_init_device_config(dev);
	if (retval < 0) {
		LOG_ERR("error setting device configuration %d", retval);
		return retval;
	}

	return 0;
}

static const struct sensor_driver_api tmag5273_driver_api = {
	.attr_set = tmag5273_attr_set,
	.attr_get = tmag5273_attr_get,
	.sample_fetch = tmag5273_sample_fetch,
	.channel_get = tmag5273_channel_get,
};

#define TMAG5273_DT_X_AXIS_BIT(axis_dts)                                                           \
	((((axis_dts & TMAG5273_DT_AXIS_X) == TMAG5273_DT_AXIS_X) ||                               \
	  (axis_dts == TMAG5273_DT_AXIS_XYX) || (axis_dts == TMAG5273_DT_AXIS_YXY) ||              \
	  (axis_dts == TMAG5273_DT_AXIS_XZX))                                                      \
		 ? TMAG5273_MAG_CH_EN_X                                                            \
		 : 0)

#define TMAG5273_DT_Y_AXIS_BIT(axis_dts)                                                           \
	((((axis_dts & TMAG5273_DT_AXIS_Y) == TMAG5273_DT_AXIS_Y) ||                               \
	  (axis_dts == TMAG5273_DT_AXIS_XYX) || (axis_dts == TMAG5273_DT_AXIS_YXY) ||              \
	  (axis_dts == TMAG5273_DT_AXIS_YZY))                                                      \
		 ? TMAG5273_MAG_CH_EN_Y                                                            \
		 : 0)

#define TMAG5273_DT_Z_AXIS_BIT(axis_dts)                                                           \
	((((axis_dts & TMAG5273_DT_AXIS_Z) == TMAG5273_DT_AXIS_Z) ||                               \
	  (axis_dts == TMAG5273_DT_AXIS_YZY) || (axis_dts == TMAG5273_DT_AXIS_XZX))                \
		 ? TMAG5273_MAG_CH_EN_Z                                                            \
		 : 0)

/** Instantiation macro */
#define TMAG5273_DEFINE(inst)                                                                      \
	BUILD_ASSERT(IS_ENABLED(CONFIG_CRC) || (DT_INST_PROP(inst, crc_enabled) == 0),             \
		     "CRC support necessary");                                                     \
	BUILD_ASSERT(!DT_INST_PROP(inst, trigger_conversion_via_int) ||                            \
			     DT_INST_NODE_HAS_PROP(inst, int_gpios),                               \
		     "trigger-conversion-via-int requires int-gpios to be defined");               \
	static const struct tmag5273_config tmag5273_driver_cfg##inst = {                          \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.mag_channel = DT_INST_PROP(inst, axis),                                           \
		.axis = (TMAG5273_DT_X_AXIS_BIT(DT_INST_PROP(inst, axis)) |                        \
			 TMAG5273_DT_Y_AXIS_BIT(DT_INST_PROP(inst, axis)) |                        \
			 TMAG5273_DT_Z_AXIS_BIT(DT_INST_PROP(inst, axis))),                        \
		.temperature = DT_INST_PROP(inst, temperature),                                    \
		.meas_range = DT_INST_PROP(inst, range),                                           \
		.temperature_coefficient = DT_INST_PROP(inst, temperature_coefficient),            \
		.angle_magnitude_axis = DT_INST_PROP(inst, angle_magnitude_axis),                  \
		.ch_mag_gain_correction = DT_INST_PROP(inst, ch_mag_gain_correction),              \
		.operation_mode = DT_INST_PROP(inst, operation_mode),                              \
		.averaging = DT_INST_PROP(inst, average_mode),                                     \
		.trigger_conv_via_int = DT_INST_PROP(inst, trigger_conversion_via_int),            \
		.low_noise_mode = DT_INST_PROP(inst, low_noise),                                   \
		.ignore_diag_fail = DT_INST_PROP(inst, ignore_diag_fail),                          \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                        \
		IF_ENABLED(CONFIG_CRC, (.crc_enabled = DT_INST_PROP(inst, crc_enabled),))};        \
	static struct tmag5273_data tmag5273_driver_data##inst;                                    \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, tmag5273_init, NULL, &tmag5273_driver_data##inst,       \
				     &tmag5273_driver_cfg##inst, POST_KERNEL,                      \
				     CONFIG_SENSOR_INIT_PRIORITY, &tmag5273_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TMAG5273_DEFINE)
