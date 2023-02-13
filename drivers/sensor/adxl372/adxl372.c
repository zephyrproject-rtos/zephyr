/*
 * Copyright (c) 2018 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl372

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>

#include "adxl372.h"

LOG_MODULE_REGISTER(ADXL372, CONFIG_SENSOR_LOG_LEVEL);

/**
 * Set the threshold for activity detection for a single axis
 * @param dev - The device structure.
 * @param axis_reg_h - The high part of the activity register.
 * @param act - The activity config structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl372_set_activity_threshold(const struct device *dev,
					  uint8_t axis_reg_h,
					  const struct adxl372_activity_threshold *act)
{
	int ret;
	uint8_t val;
	struct adxl372_data *data = dev->data;

	ret = data->hw_tf->write_reg(dev, axis_reg_h++, act->thresh >> 3);
	if (ret) {
		return ret;
	}

	switch (axis_reg_h) {
	case ADXL372_X_THRESH_ACT_L:
	case ADXL372_X_THRESH_INACT_L:
	case ADXL372_X_THRESH_ACT2_L:
		val = (act->thresh << 5) | (act->referenced << 1) | act->enable;
		break;
	default:
		val = (act->thresh << 5) | act->enable;
	}

	return data->hw_tf->write_reg(dev, axis_reg_h, val);
}

/**
 * Set the threshold for activity detection for all 3-axis
 * @param dev - The device structure.
 * @param axis_reg_h - The high part of the activity register.
 * @param act - The activity config structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl372_set_activity_threshold_xyz(const struct device *dev,
					      uint8_t axis_reg_h,
					      const struct adxl372_activity_threshold *act)
{
	int i, ret;

	for (i = 0; i < 3; i++) {
		ret = adxl372_set_activity_threshold(dev, axis_reg_h, act);
		if (ret) {
			return ret;
		}
		axis_reg_h += 2U;
	}

	return 0;
}

/**
 * Set the mode of operation.
 * @param dev - The device structure.
 * @param op_mode - Mode of operation.
 *		Accepted values: ADXL372_STANDBY
 *				 ADXL372_WAKE_UP
 *				 ADXL372_INSTANT_ON
 *				 ADXL372_FULL_BW_MEASUREMENT
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl372_set_op_mode(const struct device *dev,
			       enum adxl372_op_mode op_mode)
{
	struct adxl372_data *data = dev->data;

	return data->hw_tf->write_reg_mask(dev, ADXL372_POWER_CTL,
					   ADXL372_POWER_CTL_MODE_MSK,
					   ADXL372_POWER_CTL_MODE(op_mode));
}

/**
 * Autosleep. When set to 1, autosleep is enabled, and the device enters
 * wake-up mode automatically upon detection of inactivity.
 * @param dev - The device structure.
 * @param enable - Accepted values: true
 *				    false
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl372_set_autosleep(const struct device *dev, bool enable)
{
	struct adxl372_data *data = dev->data;

	return data->hw_tf->write_reg_mask(dev, ADXL372_MEASURE,
					   ADXL372_MEASURE_AUTOSLEEP_MSK,
					   ADXL372_MEASURE_AUTOSLEEP_MODE(enable));
}

/**
 * Select the desired output signal bandwidth.
 * @param dev - The device structure.
 * @param bw - bandwidth.
 *		Accepted values: ADXL372_BW_200HZ
 *				 ADXL372_BW_400HZ
 *				 ADXL372_BW_800HZ
 *				 ADXL372_BW_1600HZ
 *				 ADXL372_BW_3200HZ
 *				 ADXL372_BW_LPF_DISABLED
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl372_set_bandwidth(const struct device *dev,
				 enum adxl372_bandwidth bw)
{
	int ret;
	uint8_t mask;
	struct adxl372_data *data = dev->data;

	if (bw == ADXL372_BW_LPF_DISABLED) {
		mask = ADXL372_POWER_CTL_LPF_DIS_MSK;
	} else {
		mask = 0U;
	}

	ret = data->hw_tf->write_reg_mask(dev, ADXL372_POWER_CTL,
					  ADXL372_POWER_CTL_LPF_DIS_MSK, mask);
	if (ret) {
		return ret;
	}

	return data->hw_tf->write_reg_mask(dev, ADXL372_MEASURE,
					   ADXL372_MEASURE_BANDWIDTH_MSK,
					   ADXL372_MEASURE_BANDWIDTH_MODE(bw));
}

/**
 * Select the desired high-pass filter corner.
 * @param dev - The device structure.
 * @param c - bandwidth.
 *		Accepted values: ADXL372_HPF_CORNER_0
 *				 ADXL372_HPF_CORNER_1
 *				 ADXL372_HPF_CORNER_2
 *				 ADXL372_HPF_CORNER_3
 *				 ADXL372_HPF_DISABLED
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl372_set_hpf_corner(const struct device *dev,
				  enum adxl372_hpf_corner c)
{

	int ret;
	uint8_t mask;
	struct adxl372_data *data = dev->data;

	if (c == ADXL372_HPF_DISABLED) {
		mask = ADXL372_POWER_CTL_HPF_DIS_MSK;
	} else {
		mask = 0U;
	}

	ret = data->hw_tf->write_reg_mask(dev, ADXL372_POWER_CTL,
					  ADXL372_POWER_CTL_HPF_DIS_MSK, mask);
	if (ret) {
		return ret;
	}

	return data->hw_tf->write_reg(dev, ADXL372_HPF, ADXL372_HPF_CORNER(c));
}


/**
 * Link/Loop Activity Processing.
 * @param dev - The device structure.
 * @param mode - Mode of operation.
 *		Accepted values: ADXL372_DEFAULT
 *				 ADXL372_LINKED
 *				 ADXL372_LOOPED
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl372_set_act_proc_mode(const struct device *dev,
				     enum adxl372_act_proc_mode mode)
{
	struct adxl372_data *data = dev->data;

	return data->hw_tf->write_reg_mask(dev, ADXL372_MEASURE,
					   ADXL372_MEASURE_LINKLOOP_MSK,
					   ADXL372_MEASURE_LINKLOOP_MODE(mode));
}

/**
 * Set Output data rate.
 * @param dev - The device structure.
 * @param odr - Output data rate.
 *		Accepted values: ADXL372_ODR_400HZ
 *				 ADXL372_ODR_800HZ
 *				 ADXL372_ODR_1600HZ
 *				 ADXL372_ODR_3200HZ
 *				 ADXL372_ODR_6400HZ
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl372_set_odr(const struct device *dev, enum adxl372_odr odr)
{
	struct adxl372_data *data = dev->data;

	return data->hw_tf->write_reg_mask(dev, ADXL372_TIMING,
					   ADXL372_TIMING_ODR_MSK,
					   ADXL372_TIMING_ODR_MODE(odr));
}

/**
 * Select instant on threshold
 * @param dev - The device structure.
 * @param mode - 0 = low threshold, 1 = high threshold.
 *		Accepted values: ADXL372_INSTANT_ON_LOW_TH
 *				 ADXL372_INSTANT_ON_HIGH_TH
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl372_set_instant_on_th(const struct device *dev,
				     enum adxl372_instant_on_th_mode mode)
{
	struct adxl372_data *data = dev->data;

	return data->hw_tf->write_reg_mask(dev, ADXL372_POWER_CTL,
					   ADXL372_POWER_CTL_INSTANT_ON_TH_MSK,
					   ADXL372_POWER_CTL_INSTANT_ON_TH_MODE(mode));
}

/**
 * Set the Timer Rate for Wake-Up Mode.
 * @param dev - The device structure.
 * @param wur - wake up mode rate
 *		Accepted values: ADXL372_WUR_52ms
 *				 ADXL372_WUR_104ms
 *				 ADXL372_WUR_208ms
 *				 ADXL372_WUR_512ms
 *				 ADXL372_WUR_2048ms
 *				 ADXL372_WUR_4096ms
 *				 ADXL372_WUR_8192ms
 *				 ADXL372_WUR_24576ms
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl372_set_wakeup_rate(const struct device *dev,
				   enum adxl372_wakeup_rate wur)
{
	struct adxl372_data *data = dev->data;

	return data->hw_tf->write_reg_mask(dev, ADXL372_TIMING,
					   ADXL372_TIMING_WAKE_UP_RATE_MSK,
					   ADXL372_TIMING_WAKE_UP_RATE_MODE(wur));
}

/**
 * Set the activity timer
 * @param dev - The device structure.
 * @param time - The value set in this register.
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl372_set_activity_time(const struct device *dev, uint8_t time)
{
	struct adxl372_data *data = dev->data;

	return data->hw_tf->write_reg(dev, ADXL372_TIME_ACT, time);
}

/**
 * Set the inactivity timer
 * @param dev - The device structure.
 * @param time - is the 16-bit value set by the TIME_INACT_L register
 *		 (eight LSBs) and the TIME_INACT_H register (eight MSBs).
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl372_set_inactivity_time(const struct device *dev,
				       uint16_t time)
{
	int ret;
	struct adxl372_data *data = dev->data;

	ret = data->hw_tf->write_reg(dev, ADXL372_TIME_INACT_H, time >> 8);
	if (ret) {
		return ret;
	}

	return data->hw_tf->write_reg(dev, ADXL372_TIME_INACT_L, time & 0xFF);
}

/**
 * Set the filter settling period.
 * @param dev - The device structure.
 * @param mode - settle period
 *		Accepted values: ADXL372_FILTER_SETTLE_370
 *				 ADXL372_FILTER_SETTLE_16
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl372_set_filter_settle(const struct device *dev,
				     enum adxl372_filter_settle mode)
{
	struct adxl372_data *data = dev->data;

	return data->hw_tf->write_reg_mask(dev, ADXL372_POWER_CTL,
					   ADXL372_POWER_CTL_FIL_SETTLE_MSK,
					   ADXL372_POWER_CTL_FIL_SETTLE_MODE(mode));
}

/**
 * Configure the INT1 and INT2 interrupt pins.
 * @param dev - The device structure.
 * @param int1 -  INT1 interrupt pins.
 * @param int2 -  INT2 interrupt pins.
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl372_interrupt_config(const struct device *dev,
				    uint8_t int1,
				    uint8_t int2)
{
	int ret;
	struct adxl372_data *data = dev->data;

	ret = data->hw_tf->write_reg(dev, ADXL372_INT1_MAP, int1);
	if (ret) {
		return ret;
	}

	return  data->hw_tf->write_reg(dev, ADXL372_INT2_MAP, int2);

}

/**
 * Get the STATUS, STATUS2, FIFO_ENTRIES and FIFO_ENTRIES2 registers data
 * @param dev - The device structure.
 * @param status1 - Data stored in the STATUS1 register
 * @param status2 - Data stored in the STATUS2 register
 * @param fifo_entries - Number of valid data samples present in the
 *			 FIFO buffer (0 to 512)
 * @return 0 in case of success, negative error code otherwise.
 */
int adxl372_get_status(const struct device *dev,
			   uint8_t *status1,
			   uint8_t *status2,
			   uint16_t *fifo_entries)
{
	struct adxl372_data *data = dev->data;
	uint8_t buf[4], length = 1U;
	int ret;

	if (status2) {
		length++;
	}

	if (fifo_entries) {
		length += 2U;
	}

	ret = data->hw_tf->read_reg_multiple(dev, ADXL372_STATUS_1, buf, length);

	*status1 = buf[0];

	if (status2) {
		*status2 = buf[1];
	}

	if (fifo_entries) {
		*fifo_entries = ((buf[2] & 0x3) << 8) | buf[3];
	}

	return ret;
}

/**
 * Software reset.
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl372_reset(const struct device *dev)
{
	int ret;
	struct adxl372_data *data = dev->data;

	ret = adxl372_set_op_mode(dev, ADXL372_STANDBY);
	if (ret) {
		return ret;
	}
	/* Writing code 0x52 resets the device */
	ret = data->hw_tf->write_reg(dev, ADXL372_RESET, ADXL372_RESET_CODE);
	k_sleep(K_MSEC(1000));

	return ret;
}

/**
 * Configure the operating parameters for the FIFO.
 * @param dev - The device structure.
 * @param mode - FIFO Mode. Specifies FIFO operating mode.
 *		Accepted values: ADXL372_FIFO_BYPASSED
 *				 ADXL372_FIFO_STREAMED
 *				 ADXL372_FIFO_TRIGGERED
 *				 ADXL372_FIFO_OLD_SAVED
 * @param format - FIFO Format. Specifies the data is stored in the FIFO buffer.
 *		Accepted values: ADXL372_XYZ_FIFO
 *				 ADXL372_X_FIFO
 *				 ADXL372_Y_FIFO
 *				 ADXL372_XY_FIFO
 *				 ADXL372_Z_FIFO
 *				 ADXL372_XZ_FIFO
 *				 ADXL372_YZ_FIFO
 *				 ADXL372_XYZ_PEAK_FIFO
 * @param fifo_samples - FIFO Samples. Watermark number of FIFO samples that
 *			triggers a FIFO_FULL condition when reached.
 *			Values range from 0 to 512.

 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl372_configure_fifo(const struct device *dev,
				  enum adxl372_fifo_mode mode,
				  enum adxl372_fifo_format format,
				  uint16_t fifo_samples)
{
	struct adxl372_data *data = dev->data;
	uint8_t fifo_config;
	int ret;

	if (fifo_samples > 512) {
		return -EINVAL;
	}

	/*
	 * All FIFO modes must be configured while in standby mode.
	 */
	ret = adxl372_set_op_mode(dev, ADXL372_STANDBY);
	if (ret) {
		return ret;
	}

	fifo_config = (ADXL372_FIFO_CTL_FORMAT_MODE(format) |
		       ADXL372_FIFO_CTL_MODE_MODE(mode) |
		       ADXL372_FIFO_CTL_SAMPLES_MODE(fifo_samples));

	ret = data->hw_tf->write_reg(dev, ADXL372_FIFO_CTL, fifo_config);
	if (ret) {
		return ret;
	}
	ret = data->hw_tf->write_reg(dev, ADXL372_FIFO_SAMPLES, fifo_samples & 0xFF);
	if (ret) {
		return ret;
	}

	data->fifo_config.fifo_format = format;
	data->fifo_config.fifo_mode = mode;
	data->fifo_config.fifo_samples = fifo_samples;

	return 0;
}

/**
 * Retrieve 3-axis acceleration data
 * @param dev - The device structure.
 * @param maxpeak - Retrieve the highest magnitude (x, y, z) sample recorded
 *		    since the last read of the MAXPEAK registers
 * @param accel_data - pointer to a variable of type adxl372_xyz_accel_data
 *		      where (x, y, z) acceleration data will be stored.
 * @return 0 in case of success, negative error code otherwise.
 */
static int adxl372_get_accel_data(const struct device *dev, bool maxpeak,
				  struct adxl372_xyz_accel_data *accel_data)
{
	struct adxl372_data *data = dev->data;
	uint8_t buf[6];
	uint8_t status1;
	int ret;

	if (!IS_ENABLED(CONFIG_ADXL372_TRIGGER)) {
		do {
			adxl372_get_status(dev, &status1, NULL, NULL);
		} while (!(ADXL372_STATUS_1_DATA_RDY(status1)));
	}

	ret = data->hw_tf->read_reg_multiple(dev, maxpeak ? ADXL372_X_MAXPEAK_H :
					     ADXL372_X_DATA_H, buf, 6);

	accel_data->x = (buf[0] << 8) | (buf[1] & 0xF0);
	accel_data->y = (buf[2] << 8) | (buf[3] & 0xF0);
	accel_data->z = (buf[4] << 8) | (buf[5] & 0xF0);

	return ret;
}

static int adxl372_attr_set_odr(const struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	enum adxl372_odr odr;

	switch (val->val1) {
	case 400:
		odr = ADXL372_ODR_400HZ;
		break;
	case 800:
		odr = ADXL372_ODR_800HZ;
		break;
	case 1600:
		odr = ADXL372_ODR_1600HZ;
		break;
	case 3200:
		odr = ADXL372_ODR_3200HZ;
		break;
	case 6400:
		odr = ADXL372_ODR_6400HZ;
		break;
	default:
		return -EINVAL;
	}

	return adxl372_set_odr(dev, odr);
}

static int adxl372_attr_set_thresh(const struct device *dev,
				   enum sensor_channel chan,
				   enum sensor_attribute attr,
				   const struct sensor_value *val)
{
	const struct adxl372_dev_config *cfg = dev->config;
	struct adxl372_activity_threshold threshold;
	int64_t llvalue;
	int32_t value;
	int64_t micro_ms2 = val->val1 * 1000000LL + val->val2;
	uint8_t reg;

	llvalue = llabs((micro_ms2 * 10) / SENSOR_G);

	if (llvalue > 2047) {
		return -EINVAL;
	}

	value = (int32_t) llvalue;

	threshold.thresh = value;
	threshold.enable = cfg->activity_th.enable;
	threshold.referenced = cfg->activity_th.referenced;

	if (attr ==  SENSOR_ATTR_UPPER_THRESH) {
		reg = ADXL372_X_THRESH_ACT_H;
	} else {
		reg = ADXL372_X_THRESH_INACT_H;
	}

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		return adxl372_set_activity_threshold(dev, reg, &threshold);
	case SENSOR_CHAN_ACCEL_Y:
		return adxl372_set_activity_threshold(dev, reg + 2, &threshold);
	case SENSOR_CHAN_ACCEL_Z:
		return adxl372_set_activity_threshold(dev, reg + 4, &threshold);
	case SENSOR_CHAN_ACCEL_XYZ:
		return adxl372_set_activity_threshold_xyz(dev, reg, &threshold);
	default:
		LOG_ERR("attr_set() not supported on this channel");
		return -ENOTSUP;
	}
}

static int adxl372_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return adxl372_attr_set_odr(dev, chan, attr, val);
	case SENSOR_ATTR_UPPER_THRESH:
	case SENSOR_ATTR_LOWER_THRESH:
		return adxl372_attr_set_thresh(dev, chan, attr, val);
	default:
		return -ENOTSUP;
	}
}

static int adxl372_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	const struct adxl372_dev_config *cfg = dev->config;
	struct adxl372_data *data = dev->data;

	return adxl372_get_accel_data(dev, cfg->max_peak_detect_mode,
				      &data->sample);
}

static void adxl372_accel_convert(struct sensor_value *val, int16_t value)
{
	/*
	 * Sensor resolution is 100mg/LSB, 12-bit value needs to be right
	 * shifted by 4 or divided by 16. Overall this results in a scale of 160
	 */
	int32_t micro_ms2 = value * (SENSOR_G / (16 * 1000 / 100));

	val->val1 = micro_ms2 / 1000000;
	val->val2 = micro_ms2 % 1000000;
}

static int adxl372_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct adxl372_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		adxl372_accel_convert(val, data->sample.x);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		adxl372_accel_convert(val, data->sample.y);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		adxl372_accel_convert(val, data->sample.z);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		adxl372_accel_convert(val++, data->sample.x);
		adxl372_accel_convert(val++, data->sample.y);
		adxl372_accel_convert(val, data->sample.z);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api adxl372_api_funcs = {
	.attr_set     = adxl372_attr_set,
	.sample_fetch = adxl372_sample_fetch,
	.channel_get  = adxl372_channel_get,
#ifdef CONFIG_ADXL372_TRIGGER
	.trigger_set = adxl372_trigger_set,
#endif

};

static int adxl372_probe(const struct device *dev)
{
	const struct adxl372_dev_config *cfg = dev->config;
	struct adxl372_data *data = dev->data;
	uint8_t dev_id, part_id;
	int ret;

	ret = data->hw_tf->read_reg(dev, ADXL372_DEVID, &dev_id);
	if (ret) {
		return ret;
	}
	ret = data->hw_tf->read_reg(dev, ADXL372_PARTID, &part_id);
	if (ret) {
		return ret;
	}

	if (dev_id != ADXL372_DEVID_VAL || part_id != ADXL372_PARTID_VAL) {
		LOG_ERR("failed to read id (0x%X:0x%X)", dev_id, part_id);
		return -ENODEV;
	}

#ifdef CONFIG_ADXL372_TRIGGER
	data->act_proc_mode = ADXL372_LINKED,
#else
	data->act_proc_mode = ADXL372_LOOPED,
#endif

	/* Device settings */
	ret = adxl372_set_op_mode(dev, ADXL372_STANDBY);
	if (ret) {
		return ret;
	}

	ret = adxl372_reset(dev);
	if (ret) {
		return ret;
	}

	ret = adxl372_set_hpf_corner(dev, cfg->hpf);
	if (ret) {
		return ret;
	}

	ret = adxl372_set_bandwidth(dev, cfg->bw);
	if (ret) {
		return ret;
	}

	ret = adxl372_set_odr(dev, cfg->odr);
	if (ret) {
		return ret;
	}

	ret = adxl372_set_wakeup_rate(dev, cfg->wur);
	if (ret) {
		return ret;
	}

	ret = adxl372_set_autosleep(dev, cfg->autosleep);
	if (ret) {
		return ret;
	}

	ret = adxl372_set_instant_on_th(dev, cfg->th_mode);
	if (ret) {
		return ret;
	}

	ret = adxl372_set_activity_threshold_xyz(dev, ADXL372_X_THRESH_ACT_H,
						 &cfg->activity_th);
	if (ret) {
		return ret;
	}

	ret = adxl372_set_activity_threshold_xyz(dev, ADXL372_X_THRESH_INACT_H,
						 &cfg->inactivity_th);
	if (ret) {
		return ret;
	}

	ret = adxl372_set_activity_time(dev, cfg->activity_time);
	if (ret) {
		return ret;
	}

	ret = adxl372_set_inactivity_time(dev, cfg->inactivity_time);
	if (ret) {
		return ret;
	}

	ret = adxl372_set_filter_settle(dev, cfg->filter_settle);
	if (ret) {
		return ret;
	}

	ret = adxl372_configure_fifo(dev, cfg->fifo_config.fifo_mode,
				     cfg->fifo_config.fifo_format,
				     cfg->fifo_config.fifo_samples);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_ADXL372_TRIGGER
	if (adxl372_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt!");
		return -EIO;
	}
#endif

	ret = adxl372_interrupt_config(dev, cfg->int1_config, cfg->int2_config);
	if (ret) {
		return ret;
	}

	ret = adxl372_set_op_mode(dev, cfg->op_mode);
	if (ret) {
		return ret;
	}

	return adxl372_set_act_proc_mode(dev, data->act_proc_mode);
}

static int adxl372_init(const struct device *dev)
{
	int ret;
	const struct adxl372_dev_config *cfg = dev->config;

	ret = cfg->bus_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize sensor bus");
		return ret;
	}

	if (adxl372_probe(dev) < 0) {
		return -ENODEV;
	}

	return 0;
}

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "ADXL372 driver enabled without any devices"
#endif

/*
 * Device creation macro, shared by ADXL372_DEFINE_SPI() and
 * ADXL372_DEFINE_I2C().
 */

#define ADXL372_DEVICE_INIT(inst)					\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,				\
			      adxl372_init,				\
			      NULL,					\
			      &adxl372_data_##inst,			\
			      &adxl372_config_##inst,			\
			      POST_KERNEL,				\
			      CONFIG_SENSOR_INIT_PRIORITY,		\
			      &adxl372_api_funcs);

/*
 * Instantiation macros used when a device is on a SPI bus.
 */

#ifdef CONFIG_ADXL372_TRIGGER
#define ADXL372_CFG_IRQ(inst) \
		.interrupt = GPIO_DT_SPEC_INST_GET(inst, int1_gpios),
#else
#define ADXL372_CFG_IRQ(inst)
#endif /* CONFIG_ADXL372_TRIGGER */

#define ADXL372_CONFIG(inst)								\
		.bw = DT_INST_PROP(inst, bw),						\
		.hpf = DT_INST_PROP(inst, hpf),						\
		.odr = DT_INST_PROP(inst, odr),						\
		.max_peak_detect_mode = IS_ENABLED(CONFIG_ADXL372_PEAK_DETECT_MODE),	\
		.th_mode = ADXL372_INSTANT_ON_LOW_TH,					\
		.autosleep = false,							\
		.wur = ADXL372_WUR_52ms,						\
		.activity_th.thresh = CONFIG_ADXL372_ACTIVITY_THRESHOLD / 100,		\
		.activity_th.referenced =						\
			IS_ENABLED(CONFIG_ADXL372_REFERENCED_ACTIVITY_DETECTION_MODE),	\
		.activity_th.enable = 1,						\
		.activity_time = CONFIG_ADXL372_ACTIVITY_TIME,				\
		.inactivity_th.thresh = CONFIG_ADXL372_INACTIVITY_THRESHOLD / 100,	\
		.inactivity_th.referenced =						\
			IS_ENABLED(CONFIG_ADXL372_REFERENCED_ACTIVITY_DETECTION_MODE),	\
		.inactivity_th.enable = 1,						\
		.inactivity_time = CONFIG_ADXL372_INACTIVITY_TIME,			\
		.filter_settle = ADXL372_FILTER_SETTLE_370,				\
		.fifo_config.fifo_mode = ADXL372_FIFO_STREAMED,				\
		.fifo_config.fifo_format = ADXL372_XYZ_PEAK_FIFO,			\
		.fifo_config.fifo_samples = 128,					\
		.op_mode = ADXL372_FULL_BW_MEASUREMENT,					\

#define ADXL372_CONFIG_SPI(inst)					\
	{								\
		.bus_init = adxl372_spi_init,				\
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8) |	\
					SPI_TRANSFER_MSB, 0),		\
		ADXL372_CONFIG(inst)					\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int1_gpios),	\
		(ADXL372_CFG_IRQ(inst)), ())				\
	}

#define ADXL372_DEFINE_SPI(inst)					\
	static struct adxl372_data adxl372_data_##inst;			\
	static const struct adxl372_dev_config adxl372_config_##inst =	\
		ADXL372_CONFIG_SPI(inst);				\
	ADXL372_DEVICE_INIT(inst)

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define ADXL372_CONFIG_I2C(inst)					\
	{								\
		.bus_init = adxl372_i2c_init,				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),			\
		ADXL372_CONFIG(inst)					\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int1_gpios),	\
		(ADXL372_CFG_IRQ(inst)), ())				\
	}

#define ADXL372_DEFINE_I2C(inst)					\
	static struct adxl372_data adxl372_data_##inst;			\
	static const struct adxl372_dev_config adxl372_config_##inst =	\
		ADXL372_CONFIG_I2C(inst);				\
	ADXL372_DEVICE_INIT(inst)
/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define ADXL372_DEFINE(inst)						\
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),				\
		    (ADXL372_DEFINE_SPI(inst)),				\
		    (ADXL372_DEFINE_I2C(inst)))

DT_INST_FOREACH_STATUS_OKAY(ADXL372_DEFINE)
