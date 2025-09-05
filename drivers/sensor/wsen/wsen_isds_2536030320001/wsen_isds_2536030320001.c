/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_isds_2536030320001

#include <string.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include <wsen_sensors_common.h>
#include "wsen_isds_2536030320001.h"

LOG_MODULE_REGISTER(WSEN_ISDS_2536030320001, CONFIG_SENSOR_LOG_LEVEL);

/*
 * List of supported accelerometer output data rates (sensor_value struct, input to
 * sensor_attr_set()). Index into this list is used as argument for
 * ISDS_setAccOutputDataRate().
 */
static const struct sensor_value isds_2536030320001_accel_odr_list[] = {
	{.val1 = 0, .val2 = 0},    {.val1 = 12, .val2 = 5 * 100000},
	{.val1 = 26, .val2 = 0},   {.val1 = 52, .val2 = 0},
	{.val1 = 104, .val2 = 0},  {.val1 = 208, .val2 = 0},
	{.val1 = 416, .val2 = 0},  {.val1 = 833, .val2 = 0},
	{.val1 = 1660, .val2 = 0}, {.val1 = 3330, .val2 = 0},
	{.val1 = 6660, .val2 = 0}, {.val1 = 1, .val2 = 6 * 100000},
};

/*
 * List of supported gyroscope output data rates (sensor_value struct, input to
 * sensor_attr_set()). Index into this list is used as argument for
 * ISDS_setGyroOutputDataRate().
 */
static const struct sensor_value isds_2536030320001_gyro_odr_list[] = {
	{.val1 = 0, .val2 = 0},    {.val1 = 12, .val2 = 5 * 100000}, {.val1 = 26, .val2 = 0},
	{.val1 = 52, .val2 = 0},   {.val1 = 104, .val2 = 0},         {.val1 = 208, .val2 = 0},
	{.val1 = 416, .val2 = 0},  {.val1 = 833, .val2 = 0},         {.val1 = 1660, .val2 = 0},
	{.val1 = 3330, .val2 = 0}, {.val1 = 6660, .val2 = 0},
};

/*
 * List of supported accelerometer full scale values (i.e. measurement ranges, in g).
 * Index into this list is used as input for ISDS_setAccFullScale().
 */
static const uint8_t isds_2536030320001_accel_full_scale_list[] = {
	2,
	16,
	4,
	8,
};

/*
 * List of supported gyroscope full scale values (i.e. measurement ranges, in dps).
 * Index into this list is used as input for ISDS_setGyroFullScale().
 */
static const uint16_t isds_2536030320001_gyro_full_scale_list[] = {
	250, 125, 500, 0, 1000, 0, 2000,
};

static int isds_2536030320001_sample_fetch(const struct device *dev, enum sensor_channel channel)
{
	struct isds_2536030320001_data *data = dev->data;

	uint32_t accel_step_sleep_duration, gyro_step_sleep_duration, step_sleep_duration;

	switch (channel) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_AMBIENT_TEMP: {
		if (!wsen_sensor_step_sleep_duration_milli_from_odr_hz(
			    &isds_2536030320001_accel_odr_list[data->accel_odr],
			    &accel_step_sleep_duration)) {
			LOG_ERR("Accelerometer is disabled.");
			return -ENOTSUP;
		}

		if (!wsen_sensor_step_sleep_duration_milli_from_odr_hz(
			    &isds_2536030320001_gyro_odr_list[data->gyro_odr],
			    &gyro_step_sleep_duration)) {
			LOG_ERR("Gyroscope is disabled.");
			return -ENOTSUP;
		}

		step_sleep_duration = accel_step_sleep_duration < gyro_step_sleep_duration
					      ? gyro_step_sleep_duration
					      : accel_step_sleep_duration;
		break;
	}
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ: {
		if (!wsen_sensor_step_sleep_duration_milli_from_odr_hz(
			    &isds_2536030320001_accel_odr_list[data->accel_odr],
			    &step_sleep_duration)) {
			LOG_ERR("Accelerometer is disabled.");
			return -ENOTSUP;
		}
		break;
	}
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ: {
		if (!wsen_sensor_step_sleep_duration_milli_from_odr_hz(
			    &isds_2536030320001_gyro_odr_list[data->gyro_odr],
			    &step_sleep_duration)) {
			LOG_ERR("Gyroscope is disabled.");
			return -ENOTSUP;
		}
		break;
	}
	default:
		LOG_ERR("Fetching is not supported on channel %d.", channel);
		return -ENOTSUP;
	}

	ISDS_state_t acceleration_data_ready, gyro_data_ready, temp_data_ready;

	acceleration_data_ready = gyro_data_ready = temp_data_ready = ISDS_disable;

	bool data_ready = false;
	int step_count = 0;

	while (1) {
		switch (channel) {
		case SENSOR_CHAN_ALL: {
			if (ISDS_isAccelerationDataReady(&data->sensor_interface,
							 &acceleration_data_ready) != WE_SUCCESS) {
				LOG_ERR("Failed to check if acceleration data is ready.");
				return -EIO;
			}

			if (ISDS_isGyroscopeDataReady(&data->sensor_interface, &gyro_data_ready) !=
			    WE_SUCCESS) {
				LOG_ERR("Failed to check if gyroscope data is ready.");
				return -EIO;
			}

			if (ISDS_isTemperatureDataReady(&data->sensor_interface,
							&temp_data_ready) != WE_SUCCESS) {
				LOG_ERR("Failed to check if temperature data is ready.");
				return -EIO;
			}

			data_ready =
				(acceleration_data_ready == ISDS_enable &&
				 gyro_data_ready == ISDS_enable && temp_data_ready == ISDS_enable);
			break;
		}
		case SENSOR_CHAN_AMBIENT_TEMP: {
			if (ISDS_isTemperatureDataReady(&data->sensor_interface,
							&temp_data_ready) != WE_SUCCESS) {
				LOG_ERR("Failed to check if temperature data is ready.");
				return -EIO;
			}
			data_ready = (temp_data_ready == ISDS_enable);
			break;
		}
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ: {
			if (ISDS_isAccelerationDataReady(&data->sensor_interface,
							 &acceleration_data_ready) != WE_SUCCESS) {
				LOG_ERR("Failed to check if acceleration data is ready.");
				return -EIO;
			}

			data_ready = (acceleration_data_ready == ISDS_enable);
			break;
		}
		case SENSOR_CHAN_GYRO_X:
		case SENSOR_CHAN_GYRO_Y:
		case SENSOR_CHAN_GYRO_Z:
		case SENSOR_CHAN_GYRO_XYZ: {
			if (ISDS_isGyroscopeDataReady(&data->sensor_interface, &gyro_data_ready) !=
			    WE_SUCCESS) {
				LOG_ERR("Failed to check if gyroscope data is ready.");
				return -EIO;
			}

			data_ready = (gyro_data_ready == ISDS_enable);
			break;
		}
		default:
			break;
		}

		if (data_ready) {
			break;
		} else if (step_count >= MAX_POLL_STEP_COUNT) {
			return -EIO;
		}

		step_count++;
		k_sleep(K_USEC(step_sleep_duration));
	}

	int16_t temperature, acceleration_x, acceleration_y, acceleration_z, gyro_x, gyro_y, gyro_z;

	switch (channel) {
	case SENSOR_CHAN_ALL: {

		if (ISDS_getRawAccelerations(&data->sensor_interface, &acceleration_x,
					     &acceleration_y, &acceleration_z) != WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "acceleration");
			return -EIO;
		}

		if (ISDS_getRawAngularRates(&data->sensor_interface, &gyro_x, &gyro_y, &gyro_z) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "gyro");
			return -EIO;
		}

		if (ISDS_getRawTemperature(&data->sensor_interface, &temperature) != WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "temperature");
			return -EIO;
		}

		data->acceleration_x =
			ISDS_convertAcceleration_int(acceleration_x, data->accel_range);
		data->acceleration_y =
			ISDS_convertAcceleration_int(acceleration_y, data->accel_range);
		data->acceleration_z =
			ISDS_convertAcceleration_int(acceleration_z, data->accel_range);

		data->rate_x = ISDS_convertAngularRate_int(gyro_x, data->gyro_range);
		data->rate_y = ISDS_convertAngularRate_int(gyro_y, data->gyro_range);
		data->rate_z = ISDS_convertAngularRate_int(gyro_z, data->gyro_range);

		data->temperature = ISDS_convertTemperature_int(temperature);
		break;
	}
	case SENSOR_CHAN_AMBIENT_TEMP: {
		if (ISDS_getRawTemperature(&data->sensor_interface, &temperature) != WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "temperature");
			return -EIO;
		}
		data->temperature = ISDS_convertTemperature_int(temperature);
		break;
	}
	case SENSOR_CHAN_ACCEL_X: {
		if (ISDS_getRawAccelerationX(&data->sensor_interface, &acceleration_x) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "acceleration");
			return -EIO;
		}
		data->acceleration_x =
			ISDS_convertAcceleration_int(acceleration_x, data->accel_range);
		break;
	}
	case SENSOR_CHAN_ACCEL_Y: {
		if (ISDS_getRawAccelerationY(&data->sensor_interface, &acceleration_y) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "acceleration");
			return -EIO;
		}
		data->acceleration_y =
			ISDS_convertAcceleration_int(acceleration_y, data->accel_range);
		break;
	}
	case SENSOR_CHAN_ACCEL_Z: {
		if (ISDS_getRawAccelerationZ(&data->sensor_interface, &acceleration_z) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "acceleration");
			return -EIO;
		}
		data->acceleration_z =
			ISDS_convertAcceleration_int(acceleration_z, data->accel_range);
		break;
	}
	case SENSOR_CHAN_ACCEL_XYZ: {
		if (ISDS_getRawAccelerations(&data->sensor_interface, &acceleration_x,
					     &acceleration_y, &acceleration_z) != WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "acceleration");
			return -EIO;
		}

		data->acceleration_x =
			ISDS_convertAcceleration_int(acceleration_x, data->accel_range);
		data->acceleration_y =
			ISDS_convertAcceleration_int(acceleration_y, data->accel_range);
		data->acceleration_z =
			ISDS_convertAcceleration_int(acceleration_z, data->accel_range);
		break;
	}
	case SENSOR_CHAN_GYRO_X: {
		if (ISDS_getRawAngularRateX(&data->sensor_interface, &gyro_x) != WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "gyro");
			return -EIO;
		}
		data->rate_x = ISDS_convertAngularRate_int(gyro_x, data->gyro_range);
		break;
	}
	case SENSOR_CHAN_GYRO_Y: {
		if (ISDS_getRawAngularRateY(&data->sensor_interface, &gyro_y) != WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "gyro");
			return -EIO;
		}
		data->rate_y = ISDS_convertAngularRate_int(gyro_y, data->gyro_range);
		break;
	}
	case SENSOR_CHAN_GYRO_Z: {
		if (ISDS_getRawAngularRateZ(&data->sensor_interface, &gyro_z) != WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "gyro");
			return -EIO;
		}
		data->rate_z = ISDS_convertAngularRate_int(gyro_z, data->gyro_range);
		break;
	}
	case SENSOR_CHAN_GYRO_XYZ: {
		if (ISDS_getRawAngularRates(&data->sensor_interface, &gyro_x, &gyro_y, &gyro_z) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "gyro");
			return -EIO;
		}

		data->rate_x = ISDS_convertAngularRate_int(gyro_x, data->gyro_range);
		data->rate_y = ISDS_convertAngularRate_int(gyro_y, data->gyro_range);
		data->rate_z = ISDS_convertAngularRate_int(gyro_z, data->gyro_range);
		break;
	}
	default:
		break;
	}

	return 0;
}

/* Convert acceleration value from mg (int16) to m/s^2 (sensor_value). */
static inline void isds_2536030320001_convert_acceleration(struct sensor_value *val,
							   int16_t raw_val)
{
	int64_t dval;

	/* Convert to m/s^2 */
	dval = (((int64_t)raw_val) * SENSOR_G) / 1000000LL;
	val->val1 = dval / 1000LL;
	val->val2 = (dval % 1000LL) * 1000;
}

/* Convert angular rate value from mdps (int32) to radians/s (sensor_value). */
static inline void isds_2536030320001_convert_angular_rate(struct sensor_value *val,
							   int32_t raw_val)
{
	int64_t dval;

	/* Convert to radians/s */
	dval = ((((int64_t)raw_val) * SENSOR_PI) / 180000000LL);
	val->val1 = dval / 1000LL;
	val->val2 = (dval % 1000LL) * 1000;
}

static int isds_2536030320001_channel_get(const struct device *dev, enum sensor_channel channel,
					  struct sensor_value *value)
{
	struct isds_2536030320001_data *data = dev->data;

	switch (channel) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* Convert temperature from 0.01 degrees Celsius to degrees Celsius */
		value->val1 = (int32_t)data->temperature / 100;
		value->val2 = ((int32_t)data->temperature % 100) * (1000000 / 100);
		break;
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		/* Convert requested acceleration(s) */
		if (channel == SENSOR_CHAN_ACCEL_X || channel == SENSOR_CHAN_ACCEL_XYZ) {
			isds_2536030320001_convert_acceleration(value, data->acceleration_x);
			value++;
		}
		if (channel == SENSOR_CHAN_ACCEL_Y || channel == SENSOR_CHAN_ACCEL_XYZ) {
			isds_2536030320001_convert_acceleration(value, data->acceleration_y);
			value++;
		}
		if (channel == SENSOR_CHAN_ACCEL_Z || channel == SENSOR_CHAN_ACCEL_XYZ) {
			isds_2536030320001_convert_acceleration(value, data->acceleration_z);
			value++;
		}
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		if (channel == SENSOR_CHAN_GYRO_X || channel == SENSOR_CHAN_GYRO_XYZ) {
			isds_2536030320001_convert_angular_rate(value, data->rate_x);
			value++;
		}
		if (channel == SENSOR_CHAN_GYRO_Y || channel == SENSOR_CHAN_GYRO_XYZ) {
			isds_2536030320001_convert_angular_rate(value, data->rate_y);
			value++;
		}
		if (channel == SENSOR_CHAN_GYRO_Z || channel == SENSOR_CHAN_GYRO_XYZ) {
			isds_2536030320001_convert_angular_rate(value, data->rate_z);
			value++;
		}
		break;
	default:
		LOG_ERR("Channel not supported %d", channel);
		return -ENOTSUP;
	}

	return 0;
}

/* Set accelerometer output data rate. See isds_2536030320001_accel_odr_list for allowed values. */
static int isds_2536030320001_accel_odr_set(const struct device *dev,
					    const struct sensor_value *odr)
{
	struct isds_2536030320001_data *data = dev->data;
	int odr_index;

	for (odr_index = 0; odr_index < ARRAY_SIZE(isds_2536030320001_accel_odr_list);
	     odr_index++) {
		if (odr->val1 == isds_2536030320001_accel_odr_list[odr_index].val1 &&
		    odr->val2 == isds_2536030320001_accel_odr_list[odr_index].val2) {
			break;
		}
	}

	if (odr_index == ARRAY_SIZE(isds_2536030320001_accel_odr_list)) {
		/* ODR not allowed (was not found in isds_2536030320001_accel_odr_list) */
		LOG_ERR("Bad sampling frequency %d.%d", odr->val1, odr->val2);
		return -EINVAL;
	}

	if (ISDS_setAccOutputDataRate(&data->sensor_interface,
				      (ISDS_accOutputDataRate_t)odr_index) != WE_SUCCESS) {
		LOG_ERR("Failed to set accelerometer output data rate");
		return -EIO;
	}

	data->accel_odr = (ISDS_accOutputDataRate_t)odr_index;

	return 0;
}

/* Get accelerometer output data rate. See isds_2536030320001_accel_odr_list for allowed values. */
static int isds_2536030320001_accel_odr_get(const struct device *dev, struct sensor_value *odr)
{
	struct isds_2536030320001_data *data = dev->data;
	ISDS_accOutputDataRate_t odr_index;

	if (ISDS_getAccOutputDataRate(&data->sensor_interface, &odr_index) != WE_SUCCESS) {
		LOG_ERR("Failed to get output data rate");
		return -EIO;
	}

	data->accel_odr = odr_index;

	odr->val1 = isds_2536030320001_accel_odr_list[odr_index].val1;
	odr->val2 = isds_2536030320001_accel_odr_list[odr_index].val2;

	return 0;
}

/* Set gyroscope output data rate. See isds_2536030320001_gyro_odr_list for allowed values. */
static int isds_2536030320001_gyro_odr_set(const struct device *dev, const struct sensor_value *odr)
{
	struct isds_2536030320001_data *data = dev->data;
	int odr_index;

	for (odr_index = 0; odr_index < ARRAY_SIZE(isds_2536030320001_gyro_odr_list); odr_index++) {
		if (odr->val1 == isds_2536030320001_gyro_odr_list[odr_index].val1 &&
		    odr->val2 == isds_2536030320001_gyro_odr_list[odr_index].val2) {
			break;
		}
	}

	if (odr_index == ARRAY_SIZE(isds_2536030320001_gyro_odr_list)) {
		/* ODR not allowed (was not found in isds_2536030320001_gyro_odr_list) */
		LOG_ERR("Bad sampling frequency %d.%d", odr->val1, odr->val2);
		return -EINVAL;
	}

	if (ISDS_setGyroOutputDataRate(&data->sensor_interface,
				       (ISDS_gyroOutputDataRate_t)odr_index) != WE_SUCCESS) {
		LOG_ERR("Failed to set gyroscope output data rate");
		return -EIO;
	}

	data->gyro_odr = (ISDS_gyroOutputDataRate_t)odr_index;

	return 0;
}

/* Get gyroscope output data rate. See isds_2536030320001_gyro_odr_list for allowed values. */
static int isds_2536030320001_gyro_odr_get(const struct device *dev, struct sensor_value *odr)
{
	struct isds_2536030320001_data *data = dev->data;
	ISDS_gyroOutputDataRate_t odr_index;

	if (ISDS_getGyroOutputDataRate(&data->sensor_interface, &odr_index) != WE_SUCCESS) {
		LOG_ERR("Failed to get output data rate");
		return -EIO;
	}

	data->gyro_odr = odr_index;

	odr->val1 = isds_2536030320001_gyro_odr_list[odr_index].val1;
	odr->val2 = isds_2536030320001_gyro_odr_list[odr_index].val2;

	return 0;
}

/*
 * Set accelerometer full scale (measurement range). See isds_2536030320001_accel_full_scale_list
 * for allowed values.
 */
static int isds_2536030320001_accel_full_scale_set(const struct device *dev,
						   const struct sensor_value *fs)
{
	struct isds_2536030320001_data *data = dev->data;

	uint8_t scaleg = (uint8_t)sensor_ms2_to_g(fs);

	uint8_t idx;

	for (idx = 0; idx < ARRAY_SIZE(isds_2536030320001_accel_full_scale_list); idx++) {
		if (isds_2536030320001_accel_full_scale_list[idx] == scaleg) {
			break;
		}
	}

	if (idx == ARRAY_SIZE(isds_2536030320001_accel_full_scale_list)) {
		/* fullscale not allowed (was not found in isds_2536030320001_accel_full_scale_list)
		 */
		LOG_ERR("Bad scale %d", scaleg);
		return -EINVAL;
	}

	if (ISDS_setAccFullScale(&data->sensor_interface, (ISDS_accFullScale_t)idx) != WE_SUCCESS) {
		LOG_ERR("Failed to set accelerometer full scale.");
		return -EIO;
	}

	data->accel_range = (ISDS_accFullScale_t)idx;

	return 0;
}

/*
 * Get accelerometer full scale (measurement range). See isds_2536030320001_accel_full_scale_list
 * for allowed values.
 */
static int isds_2536030320001_accel_full_scale_get(const struct device *dev,
						   struct sensor_value *fs)
{
	struct isds_2536030320001_data *data = dev->data;

	ISDS_accFullScale_t accel_fs;

	if (ISDS_getAccFullScale(&data->sensor_interface, &accel_fs) != WE_SUCCESS) {
		LOG_ERR("Failed to get full scale");
		return -EIO;
	}

	data->accel_range = accel_fs;

	fs->val1 = isds_2536030320001_accel_full_scale_list[accel_fs];
	fs->val2 = 0;

	return 0;
}

/*
 * Set gyroscope full scale (measurement range). See isds_2536030320001_gyro_full_scale_list for
 * allowed values.
 */
static int isds_2536030320001_gyro_full_scale_set(const struct device *dev,
						  const struct sensor_value *fs)
{
	struct isds_2536030320001_data *data = dev->data;

	uint16_t scale_dps = (uint16_t)sensor_rad_to_degrees(fs);

	uint8_t idx;

	for (idx = 0; idx < ARRAY_SIZE(isds_2536030320001_gyro_full_scale_list); idx++) {
		if (isds_2536030320001_gyro_full_scale_list[idx] == scale_dps) {
			break;
		}
	}

	if (idx == ARRAY_SIZE(isds_2536030320001_gyro_full_scale_list)) {
		/* fullscale not allowed (was not found in isds_2536030320001_gyro_full_scale_list)
		 */
		LOG_ERR("Bad scale %d", scale_dps);
		return -EINVAL;
	}

	if (ISDS_setGyroFullScale(&data->sensor_interface, (ISDS_gyroFullScale_t)idx) !=
	    WE_SUCCESS) {
		LOG_ERR("Failed to set gyroscope full scale.");
		return -EIO;
	}

	data->gyro_range = (ISDS_gyroFullScale_t)idx;

	return 0;
}

/*
 * Get gyroscope full scale (measurement range). See isds_2536030320001_gyro_full_scale_list for
 * allowed values.
 */
static int isds_2536030320001_gyro_full_scale_get(const struct device *dev, struct sensor_value *fs)
{
	struct isds_2536030320001_data *data = dev->data;

	ISDS_gyroFullScale_t gyro_fs;

	if (ISDS_getGyroFullScale(&data->sensor_interface, &gyro_fs) != WE_SUCCESS) {
		LOG_ERR("Failed to get full scale");
		return -EIO;
	}

	data->gyro_range = gyro_fs;

	fs->val1 = isds_2536030320001_gyro_full_scale_list[gyro_fs];
	fs->val2 = 0;

	return 0;
}

static int isds_2536030320001_attr_set(const struct device *dev, enum sensor_channel chan,
				       enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		switch (chan) {
		case SENSOR_CHAN_ACCEL_XYZ:
			return isds_2536030320001_accel_odr_set(dev, val);
		case SENSOR_CHAN_GYRO_XYZ:
			return isds_2536030320001_gyro_odr_set(dev, val);
		default:
			break;
		}
		break;
	case SENSOR_ATTR_FULL_SCALE:
		switch (chan) {
		case SENSOR_CHAN_ACCEL_XYZ:
			return isds_2536030320001_accel_full_scale_set(dev, val);
		case SENSOR_CHAN_GYRO_XYZ:
			return isds_2536030320001_gyro_full_scale_set(dev, val);
		default:
			break;
		}
		break;
	default:
		break;
	}

	LOG_ERR("attr_set() is not supported on channel %d.", chan);
	return -ENOTSUP;
}

static int isds_2536030320001_attr_get(const struct device *dev, enum sensor_channel chan,
				       enum sensor_attribute attr, struct sensor_value *val)
{

	if (val == NULL) {
		LOG_WRN("address of passed value is NULL.");
		return -EFAULT;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		switch (chan) {
		case SENSOR_CHAN_ACCEL_XYZ:
			return isds_2536030320001_accel_odr_get(dev, val);
		case SENSOR_CHAN_GYRO_XYZ:
			return isds_2536030320001_gyro_odr_get(dev, val);
		default:
			break;
		}
		break;
	case SENSOR_ATTR_FULL_SCALE:
		switch (chan) {
		case SENSOR_CHAN_ACCEL_XYZ:
			return isds_2536030320001_accel_full_scale_get(dev, val);
		case SENSOR_CHAN_GYRO_XYZ:
			return isds_2536030320001_gyro_full_scale_get(dev, val);
		default:
			break;
		}
		break;
	default:
		break;
	}

	LOG_ERR("attr_get() is not supported on channel %d.", chan);
	return -ENOTSUP;
}

static DEVICE_API(sensor, isds_2536030320001_driver_api) = {
	.attr_set = isds_2536030320001_attr_set,
	.attr_get = isds_2536030320001_attr_get,
#if CONFIG_WSEN_ISDS_2536030320001_TRIGGER
	.trigger_set = isds_2536030320001_trigger_set,
#endif
	.sample_fetch = isds_2536030320001_sample_fetch,
	.channel_get = isds_2536030320001_channel_get,
};

static int isds_2536030320001_init(const struct device *dev)
{
	const struct isds_2536030320001_config *config = dev->config;
	struct isds_2536030320001_data *data = dev->data;
	struct sensor_value accel_range, gyro_range;
	uint8_t device_id;
	ISDS_state_t sw_reset;

	/* Initialize WE sensor interface */
	WE_sensorInterfaceType_t interface_type = data->sensor_interface.interfaceType;

	ISDS_getDefaultInterface(&data->sensor_interface);
	data->sensor_interface.interfaceType = interface_type;

	switch (data->sensor_interface.interfaceType) {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	case WE_i2c:
		if (!i2c_is_ready_dt(&config->bus_cfg.i2c)) {
			LOG_ERR("I2C bus device not ready");
			return -ENODEV;
		}
		data->sensor_interface.handle = (void *)&config->bus_cfg.i2c;
		break;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	case WE_spi:
		if (!spi_is_ready_dt(&config->bus_cfg.spi)) {
			LOG_ERR("SPI bus device not ready");
			return -ENODEV;
		}
		data->sensor_interface.handle = (void *)&config->bus_cfg.spi;
		break;
#endif
	default:
		LOG_ERR("Invalid interface type");
		return -EINVAL;
	}

	/* First communication test - check device ID */
	if (ISDS_getDeviceID(&data->sensor_interface, &device_id) != WE_SUCCESS) {
		LOG_ERR("Failed to read device ID.");
		return -EIO;
	}

	if (device_id != ISDS_DEVICE_ID_VALUE) {
		LOG_ERR("Invalid device ID 0x%x.", device_id);
		return -EINVAL;
	}

	/* Perform soft reset of the sensor */
	ISDS_softReset(&data->sensor_interface, ISDS_enable);

	k_sleep(K_USEC(5));

	do {
		if (ISDS_getSoftResetState(&data->sensor_interface, &sw_reset) != WE_SUCCESS) {
			LOG_ERR("Failed to get sensor reset state.");
			return -EIO;
		}
	} while (sw_reset);

	if (isds_2536030320001_accel_odr_set(
		    dev, &isds_2536030320001_accel_odr_list[config->accel_odr]) < 0) {
		LOG_ERR("Failed to set odr");
		return -EIO;
	}

	if (isds_2536030320001_gyro_odr_set(
		    dev, &isds_2536030320001_gyro_odr_list[config->gyro_odr]) < 0) {
		LOG_ERR("Failed to set odr");
		return -EIO;
	}

	if (ISDS_enableAutoIncrement(&data->sensor_interface, ISDS_enable) != WE_SUCCESS) {
		LOG_ERR("Failed to enable auto increment.");
		return -EIO;
	}

	if (ISDS_enableBlockDataUpdate(&data->sensor_interface, ISDS_enable) != WE_SUCCESS) {
		LOG_ERR("Failed to enable block data update.");
		return -EIO;
	}

	sensor_g_to_ms2((int32_t)config->accel_range, &accel_range);

	if (isds_2536030320001_accel_full_scale_set(dev, &accel_range) < 0) {
		LOG_ERR("Failed to set full scale");
		return -EIO;
	}

	sensor_degrees_to_rad((int32_t)config->gyro_range, &gyro_range);

	if (isds_2536030320001_gyro_full_scale_set(dev, &gyro_range) < 0) {
		LOG_ERR("Failed to set full scale");
		return -EIO;
	}

#if CONFIG_WSEN_ISDS_2536030320001_DISABLE_ACCEL_HIGH_PERFORMANCE_MODE
	if (ISDS_disableAccHighPerformanceMode(&data->sensor_interface, ISDS_enable) !=
	    WE_SUCCESS) {
		LOG_ERR("Failed to disable accelerometer high performance mode.");
		return -EIO;
	}
#endif /* CONFIG_WSEN_ISDS_2536030320001_DISABLE_ACCEL_HIGH_PERFORMANCE_MODE */

#if CONFIG_WSEN_ISDS_2536030320001_DISABLE_GYRO_HIGH_PERFORMANCE_MODE
	if (ISDS_disableGyroHighPerformanceMode(&data->sensor_interface, ISDS_enable) !=
	    WE_SUCCESS) {
		LOG_ERR("Failed to disable gyroscope high performance mode.");
		return -EIO;
	}
#endif /* CONFIG_WSEN_ISDS_2536030320001_DISABLE_GYRO_HIGH_PERFORMANCE_MODE */

#if CONFIG_WSEN_ISDS_2536030320001_TRIGGER
	if (isds_2536030320001_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt(s).");
		return -EIO;
	}
#endif

	return 0;
}

#ifdef CONFIG_WSEN_ISDS_2536030320001_TRIGGER
#define ISDS_2536030320001_CFG_EVENTS_IRQ(inst)                                                    \
	.events_interrupt_gpio = GPIO_DT_SPEC_INST_GET(inst, events_interrupt_gpios),
#define ISDS_2536030320001_CFG_DRDY_IRQ(inst)                                                      \
	.drdy_interrupt_gpio = GPIO_DT_SPEC_INST_GET(inst, drdy_interrupt_gpios),
#else
#define ISDS_2536030320001_CFG_EVENTS_IRQ(inst)
#define ISDS_2536030320001_CFG_DRDY_IRQ(inst)
#endif /* CONFIG_WSEN_ISDS_2536030320001_TRIGGER */

#ifdef CONFIG_WSEN_ISDS_2536030320001_TAP
#define ISDS_2536030320001_CONFIG_TAP(inst)                                                        \
	.tap_mode = DT_INST_PROP(inst, tap_mode),                                                  \
	.tap_threshold = DT_INST_PROP(inst, tap_threshold),                                        \
	.tap_axis_enable = DT_INST_PROP(inst, tap_axis_enable),                                    \
	.tap_shock = DT_INST_PROP(inst, tap_shock),                                                \
	.tap_latency = DT_INST_PROP(inst, tap_latency),                                            \
	.tap_quiet = DT_INST_PROP(inst, tap_quiet),
#else
#define ISDS_2536030320001_CONFIG_TAP(inst)
#endif /* CONFIG_WSEN_ISDS_2536030320001_TAP */

#ifdef CONFIG_WSEN_ISDS_2536030320001_FREEFALL
#define ISDS_2536030320001_CONFIG_FREEFALL(inst)                                                   \
	.freefall_duration = DT_INST_PROP(inst, freefall_duration),                                \
	.freefall_threshold =                                                                      \
		(ISDS_freeFallThreshold_t)DT_INST_ENUM_IDX(inst, freefall_threshold),
#else
#define ISDS_2536030320001_CONFIG_FREEFALL(inst)
#endif /* CONFIG_WSEN_ISDS_2536030320001_FREEFALL */

#ifdef CONFIG_WSEN_ISDS_2536030320001_DELTA
#define ISDS_2536030320001_CONFIG_DELTA(inst)                                                      \
	.delta_threshold = DT_INST_PROP(inst, delta_threshold),                                    \
	.delta_duration = DT_INST_PROP(inst, delta_duration),
#else
#define ISDS_2536030320001_CONFIG_DELTA(inst)
#endif /* CONFIG_WSEN_ISDS_2536030320001_DELTA */

/* clang-format off */

#define ISDS_2536030320001_CONFIG_COMMON(inst)	                      \
	.accel_odr = (ISDS_accOutputDataRate_t)(DT_INST_ENUM_IDX(inst, accel_odr)), \
	.gyro_odr = (ISDS_gyroOutputDataRate_t)(DT_INST_ENUM_IDX(inst, gyro_odr)),  \
	.accel_range = DT_INST_PROP(inst, accel_range),                    \
	.gyro_range = DT_INST_PROP(inst, gyro_range),                      \
	ISDS_2536030320001_CONFIG_TAP(inst)                                \
	ISDS_2536030320001_CONFIG_FREEFALL(inst)                           \
	ISDS_2536030320001_CONFIG_DELTA(inst)                              \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, events_interrupt_gpios),   \
				(ISDS_2536030320001_CFG_EVENTS_IRQ(inst)), ())         \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, drdy_interrupt_gpios),     \
				(ISDS_2536030320001_CFG_DRDY_IRQ(inst)), ())

/* clang-format on */

/*
 * Instantiation macros used when device is on SPI bus.
 */

#define ISDS_2536030320001_SPI_OPERATION                                                           \
	(SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA)

#define ISDS_2536030320001_CONFIG_SPI(inst)                                                        \
	{.bus_cfg =                                                                                \
		 {                                                                                 \
			 .spi = SPI_DT_SPEC_INST_GET(inst, ISDS_2536030320001_SPI_OPERATION, 0),   \
		 },                                                                                \
	 ISDS_2536030320001_CONFIG_COMMON(inst)}

/*
 * Instantiation macros used when device is on I2C bus.
 */

#define ISDS_2536030320001_CONFIG_I2C(inst)                                                        \
	{.bus_cfg =                                                                                \
		 {                                                                                 \
			 .i2c = I2C_DT_SPEC_INST_GET(inst),                                        \
		 },                                                                                \
	 ISDS_2536030320001_CONFIG_COMMON(inst)}

/* clang-format off */

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */
#define ISDS_2536030320001_DEFINE(inst)                                           \
	static struct isds_2536030320001_data isds_2536030320001_data_##inst =       \
		COND_CODE_1(DT_INST_ON_BUS(inst, i2c),                                    \
			({ .sensor_interface = { .interfaceType = WE_i2c } }), ())           \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),                                    \
			({ .sensor_interface = { .interfaceType = WE_spi } }), ());          \
	static const struct isds_2536030320001_config isds_2536030320001_config_##inst = \
		COND_CODE_1(DT_INST_ON_BUS(inst, i2c),                                    \
			(ISDS_2536030320001_CONFIG_I2C(inst)), ())                           \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),                                    \
			(ISDS_2536030320001_CONFIG_SPI(inst)), ());                          \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,                                           \
		isds_2536030320001_init,                                                 \
		NULL,                                                                    \
		&isds_2536030320001_data_##inst,                                         \
		&isds_2536030320001_config_##inst,                                       \
		POST_KERNEL,                                                             \
		CONFIG_SENSOR_INIT_PRIORITY,                                             \
		&isds_2536030320001_driver_api)

/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(ISDS_2536030320001_DEFINE)
