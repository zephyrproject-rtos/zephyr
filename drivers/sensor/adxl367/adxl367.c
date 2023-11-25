/*
 * Copyright (c) 2023 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl367

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>

#include "adxl367.h"

LOG_MODULE_REGISTER(ADXL367, CONFIG_SENSOR_LOG_LEVEL);

static const uint8_t adxl367_scale_mul[3] = {1, 2, 4};
static uint8_t samples_per_set;

/**
 * @brief Configures activity detection.
 *
 * @param dev         - The device structure.
 * @param th          - Structure holding the activity threshold information:
 *				Enable/Disable Activity detection
 *				Enable/Disable Referenced Activity detection
 *				Set Threshold value
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl367_setup_activity_detection(const struct device *dev,
					    const struct adxl367_activity_threshold *th)
{
	struct adxl367_data *data = dev->data;
	int ret;


	ret = data->hw_tf->write_reg_mask(dev, ADXL367_ACT_INACT_CTL,
					  ADXL367_ACT_INACT_CTL_ACT_EN_MSK |
					  ADXL367_ACT_INACT_CTL_ACT_REF_MSK,
					  FIELD_PREP(ADXL367_ACT_INACT_CTL_ACT_EN_MSK, th->enable) |
					  FIELD_PREP(ADXL367_ACT_INACT_CTL_ACT_REF_MSK,
						     th->referenced));
	if (ret != 0) {
		return ret;
	}

	ret = data->hw_tf->write_reg_mask(dev, ADXL367_THRESH_ACT_H, ADXL367_THRESH_H_MSK,
					  FIELD_PREP(ADXL367_THRESH_H_MSK, th->value >> 6));
	if (ret != 0) {
		return ret;
	}

	return data->hw_tf->write_reg_mask(dev, ADXL367_THRESH_ACT_L, ADXL367_THRESH_L_MSK,
					   FIELD_PREP(ADXL367_THRESH_L_MSK, th->value & 0x3F));
}

/**
 * @brief Configures activity detection.
 *
 * @param dev         - The device structure.
 * @param th          - Structure holding the inactivity threshold information:
 *				Enable/Disable inactivity detection
 *				Enable/Disable Referenced inactivity detection
 *				Set Threshold value
 *
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl367_setup_inactivity_detection(const struct device *dev,
					      const struct adxl367_activity_threshold *th)
{
	struct adxl367_data *data = dev->data;
	int ret;

	ret = data->hw_tf->write_reg_mask(dev, ADXL367_ACT_INACT_CTL,
					  ADXL367_ACT_INACT_CTL_INACT_EN_MSK |
					  ADXL367_ACT_INACT_CTL_INACT_REF_MSK,
					  FIELD_PREP(ADXL367_ACT_INACT_CTL_INACT_EN_MSK,
						     th->enable) |
					  FIELD_PREP(ADXL367_ACT_INACT_CTL_INACT_REF_MSK,
						     th->referenced));
	if (ret != 0) {
		return ret;
	}

	ret = data->hw_tf->write_reg_mask(dev, ADXL367_THRESH_INACT_H, ADXL367_THRESH_H_MSK,
					  FIELD_PREP(ADXL367_THRESH_H_MSK, th->value >> 6));
	if (ret != 0) {
		return ret;
	}

	return data->hw_tf->write_reg_mask(dev, ADXL367_THRESH_INACT_L, ADXL367_THRESH_L_MSK,
					   FIELD_PREP(ADXL367_THRESH_L_MSK, th->value & 0x3F));
}

/**
 * @brief Set the mode of operation.
 *
 * @param dev - The device structure.
 * @param op_mode - Mode of operation.
 *		Accepted values: ADXL367_STANDBY
 *				ADXL367_MEASURE
 *
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl367_set_op_mode(const struct device *dev,
			       enum adxl367_op_mode op_mode)
{
	struct adxl367_data *data = dev->data;
	int ret;

	ret = data->hw_tf->write_reg_mask(dev, ADXL367_POWER_CTL,
					  ADXL367_POWER_CTL_MEASURE_MSK,
					  FIELD_PREP(ADXL367_POWER_CTL_MEASURE_MSK, op_mode));
	if (ret != 0) {
		return ret;
	}

	if (op_mode == ADXL367_MEASURE)
		/* Wait 100 ms to allow the acceleration outputs to settle */
		k_sleep(K_MSEC(100));

	return 0;
}

/**
 * @brief Autosleep. When set to 1, autosleep is enabled, and the device enters
 * wake-up mode automatically upon detection of inactivity.
 *
 * @param dev - The device structure.
 * @param enable - Accepted values: true
 *				    false
 *
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl367_set_autosleep(const struct device *dev, bool enable)
{
	struct adxl367_data *data = dev->data;

	return data->hw_tf->write_reg_mask(dev, ADXL367_POWER_CTL,
					   ADXL367_POWER_CTL_AUTOSLEEP_MSK,
					   FIELD_PREP(ADXL367_POWER_CTL_AUTOSLEEP_MSK, enable));
}

/**
 * @brief Noise Mode. When set to 1, low noise mode is enabled
 *
 * @param dev - The device structure.
 * @param enable - Accepted values: true
 *				    false
 *
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl367_set_low_noise(const struct device *dev, bool enable)
{
	struct adxl367_data *data = dev->data;

	return data->hw_tf->write_reg_mask(dev, ADXL367_POWER_CTL,
					   ADXL367_POWER_CTL_NOISE_MSK,
					   FIELD_PREP(ADXL367_POWER_CTL_NOISE_MSK, enable));
}

/**
 * @brief Link/Loop Activity Processing.
 *
 * @param dev - The device structure.
 * @param mode - Mode of operation.
 *		Accepted values: ADXL367_DEFAULT
 *				 ADXL367_LINKED
 *				 ADXL367_LOOPED
 *
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl367_set_act_proc_mode(const struct device *dev,
				     enum adxl367_act_proc_mode mode)
{
	struct adxl367_data *data = dev->data;

	return data->hw_tf->write_reg_mask(dev, ADXL367_ACT_INACT_CTL,
					   ADXL367_ACT_INACT_CTL_LINKLOOP_MSK,
					   FIELD_PREP(ADXL367_ACT_INACT_CTL_LINKLOOP_MSK, mode));
}


/**
 * @brief Selects the Output Data Rate of the device.
 * @param dev - The device structure.
 * @param odr - Output Data Rate option.
 *              Accepted values: ADXL367_ODR_12P5HZ,
 *                               ADXL367_ODR_25HZ,
 *                               ADXL367_ODR_50HZ,
 *                               ADXL367_ODR_100HZ,
 *                               ADXL367_ODR_200HZ,
 *                               ADXL367_ODR_400HZ
 * @return 0 in case of success, negative error code otherwise.
 */
int adxl367_set_output_rate(const struct device *dev, enum adxl367_odr odr)
{
	struct adxl367_data *data = dev->data;

	return data->hw_tf->write_reg_mask(dev,
					  ADXL367_FILTER_CTL,
					  ADXL367_FILTER_CTL_ODR_MSK,
					  FIELD_PREP(ADXL367_FILTER_CTL_ODR_MSK, odr));
}

/**
 * @brief Selects the measurement range.
 *
 * @param dev - The device structure.
 * @param range - Range option.
 *		 Accepted values: ADXL367_2G_RANGE, +/- 2g
 *				ADXL367_4G_RANGE, +/- 4g
 *				ADXL367_8G_RANGE  +/- 8g
 *
 * @return 0 in case of success, negative error code otherwise.
 */
int adxl367_set_range(const struct device *dev, enum adxl367_range range)
{
	struct adxl367_data *data = dev->data;

	return data->hw_tf->write_reg_mask(dev,
					  ADXL367_FILTER_CTL,
					  ADXL367_FILTER_CTL_RANGE_MSK,
					  FIELD_PREP(ADXL367_FILTER_CTL_RANGE_MSK, range));
}

/**
 * @brief Set the activity timer
 *
 * @param dev - The device structure.
 * @param time - The value set in this register.
 *
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl367_set_activity_time(const struct device *dev, uint8_t time)
{
	struct adxl367_data *data = dev->data;

	return data->hw_tf->write_reg(dev, ADXL367_TIME_ACT, time);
}

/**
 * Set the inactivity timer
 * @param dev - The device structure.
 * @param time - is the 16-bit value set by the TIME_INACT_L register
 *		 (eight LSBs) and the TIME_INACT_H register (eight MSBs).
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl367_set_inactivity_time(const struct device *dev,
				       uint16_t time)
{
	int ret;
	struct adxl367_data *data = dev->data;

	ret = data->hw_tf->write_reg(dev, ADXL367_TIME_INACT_H, time >> 8);
	if (ret != 0) {
		return ret;
	}

	return data->hw_tf->write_reg(dev, ADXL367_TIME_INACT_L, time & 0xFF);
}

/**
 * @brief Performs self test.
 *
 * @param dev - The device structure.
 *
 * @return 0 in case of success, negative error code otherwise.
 */
int adxl367_self_test(const struct device *dev)
{
	int ret;
	struct adxl367_data *data = dev->data;
	const struct adxl367_dev_config *cfg = dev->config;
	int16_t x_axis_1, x_axis_2, dif, min, max;
	uint8_t read_val[2];

	uint32_t st_delay_ms;

	/* 4 / ODR value in ms */
	switch (cfg->odr) {
	case ADXL367_ODR_12P5HZ:
		st_delay_ms = 320;
		break;
	case ADXL367_ODR_25HZ:
		st_delay_ms = 160;
		break;
	case ADXL367_ODR_50HZ:
		st_delay_ms = 80;
		break;
	case ADXL367_ODR_100HZ:
		st_delay_ms = 40;
		break;
	case ADXL367_ODR_200HZ:
		st_delay_ms = 20;
		break;
	case ADXL367_ODR_400HZ:
		st_delay_ms = 10;
		break;
	default:
		return -EINVAL;
	}

	ret = adxl367_set_op_mode(dev, ADXL367_MEASURE);
	if (ret != 0) {
		return ret;
	}

	ret = data->hw_tf->write_reg_mask(dev, ADXL367_SELF_TEST, ADXL367_SELF_TEST_ST_MSK,
				    FIELD_PREP(ADXL367_SELF_TEST_ST_MSK, 1));
	if (ret != 0) {
		return ret;
	}

	/* 4 / ODR */
	k_sleep(K_MSEC(st_delay_ms));

	ret = data->hw_tf->read_reg_multiple(dev, ADXL367_X_DATA_H, read_val, 2);
	if (ret != 0) {
		return ret;
	}

	x_axis_1 = ((int16_t)read_val[0] << 6) + (read_val[1] >> 2);

	/* extend sign to 16 bits */
	if ((x_axis_1 & BIT(13)) != 0) {
		x_axis_1 |= GENMASK(15, 14);
	}

	ret = data->hw_tf->write_reg_mask(dev, ADXL367_SELF_TEST,
					  ADXL367_SELF_TEST_ST_FORCE_MSK,
					  FIELD_PREP(ADXL367_SELF_TEST_ST_FORCE_MSK, 1));
	if (ret != 0) {
		return ret;
	}

	/* 4 / ODR */
	k_sleep(K_MSEC(st_delay_ms));

	ret = data->hw_tf->read_reg_multiple(dev, ADXL367_X_DATA_H, read_val, 2);
	if (ret != 0) {
		return ret;
	}

	x_axis_2 = ((int16_t)read_val[0] << 6) + (read_val[1] >> 2);

	/* extend sign to 16 bits */
	if ((x_axis_2 & BIT(13)) != 0)
		x_axis_2 |= GENMASK(15, 14);

	ret = adxl367_set_op_mode(dev, ADXL367_STANDBY);
	if (ret != 0) {
		return ret;
	}

	ret = data->hw_tf->write_reg_mask(dev, ADXL367_SELF_TEST, ADXL367_SELF_TEST_ST_FORCE_MSK |
					  ADXL367_SELF_TEST_ST_MSK,
					  FIELD_PREP(ADXL367_SELF_TEST_ST_FORCE_MSK, 0) |
					  FIELD_PREP(ADXL367_SELF_TEST_ST_MSK, 0));
	if (ret != 0) {
		return ret;
	}

	dif = x_axis_2 - x_axis_1;
	min = ADXL367_SELF_TEST_MIN * adxl367_scale_mul[data->range];
	max = ADXL367_SELF_TEST_MAX * adxl367_scale_mul[data->range];

	if ((dif >= min) && (dif <= max)) {
		LOG_INF("ADXL367 passed self-test\n");
		ret = 0;
	} else {
		LOG_ERR("ADXL367 failed self-test\n");
		ret = -EINVAL;
	}

	return ret;
}

/**
 * @brief Enables temperature reading.
 *
 * @param dev    - The device structure.
 * @param enable - 1 - ENABLE
 *		   2 - DISABLE
 *
 * @return 0 in case of success, negative error code otherwise.
 */
int adxl367_temp_read_en(const struct device *dev, bool enable)
{
	struct adxl367_data *data = dev->data;

	return data->hw_tf->write_reg_mask(dev,
					   ADXL367_TEMP_CTL,
					   ADXL367_TEMP_EN_MSK,
					   FIELD_PREP(ADXL367_TEMP_EN_MSK, enable));
}

/**
 * @brief Sets the number of FIFO sample sets.
 *
 * @param dev     - The device structure.
 * @param sets_nb - Sample sets number. For example, if ADXL367_FIFO_FORMAT_XYZ
 *		    is selected, a value of 2 will represent 6 entries.
 *
 * @return 0 in case of success, negative error code otherwise.
 */
int adxl367_set_fifo_sample_sets_nb(const struct device *dev,
				    uint16_t sets_nb)
{
	struct adxl367_data *data = dev->data;
	int ret;
	uint8_t fifo_samples_msb = sets_nb & BIT(9) ? 1U : 0U;

	/* bit 9 goes to FIFO_SAMPLES from ADXL367_FIFO_CONTROL */
	ret = data->hw_tf->write_reg_mask(dev, ADXL367_FIFO_CONTROL,
					  ADXL367_FIFO_CONTROL_FIFO_SAMPLES_MSK,
					  FIELD_PREP(ADXL367_FIFO_CONTROL_FIFO_SAMPLES_MSK,
					  fifo_samples_msb));
	if (ret != 0) {
		return ret;
	}

	/* write last 8 bits to ADXL367_FIFO_SAMPLES */
	return data->hw_tf->write_reg(dev, ADXL367_FIFO_SAMPLES, sets_nb & 0xFF);
}

/**
 * @brief Sets FIFO mode.
 *
 * @param dev  - The device structure.
 * @param mode - FIFO mode.
 *               Accepted values: ADXL367_FIFO_DISABLED,
 *                                ADXL367_OLDEST_SAVED,
 *                                ADXL367_STREAM_MODE,
 *                                ADXL367_TRIGGERED_MODE
 *
 * @return 0 in case of success, negative error code otherwise.
 */
int adxl367_set_fifo_mode(const struct device *dev,
			  enum adxl367_fifo_mode mode)
{
	struct adxl367_data *data = dev->data;

	return data->hw_tf->write_reg_mask(dev,
					   ADXL367_FIFO_CONTROL,
					   ADXL367_FIFO_CONTROL_FIFO_MODE_MSK,
					   FIELD_PREP(ADXL367_FIFO_CONTROL_FIFO_MODE_MSK, mode));
}

/**
 * @brief Sets FIFO read mode.
 *
 * @param dev	    - The device structure.
 * @param read_mode - FIFO read mode.
 *                    Accepted values: ADXL367_12B_CHID,
 *		                       ADXL367_8B,
 *                                     ADXL367_12B,
 *                                     ADXL367_14B_CHID
 *
 * @return 0 in case of success, negative error code otherwise.
 */
int adxl367_set_fifo_read_mode(const struct device *dev,
			       enum adxl367_fifo_read_mode read_mode)
{
	struct adxl367_data *data = dev->data;

	return data->hw_tf->write_reg_mask(dev, ADXL367_ADC_CTL,
				    ADXL367_FIFO_8_12BIT_MSK,
				    FIELD_PREP(ADXL367_FIFO_8_12BIT_MSK, read_mode));
}

/**
 * @brief Sets FIFO format.
 *
 * @param dev     - The device structure.
 * @param format  - FIFO format.
 *			Accepted values: ADXL367_FIFO_FORMAT_XYZ,
 *					ADXL367_FIFO_FORMAT_X,
 *					ADXL367_FIFO_FORMAT_Y,
 *					ADXL367_FIFO_FORMAT_Z,
 *					ADXL367_FIFO_FORMAT_XYZT,
 *					ADXL367_FIFO_FORMAT_XT,
 *					ADXL367_FIFO_FORMAT_YT,
 *					ADXL367_FIFO_FORMAT_ZT,
 *					ADXL367_FIFO_FORMAT_XYZA,
 *					ADXL367_FIFO_FORMAT_XA,
 *					ADXL367_FIFO_FORMAT_YA,
 *					ADXL367_FIFO_FORMAT_ZA
 *
 * @return 0 in case of success, negative error code otherwise.
 */
int adxl367_set_fifo_format(const struct device *dev,
			    enum adxl367_fifo_format format)
{
	int ret;
	struct adxl367_data *data = dev->data;

	ret = data->hw_tf->write_reg_mask(dev,
				    ADXL367_FIFO_CONTROL,
				    ADXL367_FIFO_CONTROL_FIFO_CHANNEL_MSK,
				    FIELD_PREP(ADXL367_FIFO_CONTROL_FIFO_CHANNEL_MSK, format));
	if (ret != 0) {
		return ret;
	}

	switch (format) {
	case ADXL367_FIFO_FORMAT_XYZ:
		samples_per_set = 3;
		break;
	case ADXL367_FIFO_FORMAT_X:
	case ADXL367_FIFO_FORMAT_Y:
	case ADXL367_FIFO_FORMAT_Z:
		samples_per_set = 1;
		break;
	case ADXL367_FIFO_FORMAT_XYZT:
	case ADXL367_FIFO_FORMAT_XYZA:
		samples_per_set = 4;
		break;
	case ADXL367_FIFO_FORMAT_XT:
	case ADXL367_FIFO_FORMAT_YT:
	case ADXL367_FIFO_FORMAT_ZT:
	case ADXL367_FIFO_FORMAT_XA:
	case ADXL367_FIFO_FORMAT_YA:
	case ADXL367_FIFO_FORMAT_ZA:
		samples_per_set = 2;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Configures the FIFO feature. Uses ADXL367_14B_CHID read mode as
 *	  default.
 *
 * @param dev - The device structure.
 * @param mode - FIFO mode selection.
 *               Example: ADXL367_FIFO_DISABLED,
 *                        ADXL367_OLDEST_SAVED,
 *                        ADXL367_STREAM_MODE,
 *                        ADXL367_TRIGGERED_MODE
 * @param format - FIFO format selection.
 *                 Example:  ADXL367_FIFO_FORMAT_XYZ,
 *                           ADXL367_FIFO_FORMAT_X,
 *                           ADXL367_FIFO_FORMAT_Y,
 *                           ADXL367_FIFO_FORMAT_Z,
 *                           ADXL367_FIFO_FORMAT_XYZT,
 *                           ADXL367_FIFO_FORMAT_XT,
 *                           ADXL367_FIFO_FORMAT_YT,
 *                           ADXL367_FIFO_FORMAT_ZT,
 *                           ADXL367_FIFO_FORMAT_XYZA,
 *                           ADXL367_FIFO_FORMAT_XA,
 *                           ADXL367_FIFO_FORMAT_YA,
 *                           ADXL367_FIFO_FORMAT_ZA
 * @param read_mode - FIFO read mode.
 *                    Accepted values: ADXL367_12B_CHID,
 *		                       ADXL367_8B,
 *                                     ADXL367_12B,
 *                                     ADXL367_14B_CHID
 * @param sets_nb - Specifies the number of samples sets to store in the FIFO.
 *
 * @return 0 in case of success, negative error code otherwise.
 */
int adxl367_fifo_setup(const struct device *dev,
		       enum adxl367_fifo_mode mode,
		       enum adxl367_fifo_format format,
		       enum adxl367_fifo_read_mode read_mode,
		       uint8_t sets_nb)
{
	int ret;

	ret = adxl367_set_fifo_mode(dev, mode);
	if (ret != 0) {
		return ret;
	}

	ret = adxl367_set_fifo_format(dev, format);
	if (ret != 0) {
		return ret;
	}

	ret = adxl367_set_fifo_sample_sets_nb(dev, sets_nb);
	if (ret != 0) {
		return ret;
	}

	return adxl367_set_fifo_read_mode(dev, read_mode);
}

/**
 * @brief Software reset.
 *
 * @param dev - The device structure.
 *
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl367_reset(const struct device *dev)
{
	int ret;
	struct adxl367_data *data = dev->data;

	ret = adxl367_set_op_mode(dev, ADXL367_STANDBY);
	if (ret != 0) {
		return ret;
	}

	/* Writing code 0x52 resets the device */
	ret = data->hw_tf->write_reg(dev, ADXL367_SOFT_RESET, ADXL367_RESET_CODE);
	if (ret != 0) {
		return ret;
	}

	/* Delay required after performing software reset */
	k_sleep(K_MSEC(8));

	return ret;
}


/**
 * @brief Reads the 3-axis raw data from the accelerometer.
 *
 * @param dev - The device structure.
 * @param accel_data - store the XYZ axis accelerometer data.
 *
 * @return 0 in case of success, negative error code otherwise.
 */
int adxl367_get_accel_data(const struct device *dev,
			   struct adxl367_xyz_accel_data *accel_data)
{
	int ret;
	uint8_t xyz_values[6] = { 0 };
	uint8_t reg_data, nready = 1U;
	struct adxl367_data *data = dev->data;

	while (nready != 0) {
		ret = data->hw_tf->read_reg(dev, ADXL367_STATUS, &reg_data);
		if (ret != 0) {
			return ret;
		}

		if ((reg_data & ADXL367_STATUS_DATA_RDY) != 0) {
			nready = 0U;
		}
	}

	ret = data->hw_tf->read_reg_multiple(dev, ADXL367_X_DATA_H, xyz_values, 6);
	if (ret != 0) {
		return ret;
	}

	/* result is 14 bits long, ignore last 2 bits from low byte */
	accel_data->x = ((int16_t)xyz_values[0] << 6) + (xyz_values[1] >> 2);
	accel_data->y = ((int16_t)xyz_values[2] << 6) + (xyz_values[3] >> 2);
	accel_data->z = ((int16_t)xyz_values[4] << 6) + (xyz_values[5] >> 2);

	/* extend sign to 16 bits */
	if ((accel_data->x & BIT(13)) != 0) {
		accel_data->x |= GENMASK(15, 14);
	}

	if ((accel_data->y & BIT(13)) != 0) {
		accel_data->y |= GENMASK(15, 14);
	}

	if ((accel_data->z & BIT(13)) != 0) {
		accel_data->z |= GENMASK(15, 14);
	}

	return 0;
}

/**
 * @brief Reads the raw temperature of the device. If ADXL367_TEMP_EN is not
 *        set, use adxl367_temp_read_en() first to enable temperature reading.
 *
 * @param dev      - The device structure.
 * @param raw_temp - Raw value of temperature.
 *
 * @return 0 in case of success, negative error code otherwise.
 */
int adxl367_get_temp_data(const struct device *dev, int16_t *raw_temp)
{
	int ret;
	uint8_t temp[2] = { 0 };
	uint8_t reg_data, nready = 1U;
	struct adxl367_data *data = dev->data;

	while (nready != 0) {
		ret = data->hw_tf->read_reg(dev, ADXL367_STATUS, &reg_data);
		if (ret != 0) {
			return ret;
		}

		if ((reg_data & ADXL367_STATUS_DATA_RDY) != 0) {
			nready = 0U;
		}
	}

	ret = data->hw_tf->read_reg_multiple(dev, ADXL367_TEMP_H, temp, 2);
	if (ret != 0) {
		return ret;
	}

	*raw_temp = ((int16_t)temp[0] << 6) + (temp[1] >> 2);
	/* extend sign to 16 bits */
	if ((*raw_temp & BIT(13)) != 0) {
		*raw_temp |= GENMASK(15, 14);
	}

	return 0;
}

static int adxl367_attr_set_thresh(const struct device *dev,
				   enum sensor_channel chan,
				   enum sensor_attribute attr,
				   const struct sensor_value *val)
{
	const struct adxl367_dev_config *cfg = dev->config;
	struct adxl367_activity_threshold threshold;
	int64_t llvalue;
	int32_t value;
	int64_t micro_ms2 = val->val1 * 1000000LL + val->val2;

	llvalue = llabs((micro_ms2 * 10) / SENSOR_G);

	value = (int32_t) llvalue;

	threshold.value = value;
	threshold.enable = cfg->activity_th.enable;
	threshold.referenced = cfg->activity_th.referenced;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_UPPER_THRESH) {
			return adxl367_setup_activity_detection(dev, &threshold);
		} else {
			return adxl367_setup_inactivity_detection(dev, &threshold);
		}

	default:
		LOG_ERR("attr_set() not supported on this channel");
		return -ENOTSUP;
	}
}

static int adxl367_attr_set_odr(const struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	enum adxl367_odr odr;

	switch (val->val1) {
	case 12:
	case 13:
		odr = ADXL367_ODR_12P5HZ;
		break;
	case 25:
		odr = ADXL367_ODR_25HZ;
		break;
	case 50:
		odr = ADXL367_ODR_50HZ;
		break;
	case 100:
		odr = ADXL367_ODR_100HZ;
		break;
	case 200:
		odr = ADXL367_ODR_200HZ;
		break;
	case 400:
		odr = ADXL367_ODR_400HZ;
		break;
	default:
		return -EINVAL;
	}

	return adxl367_set_output_rate(dev, odr);
}

static int adxl367_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return adxl367_attr_set_odr(dev, chan, attr, val);
	case SENSOR_ATTR_UPPER_THRESH:
	case SENSOR_ATTR_LOWER_THRESH:
		return adxl367_attr_set_thresh(dev, chan, attr, val);
	default:
		return -ENOTSUP;
	}
}

static int adxl367_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct adxl367_data *data = dev->data;
	int ret;

	ret = adxl367_get_accel_data(dev, &data->sample);
	if (ret != 0) {
		return ret;
	}

	return adxl367_get_temp_data(dev, &data->temp_val);
}

static void adxl367_accel_convert(const struct device *dev,
				  struct sensor_value *val, int16_t value)
{
	struct adxl367_data *data = dev->data;

	int64_t micro_ms2 = value * (SENSOR_G * 250 / 10000 *
			  adxl367_scale_mul[data->range] / 1000);

	val->val1 = micro_ms2 / 1000000;
	val->val2 = micro_ms2 % 1000000;
}

static void adxl367_temp_convert(struct sensor_value *val, int16_t value)
{
	int64_t temp_data = (value + ADXL367_TEMP_OFFSET) * ADXL367_TEMP_SCALE;

	val->val1 = temp_data / ADXL367_TEMP_SCALE_DIV;
	val->val2 = temp_data % ADXL367_TEMP_SCALE_DIV;
}

static int adxl367_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct adxl367_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		adxl367_accel_convert(dev, val, data->sample.x);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		adxl367_accel_convert(dev, val, data->sample.y);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		adxl367_accel_convert(dev, val, data->sample.z);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		adxl367_accel_convert(dev, val++, data->sample.x);
		adxl367_accel_convert(dev, val++, data->sample.y);
		adxl367_accel_convert(dev, val, data->sample.z);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		adxl367_temp_convert(val, data->temp_val);
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api adxl367_api_funcs = {
	.attr_set     = adxl367_attr_set,
	.sample_fetch = adxl367_sample_fetch,
	.channel_get  = adxl367_channel_get,
#ifdef CONFIG_ADXL367_TRIGGER
	.trigger_set = adxl367_trigger_set,
#endif
};

static int adxl367_probe(const struct device *dev)
{
	const struct adxl367_dev_config *cfg = dev->config;
	struct adxl367_data *data = dev->data;
	uint8_t dev_id, part_id;
	int ret;

	ret = adxl367_reset(dev);
	if (ret != 0) {
		return ret;
	}

	ret = data->hw_tf->read_reg(dev, ADXL367_DEVID, &dev_id);
	if (ret != 0) {
		return ret;
	}
	ret = data->hw_tf->read_reg(dev, ADXL367_PART_ID, &part_id);
	if (ret != 0) {
		return ret;
	}

	if (dev_id != ADXL367_DEVID_VAL || part_id != ADXL367_PARTID_VAL) {
		LOG_ERR("failed to read id (0x%X:0x%X)\n", dev_id, part_id);
		return -ENODEV;
	}

	data->range = cfg->range;

#ifdef CONFIG_ADXL367_TRIGGER
	data->act_proc_mode = ADXL367_LINKED;
#else
	data->act_proc_mode = ADXL367_LOOPED;
#endif

	ret = adxl367_self_test(dev);
	if (ret != 0) {
		return ret;
	}

	ret = adxl367_temp_read_en(dev, cfg->temp_en);
	if (ret != 0) {
		return ret;
	}

	ret = adxl367_set_autosleep(dev, cfg->autosleep);
	if (ret != 0) {
		return ret;
	}

	ret = adxl367_set_low_noise(dev, cfg->low_noise);
	if (ret != 0) {
		return ret;
	}

	ret = adxl367_setup_activity_detection(dev, &cfg->activity_th);
	if (ret != 0) {
		return ret;
	}

	ret = adxl367_setup_inactivity_detection(dev, &cfg->inactivity_th);
	if (ret != 0) {
		return ret;
	}

	ret = adxl367_set_activity_time(dev, cfg->activity_time);
	if (ret != 0) {
		return ret;
	}

	ret = adxl367_set_inactivity_time(dev, cfg->inactivity_time);
	if (ret != 0) {
		return ret;
	}

	ret = adxl367_set_output_rate(dev, cfg->odr);
	if (ret != 0) {
		return ret;
	}

	ret = adxl367_fifo_setup(dev, cfg->fifo_config.fifo_mode,
				 cfg->fifo_config.fifo_format,
				 cfg->fifo_config.fifo_read_mode,
				 cfg->fifo_config.fifo_samples);
	if (ret != 0) {
		return ret;
	}

if (IS_ENABLED(CONFIG_ADXL367_TRIGGER)) {
	ret = adxl367_init_interrupt(dev);
	if (ret != 0) {
		LOG_ERR("Failed to initialize interrupt!");
		return -EIO;
	}
}

	ret = adxl367_set_op_mode(dev, cfg->op_mode);
	if (ret != 0) {
		return ret;
	}

	ret = adxl367_set_range(dev, data->range);
	if (ret != 0) {
		return ret;
	}

	return adxl367_set_act_proc_mode(dev, data->act_proc_mode);
}

static int adxl367_init(const struct device *dev)
{
	int ret;
	const struct adxl367_dev_config *cfg = dev->config;

	ret = cfg->bus_init(dev);
	if (ret != 0) {
		LOG_ERR("Failed to initialize sensor bus\n");
		return ret;
	}

	return adxl367_probe(dev);
}

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "ADXL367 driver enabled without any devices"
#endif

/*
 * Device creation macro, shared by ADXL367_DEFINE_SPI() and
 * ADXL367_DEFINE_I2C().
 */

#define ADXL367_DEVICE_INIT(inst)					\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,				\
			      adxl367_init,				\
			      NULL,					\
			      &adxl367_data_##inst,			\
			      &adxl367_config_##inst,			\
			      POST_KERNEL,				\
			      CONFIG_SENSOR_INIT_PRIORITY,		\
			      &adxl367_api_funcs);

#ifdef CONFIG_ADXL367_TRIGGER
#define ADXL367_CFG_IRQ(inst) \
		.interrupt = GPIO_DT_SPEC_INST_GET(inst, int1_gpios),
#else
#define ADXL367_CFG_IRQ(inst)
#endif /* CONFIG_ADXL367_TRIGGER */

#define ADXL367_CONFIG(inst)								\
		.odr = DT_INST_PROP(inst, odr),						\
		.autosleep = false,							\
		.low_noise = false,							\
		.temp_en = true,							\
		.range = ADXL367_2G_RANGE,						\
		.activity_th.value = CONFIG_ADXL367_ACTIVITY_THRESHOLD,			\
		.activity_th.referenced =						\
			IS_ENABLED(CONFIG_ADXL367_REFERENCED_ACTIVITY_DETECTION_MODE),	\
		.activity_th.enable =							\
			IS_ENABLED(CONFIG_ADXL367_ACTIVITY_DETECTION_MODE),		\
		.activity_time = CONFIG_ADXL367_ACTIVITY_TIME,				\
		.inactivity_th.value = CONFIG_ADXL367_INACTIVITY_THRESHOLD,		\
		.inactivity_th.referenced =						\
			IS_ENABLED(CONFIG_ADXL367_REFERENCED_INACTIVITY_DETECTION_MODE),\
		.inactivity_th.enable =							\
			IS_ENABLED(CONFIG_ADXL367_INACTIVITY_DETECTION_MODE),		\
		.inactivity_time = CONFIG_ADXL367_INACTIVITY_TIME,			\
		.fifo_config.fifo_mode = ADXL367_FIFO_DISABLED,				\
		.fifo_config.fifo_format = ADXL367_FIFO_FORMAT_XYZ,			\
		.fifo_config.fifo_samples = 128,					\
		.fifo_config.fifo_read_mode = ADXL367_14B_CHID,				\
		.op_mode = ADXL367_MEASURE,

/*
 * Instantiation macros used when a device is on a SPI bus.
 */

#define ADXL367_CONFIG_SPI(inst)					\
	{								\
		.bus_init = adxl367_spi_init,				\
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8) |	\
					SPI_TRANSFER_MSB, 0),		\
		ADXL367_CONFIG(inst)					\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int1_gpios),	\
		(ADXL367_CFG_IRQ(inst)), ())				\
	}

#define ADXL367_DEFINE_SPI(inst)					\
	static struct adxl367_data adxl367_data_##inst;			\
	static const struct adxl367_dev_config adxl367_config_##inst =	\
		ADXL367_CONFIG_SPI(inst);				\
	ADXL367_DEVICE_INIT(inst)

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define ADXL367_CONFIG_I2C(inst)					\
	{								\
		.bus_init = adxl367_i2c_init,				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),			\
		ADXL367_CONFIG(inst)					\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int1_gpios),	\
		(ADXL367_CFG_IRQ(inst)), ())				\
	}

#define ADXL367_DEFINE_I2C(inst)					\
	static struct adxl367_data adxl367_data_##inst;			\
	static const struct adxl367_dev_config adxl367_config_##inst =	\
		ADXL367_CONFIG_I2C(inst);				\
	ADXL367_DEVICE_INIT(inst)
/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define ADXL367_DEFINE(inst)						\
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),				\
		    (ADXL367_DEFINE_SPI(inst)),				\
		    (ADXL367_DEFINE_I2C(inst)))

DT_INST_FOREACH_STATUS_OKAY(ADXL367_DEFINE)
