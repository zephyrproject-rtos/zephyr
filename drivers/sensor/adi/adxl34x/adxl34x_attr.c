/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/adxl34x.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

#include "adxl34x_reg.h"
#include "adxl34x_private.h"
#include "adxl34x_attr.h"

LOG_MODULE_DECLARE(adxl34x, CONFIG_SENSOR_LOG_LEVEL);

static const uint16_t offset_ug_lsb = 15600;

/**
 * @brief Converstion array to convert frequencies from sensor-values to its enumerated
 *        values, and back.
 *
 * @var reg_to_hz_conv
 */
static const struct sensor_value reg_to_hz_conv[] = {
	/* clang-format off */
	[ADXL34X_ACCEL_FREQ_0_10] =    {0, 100000},
	[ADXL34X_ACCEL_FREQ_0_20] =    {0, 200000},
	[ADXL34X_ACCEL_FREQ_0_39] =    {0, 390000},
	[ADXL34X_ACCEL_FREQ_0_78] =    {0, 780000},
	[ADXL34X_ACCEL_FREQ_1_56] =    {1, 560000},
	[ADXL34X_ACCEL_FREQ_3_13] =    {3, 130000},
	[ADXL34X_ACCEL_FREQ_6_25] =    {6, 250000},
	[ADXL34X_ACCEL_FREQ_12_5] =   {12, 500000},
	[ADXL34X_ACCEL_FREQ_25]   =   {25, 0},
	[ADXL34X_ACCEL_FREQ_50]   =   {50, 0},
	[ADXL34X_ACCEL_FREQ_100]  =  {100, 0},
	[ADXL34X_ACCEL_FREQ_200]  =  {200, 0},
	[ADXL34X_ACCEL_FREQ_400]  =  {400, 0},
	[ADXL34X_ACCEL_FREQ_800]  =  {800, 0},
	[ADXL34X_ACCEL_FREQ_1600] = {1600, 0},
	[ADXL34X_ACCEL_FREQ_3200] = {3200, 0},
	/* clang-format on */
};

/**
 * @brief Converstion array to convert accelero range from sensor-values to its
 *        enumerated values, and back.
 *
 * @var reg_to_range_conv
 */
static const struct sensor_value reg_to_range_conv[] = {
	[ADXL34X_RANGE_2G] = {2, 0},
	[ADXL34X_RANGE_4G] = {4, 0},
	[ADXL34X_RANGE_8G] = {8, 0},
	[ADXL34X_RANGE_16G] = {16, 0},
};

/**
 * Convert a single number to a sensor value
 *
 * @param[in] u_value The number to convert times 1000000
 * @param[out] out Pointer to store the converted number
 */
static void sensor_value_from_u_value(int64_t u_value, struct sensor_value *out)
{
	__ASSERT_NO_MSG(out != NULL);
	out->val1 = u_value / 100000LL;
	out->val2 = u_value - out->val1 * 1000000LL;
}

/**
 * Convert a sensor_value to an enumeration
 *
 * @param[in] in The sensor value to convert
 * @param[in] conv An array of sensor_values to lookup the enumeration
 * @param[in] max The maximum index of the enumeration
 * @return 0 if successful, negative errno code if failure.
 */
static int sensor_value_to_enum(const struct sensor_value *in, const struct sensor_value conv[],
				const int max)
{
	int index = max;

	while (index >= 0) {
		if (conv[index].val1 < in->val1) {
			return index;
		} else if (conv[index].val1 == in->val1 && conv[index].val2 <= in->val2) {
			return index;
		}
		index--;
	}
	return 0;
}

/**
 * Convert an enumeration to a sensor value
 *
 * @param[in] value The enumerated value
 * @param[in] conv An array of sensor_values to lookup the enumeration
 * @param[out] out Pointer to were the store the converted value
 * @param[in] max The maximum index of the enumeration value
 * @return 0 if successful, negative errno code if failure.
 */
static int sensor_value_from_enum(int value, const struct sensor_value conv[],
				  struct sensor_value *out, int max)
{
	if (value <= max) {
		out->val1 = conv[value].val1;
		out->val2 = conv[value].val2;
	} else {
		LOG_WRN("Unknown value when converting attribute");
		out->val1 = 0;
		out->val2 = 0;
	}
	return 0;
}

/**
 * Set the frequency sensor attribute
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] in The new frequency of the sensor
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_freq_to_reg(const struct device *dev, const struct sensor_value *in)
{
	__ASSERT_NO_MSG(in != NULL);
	const struct adxl34x_dev_data *data = dev->data;
	const struct adxl34x_cfg *cfg = &data->cfg;
	int rc;
	struct adxl34x_bw_rate bw_rate = cfg->bw_rate;

	bw_rate.rate = sensor_value_to_enum(in, reg_to_hz_conv, ADXL34X_ACCEL_FREQ_3200);
	rc = adxl34x_set_bw_rate(dev, &bw_rate);
	return rc;
}

/**
 * Convert the frequency sensor attribute
 *
 * @param[in] freq The current frequency of the sensor
 * @param[out] out Pointer to were the store the frequency
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_reg_to_freq(enum adxl34x_accel_freq freq, struct sensor_value *out)
{
	__ASSERT_NO_MSG(out != NULL);
	return sensor_value_from_enum(freq, reg_to_hz_conv, out, ADXL34X_ACCEL_FREQ_3200);
}

/**
 * Set the range sensor attribute
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] in The new range of the sensor
 * @return 0 if successful, negative errno code if failure.
 */
static enum adxl34x_accel_range adxl34x_range_to_reg(const struct device *dev,
						     const struct sensor_value *in)
{
	__ASSERT_NO_MSG(in != NULL);
	const struct adxl34x_dev_data *data = dev->data;
	const struct adxl34x_cfg *cfg = &data->cfg;
	int rc;
	struct sensor_value range;
	struct adxl34x_data_format data_format = cfg->data_format;
	int32_t range_ug = sensor_ms2_to_ug(in);

	sensor_value_from_u_value(range_ug, &range);
	data_format.range = sensor_value_to_enum(&range, reg_to_range_conv, ADXL34X_RANGE_16G);
	rc = adxl34x_set_data_format(dev, &data_format);
	return rc;
}

/**
 * Convert the range sensor attribute
 *
 * @param[in] range The current range of the sensor
 * @param[out] out Pointer to were the store the range
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_reg_to_range(enum adxl34x_accel_range range, struct sensor_value *out)
{
	__ASSERT_NO_MSG(out != NULL);
	return sensor_value_from_enum(range, reg_to_range_conv, out, ADXL34X_RANGE_16G);
}

/**
 * Set the offset sensor attribute
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] chan The channel the attribute belongs to, if any
 * @param[in] in The value to set the offset attribute to
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_offset_to_reg(const struct device *dev, enum sensor_channel chan,
				 const struct sensor_value *in)
{
	__ASSERT_NO_MSG(in != NULL);
	int rc = 0;

	if (chan == SENSOR_CHAN_ACCEL_X || chan == SENSOR_CHAN_ACCEL_XYZ) {
		const int32_t offset_x_ug = sensor_ms2_to_ug(&in[0]);
		const int8_t offset_x =
			(int8_t)CLAMP(offset_x_ug / offset_ug_lsb, INT8_MIN, INT8_MAX);
		rc |= adxl34x_set_ofsx(dev, offset_x);
	}
	if (chan == SENSOR_CHAN_ACCEL_Y || chan == SENSOR_CHAN_ACCEL_XYZ) {
		const int32_t offset_y_ug = sensor_ms2_to_ug(&in[1]);
		const int8_t offset_y =
			(int8_t)CLAMP(offset_y_ug / offset_ug_lsb, INT8_MIN, INT8_MAX);
		rc |= adxl34x_set_ofsy(dev, offset_y);
	}
	if (chan == SENSOR_CHAN_ACCEL_Z || chan == SENSOR_CHAN_ACCEL_XYZ) {
		const int32_t offset_z_ug = sensor_ms2_to_ug(&in[2]);
		const int8_t offset_z =
			(int8_t)CLAMP(offset_z_ug / offset_ug_lsb, INT8_MIN, INT8_MAX);
		rc |= adxl34x_set_ofsz(dev, offset_z);
	}
	return rc;
}

/**
 * Get the offset sensor attribute
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] chan The channel the attribute belongs to, if any
 * @param[out] out Pointer to where to store the offset attribute
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_reg_to_offset(const struct device *dev, enum sensor_channel chan,
				 struct sensor_value *out)
{
	__ASSERT_NO_MSG(out != NULL);
	const struct adxl34x_dev_data *data = dev->data;
	const struct adxl34x_cfg *cfg = &data->cfg;

	if (chan == SENSOR_CHAN_ACCEL_X || chan == SENSOR_CHAN_ACCEL_XYZ) {
		const int64_t offset_x_ug = cfg->ofsx * offset_ug_lsb;

		sensor_ug_to_ms2(offset_x_ug, &out[0]);
	}
	if (chan == SENSOR_CHAN_ACCEL_Y || chan == SENSOR_CHAN_ACCEL_XYZ) {
		const int64_t offset_y_ug = cfg->ofsy * offset_ug_lsb;

		sensor_ug_to_ms2(offset_y_ug, &out[1]);
	}
	if (chan == SENSOR_CHAN_ACCEL_Z || chan == SENSOR_CHAN_ACCEL_XYZ) {
		const int64_t offset_z_ug = cfg->ofsz * offset_ug_lsb;

		sensor_ug_to_ms2(offset_z_ug, &out[2]);
	}
	return 0;
}

/**
 * Callback API upon setting a sensor's attributes
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] chan The channel the attribute belongs to, if any
 * @param[in] attr The attribute to set
 * @param[in] val The value to set the attribute to
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		     const struct sensor_value *val)
{
	int rc = -ENOTSUP;
	enum pm_device_state pm_state;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(val != NULL);

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_ACCEL_X && chan != SENSOR_CHAN_ACCEL_Y &&
	    chan != SENSOR_CHAN_ACCEL_Z && chan != SENSOR_CHAN_ACCEL_XYZ) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	rc = pm_device_state_get(dev, &pm_state);
	if (rc == 0 && pm_state == PM_DEVICE_STATE_OFF) {
		return -EIO;
	}

	switch (attr) { /* NOLINT(clang-diagnostic-switch-enum) */
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		rc = adxl34x_freq_to_reg(dev, val);
		break;
	case SENSOR_ATTR_LOWER_THRESH:
	case SENSOR_ATTR_UPPER_THRESH:
	case SENSOR_ATTR_SLOPE_TH:
	case SENSOR_ATTR_SLOPE_DUR:
	case SENSOR_ATTR_HYSTERESIS:
	case SENSOR_ATTR_OVERSAMPLING:
		break;
	case SENSOR_ATTR_FULL_SCALE:
		rc = adxl34x_range_to_reg(dev, val);
		break;
	case SENSOR_ATTR_OFFSET:
		rc = adxl34x_offset_to_reg(dev, chan, val);
		break;
	case SENSOR_ATTR_CALIB_TARGET:
	case SENSOR_ATTR_CONFIGURATION:
	case SENSOR_ATTR_CALIBRATION:
	case SENSOR_ATTR_FEATURE_MASK:
	case SENSOR_ATTR_ALERT:
	case SENSOR_ATTR_FF_DUR:
	case SENSOR_ATTR_BATCH_DURATION:
	case SENSOR_ATTR_COMMON_COUNT:
	case SENSOR_ATTR_GAIN:
	case SENSOR_ATTR_RESOLUTION:
		break;
	case SENSOR_ATTR_MAX:
	default:
		LOG_ERR("Unknown attribute");
		rc = -ENOTSUP;
	}
	return rc;
}

/**
 * Callback API upon getting a sensor's attributes
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] chan The channel the attribute belongs to, if any
 * @param[in] attr The attribute to get
 * @param[out] val Pointer to where to store the attribute
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_attr_get(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		     struct sensor_value *val)
{
	const struct adxl34x_dev_data *data = dev->data;
	const struct adxl34x_cfg *cfg = &data->cfg;
	int rc = -ENOTSUP;
	enum pm_device_state pm_state;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(val != NULL);

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_ACCEL_X && chan != SENSOR_CHAN_ACCEL_Y &&
	    chan != SENSOR_CHAN_ACCEL_Z && chan != SENSOR_CHAN_ACCEL_XYZ) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	rc = pm_device_state_get(dev, &pm_state);
	if (rc == 0 && pm_state == PM_DEVICE_STATE_OFF) {
		return -EIO;
	}

	switch (attr) { /* NOLINT(clang-diagnostic-switch-enum) */
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		rc = adxl34x_reg_to_freq(cfg->bw_rate.rate, val);
		break;
	case SENSOR_ATTR_LOWER_THRESH:
	case SENSOR_ATTR_UPPER_THRESH:
	case SENSOR_ATTR_SLOPE_TH:
	case SENSOR_ATTR_SLOPE_DUR:
	case SENSOR_ATTR_HYSTERESIS:
	case SENSOR_ATTR_OVERSAMPLING:
		break;
	case SENSOR_ATTR_FULL_SCALE:
		rc = adxl34x_reg_to_range(cfg->data_format.range, val);
		break;
	case SENSOR_ATTR_OFFSET:
		rc = adxl34x_reg_to_offset(dev, chan, val);
		break;
	case SENSOR_ATTR_CALIB_TARGET:
	case SENSOR_ATTR_CONFIGURATION:
	case SENSOR_ATTR_CALIBRATION:
	case SENSOR_ATTR_FEATURE_MASK:
	case SENSOR_ATTR_ALERT:
	case SENSOR_ATTR_FF_DUR:
	case SENSOR_ATTR_BATCH_DURATION:
	case SENSOR_ATTR_COMMON_COUNT:
	case SENSOR_ATTR_GAIN:
	case SENSOR_ATTR_RESOLUTION:
		break;
	case SENSOR_ATTR_MAX:
	default:
		LOG_ERR("Unknown attribute");
	}

	return rc;
}
