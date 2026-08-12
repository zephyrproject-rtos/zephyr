/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <string.h>
#include "maxm86161.h"

#define DT_DRV_COMPAT adi_maxm86161

LOG_MODULE_REGISTER(MAXM86161, CONFIG_SENSOR_LOG_LEVEL);

/*
 * Attribute descriptor look-up table. Each entry is placed at the index derived
 * from its attribute ID (MAXM86161_ATTR_IDX) so maxm86161_attr_search() resolves
 * an attribute to its descriptor via direct indexing.
 */
#define MAXM86161_ATTR_IDX(_attr) ((_attr) - SENSOR_ATTR_PRIV_START)

static int maxm86161_update_odr_cb(const struct device *dev, enum sensor_channel channel,
				   enum sensor_attribute attr, const struct sensor_value *val);

static int maxm86161_probe(const struct device *dev);

static int maxm86161_reset_cb(const struct device *dev, enum sensor_channel channel,
			      enum sensor_attribute attr, const struct sensor_value *val);

static int maxm86161_update_led_state_cb(const struct device *dev,
					 enum sensor_channel channel,
					 enum sensor_attribute attr,
					 const struct sensor_value *val);

static int maxm86161_prep_watermark(const struct device *dev, enum sensor_channel channel,
				    enum sensor_attribute attr, struct sensor_value *val);

static int maxm86161_prep_burst_en(const struct device *dev, enum sensor_channel channel,
				     enum sensor_attribute attr, struct sensor_value *val);

const struct maxm86161_attr_desc maxm86161_attr_table[] = {
	{(int)SENSOR_ATTR_MAXM86161_FIFO_WATERMARK, MAXM86161_REG_FIFO_CONFIG1,
	 MAXM86161_MSK_FIFO_A_FULL, 0, maxm86161_prep_watermark, NULL},
	{(int)SENSOR_ATTR_MAXM86161_FIFO_FLUSH, MAXM86161_REG_FIFO_CONFIG2,
	 MAXM86161_MSK_FIFO_FLUSH, MAXM86161_ATTR_FLAG_WR_ONLY, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_FIFO_A_FULL_TYPE, MAXM86161_REG_FIFO_CONFIG2,
	 MAXM86161_MSK_FIFO_A_FULL_TYPE, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_FIFO_ROLLOVER, MAXM86161_REG_FIFO_CONFIG2,
	 MAXM86161_MSK_FIFO_ROLLOVER, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_LOW_POWER_MODE, MAXM86161_REG_SYSTEM_CONTROL,
	 MAXM86161_MSK_SYSTEM_CONTROL_LP_MODE, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_RESET, MAXM86161_REG_SYSTEM_CONTROL,
	 MAXM86161_MSK_SYSTEM_CONTROL_RESET, 0, NULL, maxm86161_reset_cb},
	{(int)SENSOR_ATTR_MAXM86161_TIME_STAMP_ENABLE, MAXM86161_REG_PPG_SYNC_CONTROL,
	 MAXM86161_MSK_PPG_SYNC_CONTROL_TIME_STAMP_EN, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_DAC_CODE_CHANGE_TAG_ENABLE, MAXM86161_REG_PPG_SYNC_CONTROL,
	 MAXM86161_MSK_PPG_SYNC_CONTROL_DAC_CODE_CHG_TAG, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_SW_FORCE_SYNC_ENABLE, MAXM86161_REG_PPG_SYNC_CONTROL,
	 MAXM86161_MSK_PPG_SYNC_CONTROL_SW_FORCE_SYNC, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_ALC_DISABLE, MAXM86161_REG_PPG_CONFIG1,
	 MAXM86161_MSK_PPG_CONFIG1_ALC_DISABLE, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_ADD_OFFSET_ENABLE, MAXM86161_REG_PPG_CONFIG1,
	 MAXM86161_MSK_PPG_CONFIG1_ADD_OFFSET, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_ADC_RANGE, MAXM86161_REG_PPG_CONFIG1,
	 MAXM86161_MSK_PPG_CONFIG1_PPG1_ADC_RGE, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_LED_INTEGRATION_TIME, MAXM86161_REG_PPG_CONFIG1,
	 MAXM86161_MSK_PPG_CONFIG1_PPG_TINT, 0, NULL, NULL},
	{SENSOR_ATTR_SAMPLING_FREQUENCY, MAXM86161_REG_PPG_CONFIG2,
	 MAXM86161_MSK_PPG_CONFIG2_PPG_SR, 0, NULL, maxm86161_update_odr_cb},
	{(int)SENSOR_ATTR_MAXM86161_SAMPLE_AVERAGING, MAXM86161_REG_PPG_CONFIG2,
	 MAXM86161_MSK_PPG_CONFIG2_SMP_AVE, 0, NULL, maxm86161_update_odr_cb},
	{(int)SENSOR_ATTR_MAXM86161_LED_SETTLING_TIME, MAXM86161_REG_PPG_CONFIG3,
	 MAXM86161_MSK_PPG_CONFIG3_LED_SETLNG, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_DIGITAL_FILTER_SELECT, MAXM86161_REG_PPG_CONFIG3,
	 MAXM86161_MSK_PPG_CONFIG3_DIG_FILT_SEL, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_BURST_RATE, MAXM86161_REG_PPG_CONFIG3,
	 MAXM86161_MSK_PPG_CONFIG3_BURST_RATE, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_BURST_ENABLE, MAXM86161_REG_PPG_CONFIG3,
	 MAXM86161_MSK_PPG_CONFIG3_BURST_EN, 0, maxm86161_prep_burst_en, NULL},
	{(int)SENSOR_ATTR_MAXM86161_PROX_INT_THRESHOLD, MAXM86161_REG_PPG_PROX_INT_THRESH,
	 MAXM86161_MSK_PPG_PROX_INT_THRESH, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_PHOTODIODE_BIAS, MAXM86161_REG_PPG_PD_BIAS,
	 MAXM86161_MSK_PPG_PD_BIAS, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_LED_SEQUENCE_1, MAXM86161_REG_LED_SEQ_REG1,
	 MAXM86161_MSK_LED_SEQ_ODD, 0, NULL, maxm86161_update_led_state_cb},
	{(int)SENSOR_ATTR_MAXM86161_LED_SEQUENCE_2, MAXM86161_REG_LED_SEQ_REG1,
	 MAXM86161_MSK_LED_SEQ_EVEN, 0, NULL, maxm86161_update_led_state_cb},
	{(int)SENSOR_ATTR_MAXM86161_LED_SEQUENCE_3, MAXM86161_REG_LED_SEQ_REG2,
	 MAXM86161_MSK_LED_SEQ_ODD, 0, NULL, maxm86161_update_led_state_cb},
	{(int)SENSOR_ATTR_MAXM86161_LED_SEQUENCE_4, MAXM86161_REG_LED_SEQ_REG2,
	 MAXM86161_MSK_LED_SEQ_EVEN, 0, NULL, maxm86161_update_led_state_cb},
	{(int)SENSOR_ATTR_MAXM86161_LED_SEQUENCE_5, MAXM86161_REG_LED_SEQ_REG3,
	 MAXM86161_MSK_LED_SEQ_ODD, 0, NULL, maxm86161_update_led_state_cb},
	{(int)SENSOR_ATTR_MAXM86161_LED_SEQUENCE_6, MAXM86161_REG_LED_SEQ_REG3,
	 MAXM86161_MSK_LED_SEQ_EVEN, 0, NULL, maxm86161_update_led_state_cb},
	{(int)SENSOR_ATTR_MAXM86161_LED1_GREEN_CURRENT, MAXM86161_REG_LED1_PA, MAXM86161_MSK_LED_PA,
	 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_LED2_IR_CURRENT, MAXM86161_REG_LED2_PA, MAXM86161_MSK_LED_PA, 0,
	 NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_LED3_RED_CURRENT, MAXM86161_REG_LED3_PA, MAXM86161_MSK_LED_PA,
	 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_PILOT_ON_GREEN_LED_CURRENT, MAXM86161_REG_LED_PILOT_PA,
	 MAXM86161_MSK_LED_PA, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_LED1_GREEN_CURRENT_RANGE, MAXM86161_REG_LED_RANGE_1,
	 MAXM86161_MSK_LED_RANGE1_LED1_RGE, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_LED2_IR_CURRENT_RANGE, MAXM86161_REG_LED_RANGE_1,
	 MAXM86161_MSK_LED_RANGE1_LED2_RGE, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_LED3_RED_CURRENT_RANGE, MAXM86161_REG_LED_RANGE_1,
	 MAXM86161_MSK_LED_RANGE1_LED3_RGE, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_DAC_CALIBRATION, MAXM86161_REG_DAC_CALIBRATION,
	 MAXM86161_MSK_DAC_START_CAL, 0, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_STATUS1, MAXM86161_REG_INT_STATUS1, MAXM86161_MSK_INT_STATUS1,
	 MAXM86161_ATTR_FLAG_RD_ONLY, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_SHA_DONE, MAXM86161_REG_INT_STATUS2,
	 MAXM86161_MSK_INT_STATUS2_SHA_DONE, MAXM86161_ATTR_FLAG_RD_ONLY, NULL, NULL},
	{SENSOR_ATTR_CHIP_ID, MAXM86161_REG_PART_ID, MAXM86161_MSK_PART_ID_PART_ID,
	 MAXM86161_ATTR_FLAG_RD_ONLY, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_FIFO_OVERFLOW_COUNT, MAXM86161_REG_FIFO_OVF_COUNTER,
	 MAXM86161_MSK_FIFO_OVF_COUNTER, MAXM86161_ATTR_FLAG_RD_ONLY, NULL, NULL},
	{(int)SENSOR_ATTR_MAXM86161_FIFO_DATA_COUNT, MAXM86161_REG_FIFO_DATA_COUNTER,
	 MAXM86161_MSK_FIFO_DATA_COUNT, MAXM86161_ATTR_FLAG_RD_ONLY, NULL, NULL},
};

static const struct i2c_dt_spec *get_i2c_spec(const struct device *dev)
{
	const struct maxm86161_config *dcfg = dev->config;

	return &dcfg->i2c;
}

int maxm86161_i2c_write_byte(const struct device *dev, uint8_t reg_addr, uint8_t value)
{
	return i2c_reg_write_byte_dt(get_i2c_spec(dev), reg_addr, value);
}

int maxm86161_i2c_update_byte(const struct device *dev, uint8_t reg_addr, uint8_t mask,
			      uint8_t value)
{
	uint8_t field_val = FIELD_PREP(mask, value);

	return i2c_reg_update_byte_dt(get_i2c_spec(dev), reg_addr, mask, field_val);
}

int maxm86161_i2c_read_byte(const struct device *dev, uint8_t reg_addr, uint8_t *rd_buf)
{
	return i2c_reg_read_byte_dt(get_i2c_spec(dev), reg_addr, rd_buf);
}

int maxm86161_i2c_burst_write(const struct device *dev, uint8_t start_addr, uint8_t *wr_buf,
			      uint8_t num_bytes)
{
	return i2c_burst_write_dt(get_i2c_spec(dev), start_addr, wr_buf, num_bytes);
}

int maxm86161_i2c_burst_read(const struct device *dev, uint8_t start_addr, uint8_t *rd_buf,
			     uint8_t num_bytes)
{
	return i2c_burst_read_dt(get_i2c_spec(dev), start_addr, rd_buf, num_bytes);
}

static int maxm86161_update_odr_cb(const struct device *dev, enum sensor_channel channel,
				   enum sensor_attribute attr, const struct sensor_value *val)
{
	struct maxm86161_data *data = dev->data;
	uint8_t reg_val;
	uint8_t ppg_sr, smp_avg;
	int ret;

	ret = maxm86161_i2c_read_byte(dev, MAXM86161_REG_PPG_CONFIG2, &reg_val);
	if (ret) {
		return ret;
	}

	ppg_sr = FIELD_GET(MAXM86161_MSK_PPG_CONFIG2_PPG_SR, reg_val);
	smp_avg = FIELD_GET(MAXM86161_MSK_PPG_CONFIG2_SMP_AVE, reg_val);

	data->odr = (uint32_t)sample_rate[ppg_sr] / sample_avg[smp_avg];

	return 0;
}

static int maxm86161_reset_cb(const struct device *dev, enum sensor_channel channel,
			      enum sensor_attribute attr, const struct sensor_value *val)
{
	k_sleep(K_USEC(MAXM86161_RESET_DELAY_US));

	return maxm86161_probe(dev);
}

static void maxm86161_update_fifo_data_map(const struct device *dev, uint8_t seq_idx,
					   uint8_t exposure_type)
{
	struct maxm86161_data *ddata = dev->data;
	uint8_t prev = ddata->led_state.map[seq_idx];

	/* Undo the previous contribution of this sequence slot, if any. */
	if (prev != MAXM86161_EXPOSURE_NONE) {
		ddata->led_state.chan_pos[prev] = MAXM86161_LED_STATE_POS_NONE;
		ddata->led_state.num_active_channels--;
	}

	ddata->led_state.map[seq_idx] = exposure_type;

	if (exposure_type == MAXM86161_EXPOSURE_NONE) {
		return;
	}

	if (exposure_type == MAXM86161_EXPOSURE_PILOT_ON_GREEN) {
		ddata->led_state.chan_pos[exposure_type] = MAXM86161_LED_STATE_POS_PROX;
	} else {
		ddata->led_state.chan_pos[exposure_type] = seq_idx;
	}

	ddata->led_state.num_active_channels++;
}

/*
 * Build the FIFO data map from the statically configured LED sequence. All
 * channel positions start as "none" so that channels whose exposure is not part
 * of the sequence (including proximity via the pilot LED) are reported as
 * uninitialized by the fetch/get guards.
 */
static void maxm86161_init_led_state(const struct device *dev)
{
	const struct maxm86161_config *dcfg = dev->config;
	struct maxm86161_data *ddata = dev->data;

	memset(ddata->led_state.map, MAXM86161_EXPOSURE_NONE, sizeof(ddata->led_state.map));
	for (size_t i = 0; i < ARRAY_SIZE(ddata->led_state.chan_pos); i++) {
		ddata->led_state.chan_pos[i] = MAXM86161_LED_STATE_POS_NONE;
	}
	ddata->led_state.num_active_channels = 0;

	for (uint8_t slot = 0; slot < MAXM86161_LED_SEQ_COUNT; slot++) {
		maxm86161_update_fifo_data_map(dev, slot, dcfg->led_cfg.led_seq[slot]);
	}
}

static int maxm86161_update_led_state_cb(const struct device *dev, enum sensor_channel channel,
					 enum sensor_attribute attr, const struct sensor_value *val)
{
	maxm86161_update_fifo_data_map(dev, (int)attr - SENSOR_ATTR_MAXM86161_LED_SEQUENCE_1,
				       (uint8_t)val->val1);

	return 0;
}

static const struct maxm86161_attr_desc *maxm86161_attr_search(enum sensor_attribute attr)
{
	for (size_t i = 0; i < ARRAY_SIZE(maxm86161_attr_table); i++) {
		if (maxm86161_attr_table[i].attr == attr) {
			return &maxm86161_attr_table[i];
		}
	}

	return NULL;
}

static inline uint8_t maxm86161_watermark_to_afull(uint8_t watermark)
{
	if (watermark == 0) {
		return 0;
	}

	return CLAMP(MAXM86161_FIFO_DEPTH - watermark, 1, MAXM86161_FIFO_WMARK_MAX);
}

static int maxm86161_prep_watermark(const struct device *dev, enum sensor_channel channel,
				    enum sensor_attribute attr, struct sensor_value *val)
{
	val->val1 = maxm86161_watermark_to_afull(val->val1);

	return 0;
}

static inline int8_t mamx86161_check_valid_burst_ave(uint8_t sample_rate_code,
						   uint8_t burst_rate_code)
{
	return sample_burst_count[sample_rate_code][burst_rate_code];
}

static int maxm86161_prep_burst_en(const struct device *dev, enum sensor_channel channel,
				     enum sensor_attribute attr, struct sensor_value *val)
{
	uint8_t sample_rate_code, burst_rate_code;
	int ret;

	ret = maxm86161_i2c_read_byte(dev, MAXM86161_REG_PPG_CONFIG2, &sample_rate_code);
	if (ret) {
		return ret;
	}

	sample_rate_code = FIELD_GET(MAXM86161_MSK_PPG_CONFIG2_PPG_SR, sample_rate_code);

	ret = maxm86161_i2c_read_byte(dev, MAXM86161_REG_PPG_CONFIG3, &burst_rate_code);
	if (ret) {
		return ret;
	}

	burst_rate_code = FIELD_GET(MAXM86161_MSK_PPG_CONFIG3_BURST_RATE, burst_rate_code);

	if (mamx86161_check_valid_burst_ave(sample_rate_code, burst_rate_code) < 0) {
		LOG_ERR("Cannot enable burst rate with current sample and burst rate.");
		return -EINVAL;
	}

	return 0;
}

static int maxm86161_attr_set(const struct device *dev, enum sensor_channel channel,
			      enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct maxm86161_attr_desc *attr_desc = maxm86161_attr_search(attr);
	struct sensor_value sensor_val = *val;
	size_t max_value;
	uint8_t reg_val;
	int ret;

	if ((int)channel != SENSOR_CHAN_MAXM86161_PPG) {
		LOG_ERR("Channel can only be PPG");
		return -ENOTSUP;
	}

	if (!attr_desc) {
		LOG_ERR("Attribute could not be found");
		return -ENOTSUP;
	}

	if (attr_desc->flags & MAXM86161_ATTR_FLAG_RD_ONLY) {
		LOG_ERR("Cannot write a read-only attribute");
		return -ENOTSUP;
	}

	max_value = FIELD_GET(attr_desc->mask, attr_desc->mask);

	/* use prep function if available to encode/validate input */
	if (attr_desc->set_prep != NULL) {
		ret = attr_desc->set_prep(dev, channel, attr, &sensor_val);
		if (ret) {
			return ret;
		}
	}

	if (sensor_val.val1 < 0 || sensor_val.val1 > max_value) {
		LOG_ERR("Invalid val %d out of range", sensor_val.val1);
		return -EINVAL;
	}

	reg_val = (uint8_t)sensor_val.val1;

	/* write as a whole register or as an update */
	if (max_value == UINT8_MAX) {
		ret = maxm86161_i2c_write_byte(dev, attr_desc->reg, reg_val);
		if (ret) {
			LOG_ERR("Error in writing to register %d: %d", attr_desc->reg, ret);
			return ret;
		}
	} else {
		ret = maxm86161_i2c_update_byte(dev, attr_desc->reg, attr_desc->mask, reg_val);
		if (ret) {
			LOG_ERR("Error in updating register %d: %d", attr_desc->reg, ret);
			return ret;
		}
	}

	/* Post-process the successful write (e.g. refresh cached ODR) */
	if (attr_desc->set_callback) {
		ret = attr_desc->set_callback(dev, channel, attr, val);
		if (ret) {
			LOG_ERR("Post-set callback failed for register %d: %d",
				attr_desc->reg, ret);
			return ret;
		}
	}

	return 0;
}

static int maxm86161_attr_get(const struct device *dev, enum sensor_channel channel,
			      enum sensor_attribute attr, struct sensor_value *val)
{
	const struct maxm86161_attr_desc *attr_desc = maxm86161_attr_search(attr);
	size_t max_value;
	uint8_t reg_val;
	int ret;

	if ((int)channel != SENSOR_CHAN_MAXM86161_PPG) {
		LOG_ERR("Channel can only be PPG");
		return -ENOTSUP;
	}

	if (!attr_desc) {
		LOG_ERR("Attribute could not be found");
		return -ENOTSUP;
	}

	if (attr_desc->flags & MAXM86161_ATTR_FLAG_WR_ONLY) {
		LOG_ERR("Cannot read a write-only attribute");
		return -ENOTSUP;
	}

	max_value = FIELD_GET(attr_desc->mask, attr_desc->mask);

	ret = maxm86161_i2c_read_byte(dev, attr_desc->reg, &reg_val);
	if (ret) {
		LOG_ERR("Error in reading register %d: %d", attr_desc->reg, ret);
		return ret;
	}

	/* if attribute is a bitmasked value in a register, get only that value */
	if (max_value != UINT8_MAX) {
		reg_val = FIELD_GET(attr_desc->mask, reg_val);
	}

	val->val1 = reg_val;
	val->val2 = 0;

	return 0;
}

int maxm86161_start_die_temp_meas(const struct device *dev)
{
	return maxm86161_i2c_write_byte(dev, MAXM86161_REG_DIE_TEMP_CONFIG, true);
}

static int maxm86161_poll_die_temp_meas(const struct device *dev)
{
	int ret;
	uint8_t reg_val = 0U;

	/* Trigger measurement */
	ret = maxm86161_start_die_temp_meas(dev);
	if (ret < 0) {
		return ret;
	}

	/* Wait until measurement completes, with timeout */
	for (size_t i = 0; i < MAXM86161_TEMP_MEAS_WAIT_TRIES; i++) {
		k_sleep(K_MSEC(MAXM86161_TEMP_MEAS_DELAY_MS));
		ret = maxm86161_i2c_read_byte(dev, MAXM86161_REG_DIE_TEMP_CONFIG, &reg_val);
		if (ret < 0) {
			return ret;
		}

		if (!FIELD_GET(MAXM86161_MSK_DIE_TEMP_CONFIG_TEMP_EN, reg_val)) {
			return 0;
		}
	}

	return -ETIMEDOUT;
}

static int maxm86161_fetch_die_temp(const struct device *dev)
{
	struct maxm86161_data *ddata = dev->data;
	int ret = 0;
	uint8_t temp_raw[2] = {0};

	ret = maxm86161_poll_die_temp_meas(dev);
	if (ret) {
		LOG_ERR("Could not get die temp: %d", ret);
	}

	ret = maxm86161_i2c_burst_read(dev, MAXM86161_REG_DIE_TEMP_INTEGER, temp_raw, 2);
	if (ret < 0) {
		return ret;
	}

	ddata->temp_val.val1 = (int8_t)temp_raw[0];
	ddata->temp_val.val2 = FIELD_GET(MAXM86161_MSK_DIE_TEMP_FRACTION_TEMP_FRAC, temp_raw[1]) *
			       MAXM86161_TEMP_FRAC_SCALE;

	return ret;
}

/* Performs a burst read of the FIFO frame to retrieve the latest sample for each enabled channel.
 */
static int maxm86161_fetch_fifo(const struct device *dev)
{
	struct maxm86161_data *ddata = dev->data;
	uint32_t fifo_raw;
	uint8_t *fifo_raw_ptr = (uint8_t *)&fifo_raw;
	uint8_t num_samples = ddata->led_state.num_active_channels;
	int ret;

	for (int i = 0; i < num_samples; i++) {
		ret = maxm86161_i2c_burst_read(dev, MAXM86161_REG_FIFO_DATA_REGISTER, fifo_raw_ptr,
					       MAXM86161_FIFO_SAMPLE_SIZE);
		if (ret < 0) {
			return ret;
		}

		fifo_raw = sys_be24_to_cpu(fifo_raw);

		uint32_t fifo_data = FIELD_GET(MAXM86161_FIFO_DATA_MASK, fifo_raw);
		uint8_t tag = FIELD_GET(MAXM86161_FIFO_TAG_MASK, fifo_raw);

		if (tag >= MAXM86161_FIFO_TAG_LEDC1 && tag <= MAXM86161_FIFO_TAG_LEDC6) {
			ddata->led_state.raw[tag - MAXM86161_FIFO_TAG_LEDC1] = fifo_data;
		} else if (tag >= MAXM86161_FIFO_TAG_LEDC1_PF &&
			   tag <= MAXM86161_FIFO_TAG_LEDC3_PF) {
			ddata->led_state.raw[tag - MAXM86161_FIFO_TAG_LEDC1_PF] = fifo_data;
		} else if (ddata->prox_attr.enabled && tag == MAXM86161_FIFO_TAG_PROX) {
			/* PROX data is a standalone measurement at 0 */
			ddata->led_state.raw[0] = fifo_data;
		}
	}

	return 0;
}

static int maxm86161_sample_fetch(const struct device *dev, enum sensor_channel channel)
{
	struct maxm86161_data *ddata = dev->data;
	int8_t pos = 0;

	switch (channel) {
	case SENSOR_CHAN_ALL: {
		int ret;

		ret = maxm86161_fetch_die_temp(dev);
		if (ret) {
			return ret;
		}
		break;
	}
	case SENSOR_CHAN_DIE_TEMP:
		return maxm86161_fetch_die_temp(dev);
	case SENSOR_CHAN_GREEN:
		pos = ddata->led_state.chan_pos[MAXM86161_EXPOSURE_LED1_GREEN];
		break;
	case SENSOR_CHAN_IR:
		pos = ddata->led_state.chan_pos[MAXM86161_EXPOSURE_LED2_IR];
		break;
	case SENSOR_CHAN_RED:
		pos = ddata->led_state.chan_pos[MAXM86161_EXPOSURE_LED3_RED];
		break;
	case SENSOR_CHAN_AMBIENT_LIGHT:
		pos = ddata->led_state.chan_pos[MAXM86161_EXPOSURE_AMBIENT_LIGHT];
		break;
	case SENSOR_CHAN_PROX:
		pos = ddata->led_state.chan_pos[MAXM86161_EXPOSURE_PILOT_ON_GREEN];
		break;
	default:
		LOG_ERR("Channel is not supported to be fetched");
		return -ENOTSUP;
	}

	if (pos < 0) {
		LOG_ERR("Channel %d is not set to be measured", channel);
		return -ENODATA;
	}

	return maxm86161_fetch_fifo(dev);
}

static int maxm86161_channel_get(const struct device *dev, enum sensor_channel channel,
				 struct sensor_value *val)
{
	struct maxm86161_data *ddata = dev->data;
	int8_t pos = 0;

	switch (channel) {
	case SENSOR_CHAN_DIE_TEMP:
		*val = ddata->temp_val;
		return 0;
	case SENSOR_CHAN_GREEN:
		pos = ddata->led_state.chan_pos[MAXM86161_EXPOSURE_LED1_GREEN];
		break;
	case SENSOR_CHAN_IR:
		pos = ddata->led_state.chan_pos[MAXM86161_EXPOSURE_LED2_IR];
		break;
	case SENSOR_CHAN_RED:
		pos = ddata->led_state.chan_pos[MAXM86161_EXPOSURE_LED3_RED];
		break;
	case SENSOR_CHAN_AMBIENT_LIGHT:
		pos = ddata->led_state.chan_pos[MAXM86161_EXPOSURE_AMBIENT_LIGHT];
		break;
	case SENSOR_CHAN_PROX:
		pos = ddata->led_state.chan_pos[MAXM86161_EXPOSURE_PILOT_ON_GREEN];
		break;
	default:
		LOG_ERR("Channel is not supported to be retrieved");
		return -ENOTSUP;
	}

	if (pos < 0) {
		LOG_ERR("Channel %d is not set to be measured", channel);
		return -ENODATA;
	}

	val->val1 = (int32_t)ddata->led_state.raw[pos];
	val->val2 = 0;

	return 0;
}

static inline int maxm86161_init_odr(const struct device *dev, uint16_t *odr)
{
	const struct maxm86161_config *config = dev->config;
	uint8_t samples_per_sec = config->ppg_cfg.ppg_cfg2.ppg_sr;
	uint8_t averaging = config->ppg_cfg.ppg_cfg2.smp_avg;

	*odr = (uint32_t)sample_rate[samples_per_sec] / sample_avg[averaging];

	return 0;
}

static int maxm86161_config_fifo(const struct device *dev)
{
	const struct maxm86161_config *dcfg = dev->config;
	uint8_t watermark = maxm86161_watermark_to_afull(dcfg->fifo_cfg.watermark);
	uint8_t reg_val;
	int ret;

	reg_val = FIELD_PREP(MAXM86161_MSK_FIFO_A_FULL, watermark);

	ret = maxm86161_i2c_write_byte(dev, MAXM86161_REG_FIFO_CONFIG1, reg_val);
	if (ret) {
		LOG_ERR("Error configuring FIFO Watermark register: %d", ret);
		return ret;
	}

	reg_val = FIELD_PREP(MAXM86161_MSK_FIFO_A_FULL_TYPE, dcfg->fifo_cfg.a_full_type) |
		  FIELD_PREP(MAXM86161_MSK_FIFO_ROLLOVER, dcfg->fifo_cfg.rollover);

	ret = maxm86161_i2c_write_byte(dev, MAXM86161_REG_FIFO_CONFIG2, reg_val);
	if (ret) {
		LOG_ERR("Error configuring FIFO Config2 register: %d", ret);
		return ret;
	}

	return 0;
}

static int maxm86161_config_ppg_sync(const struct device *dev)
{
	const struct maxm86161_config *dcfg = dev->config;
	uint8_t reg_val;
	int ret;

	reg_val = FIELD_PREP(MAXM86161_MSK_PPG_SYNC_CONTROL_GPIO_CTRL,
			     dcfg->ppg_cfg.sync_ctrl.gpio_ctrl) |
		  FIELD_PREP(MAXM86161_MSK_PPG_SYNC_CONTROL_DAC_CODE_CHG_TAG,
			     dcfg->ppg_cfg.sync_ctrl.dac_code_chg_tag) |
		  FIELD_PREP(MAXM86161_MSK_PPG_SYNC_CONTROL_TIME_STAMP_EN,
			     dcfg->ppg_cfg.sync_ctrl.time_stamp_en);

	ret = maxm86161_i2c_write_byte(dev, MAXM86161_REG_PPG_SYNC_CONTROL, reg_val);
	if (ret) {
		LOG_ERR("Error configuring PPG Sync Control register: %d", ret);
		return ret;
	}

	return 0;
}

static int maxm86161_config_ppg(const struct device *dev)
{
	const struct maxm86161_config *dcfg = dev->config;
	uint8_t reg_val;
	int ret;

	reg_val = FIELD_PREP(MAXM86161_MSK_PPG_CONFIG1_PPG_TINT, dcfg->ppg_cfg.ppg_cfg1.ppg_tint) |
		  FIELD_PREP(MAXM86161_MSK_PPG_CONFIG1_PPG1_ADC_RGE,
			     dcfg->ppg_cfg.ppg_cfg1.ppg1_adc_rge) |
		  FIELD_PREP(MAXM86161_MSK_PPG_CONFIG1_ADD_OFFSET,
			     dcfg->ppg_cfg.ppg_cfg1.add_offset) |
		  FIELD_PREP(MAXM86161_MSK_PPG_CONFIG1_ALC_DISABLE,
			     dcfg->ppg_cfg.ppg_cfg1.alc_disable);

	ret = maxm86161_i2c_write_byte(dev, MAXM86161_REG_PPG_CONFIG1, reg_val);
	if (ret) {
		LOG_ERR("Error configuring PPG Config1 register: %d", ret);
		return ret;
	}

	reg_val = FIELD_PREP(MAXM86161_MSK_PPG_CONFIG2_SMP_AVE, dcfg->ppg_cfg.ppg_cfg2.smp_avg) |
		  FIELD_PREP(MAXM86161_MSK_PPG_CONFIG2_PPG_SR, dcfg->ppg_cfg.ppg_cfg2.ppg_sr);

	ret = maxm86161_i2c_write_byte(dev, MAXM86161_REG_PPG_CONFIG2, reg_val);
	if (ret) {
		LOG_ERR("Error configuring PPG Config2 register: %d", ret);
		return ret;
	}

	reg_val = FIELD_PREP(MAXM86161_MSK_PPG_CONFIG3_BURST_EN, dcfg->ppg_cfg.ppg_cfg3.burst_en) |
		  FIELD_PREP(MAXM86161_MSK_PPG_CONFIG3_BURST_RATE,
			     dcfg->ppg_cfg.ppg_cfg3.burst_rate) |
		  FIELD_PREP(MAXM86161_MSK_PPG_CONFIG3_DIG_FILT_SEL,
			     dcfg->ppg_cfg.ppg_cfg3.dig_filt_sel) |
		  FIELD_PREP(MAXM86161_MSK_PPG_CONFIG3_LED_SETLNG,
			     dcfg->ppg_cfg.ppg_cfg3.led_setlng);

	ret = maxm86161_i2c_write_byte(dev, MAXM86161_REG_PPG_CONFIG3, reg_val);
	if (ret) {
		LOG_ERR("Error configuring PPG Config3 register: %d", ret);
		return ret;
	}

	reg_val = FIELD_PREP(MAXM86161_MSK_PPG_PROX_INT_THRESH, dcfg->ppg_cfg.prox_int_thresh);

	ret = maxm86161_i2c_write_byte(dev, MAXM86161_REG_PPG_PROX_INT_THRESH, reg_val);
	if (ret) {
		LOG_ERR("Error configuring PPG Proximity Threshold register: %d", ret);
		return ret;
	}

	reg_val = FIELD_PREP(MAXM86161_MSK_PPG_PD_BIAS, dcfg->ppg_cfg.pd_bias);

	ret = maxm86161_i2c_write_byte(dev, MAXM86161_REG_PPG_PD_BIAS, reg_val);
	if (ret) {
		LOG_ERR("Error configuring PPG PD Bias register: %d", ret);
		return ret;
	}

	return 0;
}

static int maxm86161_config_picket_fence(const struct device *dev)
{
	const struct maxm86161_config *dcfg = dev->config;
	uint8_t reg_val;
	int ret;

	reg_val = FIELD_PREP(MAXM86161_MSK_PPG_PICKET_FENCE_THRESH_SIGMA,
			     dcfg->pf_cfg.thresh_sigma) |
		  FIELD_PREP(MAXM86161_MSK_PPG_PICKET_FENCE_IIR_INIT_VAL,
			     dcfg->pf_cfg.iir_init_val) |
		  FIELD_PREP(MAXM86161_MSK_PPG_PICKET_FENCE_IIR_TC, dcfg->pf_cfg.iir_tc) |
		  FIELD_PREP(MAXM86161_MSK_PPG_PICKET_FENCE_PF_ORDER, dcfg->pf_cfg.order) |
		  FIELD_PREP(MAXM86161_MSK_PPG_PICKET_FENCE_PF_ENABLE, dcfg->pf_cfg.enable);

	ret = maxm86161_i2c_write_byte(dev, MAXM86161_REG_PPG_PICKET_FENCE, reg_val);
	if (ret) {
		LOG_ERR("Error configuring Picket Fence register: %d", ret);
		return ret;
	}

	return 0;
}

static int maxm86161_config_sys(const struct device *dev)
{
	const struct maxm86161_config *dcfg = dev->config;
	int ret;

	ret = maxm86161_i2c_update_byte(dev, MAXM86161_REG_SYSTEM_CONTROL,
					MAXM86161_MSK_SYSTEM_CONTROL_LP_MODE,
					dcfg->sys_cfg.low_power_mode);
	if (ret) {
		LOG_ERR("Error configuring System Control low power mode: %d", ret);
		return ret;
	}

	ret = maxm86161_i2c_update_byte(dev, MAXM86161_REG_SYSTEM_CONTROL,
					MAXM86161_MSK_SYSTEM_CONTROL_SINGLE_PPG,
					dcfg->sys_cfg.single_ppg);
	if (ret) {
		LOG_ERR("Error configuring System Control single PPG mode: %d", ret);
		return ret;
	}

	return 0;
}

static int maxm86161_config_led(const struct device *dev)
{
	const struct maxm86161_config *dcfg = dev->config;
	uint8_t reg_val;
	int ret;

	reg_val = FIELD_PREP(MAXM86161_MSK_LED_SEQ_ODD, dcfg->led_cfg.led_seq[0]) |
		  FIELD_PREP(MAXM86161_MSK_LED_SEQ_EVEN, dcfg->led_cfg.led_seq[1]);

	ret = maxm86161_i2c_write_byte(dev, MAXM86161_REG_LED_SEQ_REG1, reg_val);
	if (ret) {
		LOG_ERR("Error configuring LED Sequence register 1: %d", ret);
		return ret;
	}

	reg_val = FIELD_PREP(MAXM86161_MSK_LED_SEQ_ODD, dcfg->led_cfg.led_seq[2]) |
		  FIELD_PREP(MAXM86161_MSK_LED_SEQ_EVEN, dcfg->led_cfg.led_seq[3]);

	ret = maxm86161_i2c_write_byte(dev, MAXM86161_REG_LED_SEQ_REG2, reg_val);
	if (ret) {
		LOG_ERR("Error configuring LED Sequence register 2: %d", ret);
		return ret;
	}

	reg_val = FIELD_PREP(MAXM86161_MSK_LED_SEQ_ODD, dcfg->led_cfg.led_seq[4]) |
		  FIELD_PREP(MAXM86161_MSK_LED_SEQ_EVEN, dcfg->led_cfg.led_seq[5]);

	ret = maxm86161_i2c_write_byte(dev, MAXM86161_REG_LED_SEQ_REG3, reg_val);
	if (ret) {
		LOG_ERR("Error configuring LED Sequence register 3: %d", ret);
		return ret;
	}

	reg_val = FIELD_PREP(MAXM86161_MSK_LED_PA, dcfg->led_cfg.led_pa_cfg.led1_pa);

	ret = maxm86161_i2c_write_byte(dev, MAXM86161_REG_LED1_PA, reg_val);
	if (ret) {
		LOG_ERR("Error configuring LED1 pulse amplitude: %d", ret);
		return ret;
	}

	reg_val = FIELD_PREP(MAXM86161_MSK_LED_PA, dcfg->led_cfg.led_pa_cfg.led2_pa);

	ret = maxm86161_i2c_write_byte(dev, MAXM86161_REG_LED2_PA, reg_val);
	if (ret) {
		LOG_ERR("Error configuring LED2 pulse amplitude: %d", ret);
		return ret;
	}

	reg_val = FIELD_PREP(MAXM86161_MSK_LED_PA, dcfg->led_cfg.led_pa_cfg.led3_pa);

	ret = maxm86161_i2c_write_byte(dev, MAXM86161_REG_LED3_PA, reg_val);
	if (ret) {
		LOG_ERR("Error configuring LED3 pulse amplitude: %d", ret);
		return ret;
	}

	reg_val = FIELD_PREP(MAXM86161_MSK_LED_PA, dcfg->led_cfg.led_pa_cfg.led_pilot_pa);

	ret = maxm86161_i2c_write_byte(dev, MAXM86161_REG_LED_PILOT_PA, reg_val);
	if (ret) {
		LOG_ERR("Error configuring LED pilot pulse amplitude: %d", ret);
		return ret;
	}

	reg_val = FIELD_PREP(MAXM86161_MSK_LED_RANGE1_LED1_RGE,
			     dcfg->led_cfg.led_rge_cfg.led1_rge) |
		  FIELD_PREP(MAXM86161_MSK_LED_RANGE1_LED2_RGE,
			     dcfg->led_cfg.led_rge_cfg.led2_rge) |
		  FIELD_PREP(MAXM86161_MSK_LED_RANGE1_LED3_RGE,
			     dcfg->led_cfg.led_rge_cfg.led3_rge);

	ret = maxm86161_i2c_write_byte(dev, MAXM86161_REG_LED_RANGE_1, reg_val);
	if (ret) {
		LOG_ERR("Error configuring LED Range register: %d", ret);
		return ret;
	}

	return 0;
}

static int maxm86161_probe(const struct device *dev)
{
	struct maxm86161_data *data = dev->data;
	int ret;

	/* Initialize output datarate for streaming timestamps */
	maxm86161_init_odr(dev, &data->odr);

	/* Build the FIFO data map from the configured LED sequence */
	maxm86161_init_led_state(dev);

	/* Disable device */
	ret = maxm86161_i2c_update_byte(dev, MAXM86161_REG_SYSTEM_CONTROL,
					MAXM86161_MSK_SYSTEM_CONTROL_SHDN, true);
	if (ret) {
		LOG_ERR("Error shutting down device: %d", ret);
		return ret;
	}

	ret = maxm86161_config_fifo(dev);
	if (ret) {
		return ret;
	}

	ret = maxm86161_config_ppg_sync(dev);
	if (ret) {
		return ret;
	}

	ret = maxm86161_config_ppg(dev);
	if (ret) {
		return ret;
	}

	ret = maxm86161_config_picket_fence(dev);
	if (ret) {
		return ret;
	}

	ret = maxm86161_config_sys(dev);
	if (ret) {
		return ret;
	}

	ret = maxm86161_config_led(dev);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_MAXM86161_TRIGGER
	ret = maxm86161_init_interrupt(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize interrupt: %d", ret);
		return ret;
	}
#endif /* CONFIG_MAXM86161_TRIGGER */

#ifdef CONFIG_MAXM86161_STREAM
	k_mutex_init(&data->trigger_mutex);
#endif /* CONFIG_MAXM86161_STREAM */

	/* Re-enable device */
	ret = maxm86161_i2c_update_byte(dev, MAXM86161_REG_SYSTEM_CONTROL,
					MAXM86161_MSK_SYSTEM_CONTROL_SHDN, false);
	if (ret) {
		LOG_ERR("Error shutting down device: %d", ret);
		return ret;
	}

	return 0;
}

static int maxm86161_init(const struct device *dev)
{
	const struct maxm86161_config *dcfg = dev->config;
	uint8_t reg_val;
	int ret;

	if (!i2c_is_ready_dt(&dcfg->i2c)) {
		LOG_ERR("I2C Bus is not ready");
		return -ENODEV;
	}

	ret = maxm86161_i2c_read_byte(dev, MAXM86161_REG_PART_ID, &reg_val);
	if (ret) {
		LOG_ERR("Error reading part ID: %d", ret);
		return ret;
	}

	if (reg_val != MAXM86161_PART_ID_VAL) {
		LOG_ERR("Unexpected part ID: 0x%02X (expected 0x%02X)", reg_val,
			MAXM86161_PART_ID_VAL);
		return -ENODEV;
	}

	LOG_INF("MAXM86161 Part ID: 0x%02X", reg_val);

	ret = maxm86161_probe(dev);
	if (ret) {
		return ret;
	}

	return 0;
}

static DEVICE_API(sensor, maxm86161_driver_api) = {
	.attr_set = maxm86161_attr_set,
	.attr_get = maxm86161_attr_get,
	.sample_fetch = maxm86161_sample_fetch,
	.channel_get = maxm86161_channel_get,
#ifdef CONFIG_MAXM86161_TRIGGER
	.trigger_set = maxm86161_trigger_set,
#endif /* CONFIG_MAXM86161_TRIGGER */
#ifdef CONFIG_MAXM86161_STREAM
	.submit = maxm86161_submit,
	.get_decoder = maxm86161_get_decoder,
#endif
};

#define MAXM86161_RTIO_DEFINE(inst)			\
	I2C_DT_IODEV_DEFINE(maxm86161_iodev_##inst,	\
			    DT_DRV_INST(inst));		\
	RTIO_DEFINE(maxm86161_rtio_ctx_##inst, 128, 128);

#define MAXM86161_FIFO_DEFINE(inst)					\
	{								\
	    .watermark = DT_INST_PROP(inst, fifo_watermark),		\
	    .a_full_type = DT_INST_PROP(inst, fifo_almost_full_type),	\
	    .rollover = DT_INST_PROP(inst, fifo_rollover_mode),		\
	}

#define MAXM86161_PPG_SYNC_DEFINE(inst)							\
	{										\
	    .gpio_ctrl = DT_INST_PROP(inst, ppg_sync_gpio_function),			\
	    .dac_code_chg_tag = DT_INST_PROP(inst, ppg_dac_code_change_tag_enable),	\
	    .time_stamp_en = DT_INST_PROP(inst, ppg_timestamp_enable),			\
	}

#define MAXM86161_PPG_CFG1_DEFINE(inst)							\
	{										\
	    .ppg_tint = DT_INST_PROP(inst, ppg_time_integration),			\
	    .ppg1_adc_rge = DT_INST_PROP(inst, ppg_adc_range),				\
	    .add_offset = DT_INST_PROP(inst, ppg_add_offset_enable),			\
	    .alc_disable = DT_INST_PROP(inst, ppg_ambient_light_cancellation_disable),	\
	}

#define MAXM86161_PPG_CFG2_DEFINE(inst)				\
	{							\
	    .smp_avg = DT_INST_PROP(inst, ppg_sample_averaging),\
	    .ppg_sr = DT_INST_PROP(inst, ppg_sample_rate),	\
	}

#define MAXM86161_PPG_CFG3_DEFINE(inst)					\
	{								\
	    .burst_en = DT_INST_PROP(inst, burst_rate_enable),		\
	    .burst_rate = DT_INST_PROP(inst, burst_rate),		\
	    .dig_filt_sel = DT_INST_PROP(inst, digital_filter_select),	\
	    .led_setlng = DT_INST_PROP(inst, led_settling_time),	\
	}

#define MAXM86161_PPG_DEFINE(inst)							\
	{										\
	    .sync_ctrl = MAXM86161_PPG_SYNC_DEFINE(inst),				\
	    .ppg_cfg1 = MAXM86161_PPG_CFG1_DEFINE(inst),				\
	    .ppg_cfg2 = MAXM86161_PPG_CFG2_DEFINE(inst),				\
	    .ppg_cfg3 = MAXM86161_PPG_CFG3_DEFINE(inst),				\
	    .prox_int_thresh = DT_INST_PROP(inst, proximity_interrupt_threshold),	\
	    .pd_bias = DT_INST_PROP(inst, photodiode_bias_cap),				\
	}

#define MAXM86161_PICKET_FENCE_DEFINE(inst)				\
	{								\
	    .thresh_sigma = DT_INST_PROP(inst, pf_threshold_sigma),	\
	    .iir_init_val = DT_INST_PROP(inst, pf_iir_init_value),	\
	    .iir_tc = DT_INST_PROP(inst, pf_iir_time_constant),		\
	    .order = DT_INST_PROP(inst, pf_order),			\
	    .enable = DT_INST_PROP(inst, pf_enable),			\
	}

#define MAXM86161_SYS_DEFINE(inst)					\
	{								\
	    .low_power_mode = DT_INST_PROP(inst, low_power_mode_enable),\
	    .single_ppg = true,						\
	}

#define MAXM86161_LED_SEQ_SLOT(idx, inst) \
	DT_INST_PROP(inst, UTIL_CAT(led_sequence_slot, UTIL_INC(idx)))

#define MAXM86161_LED_SEQ_DEFINE(inst)						\
	{									\
	    LISTIFY(MAXM86161_LED_SEQ_COUNT, MAXM86161_LED_SEQ_SLOT, (,), inst)	\
	}

#define MAXM86161_LED_RGE_DEFINE(inst)				\
	{							\
		.led1_rge = DT_INST_PROP(inst, led1_range),	\
		.led2_rge = DT_INST_PROP(inst, led2_range),	\
		.led3_rge = DT_INST_PROP(inst, led3_range),	\
	}

#define MAXM86161_LED_PA_DEFINE(inst)						\
	{									\
		.led1_pa = DT_INST_PROP(inst, led1_pulse_amplitude),		\
		.led2_pa = DT_INST_PROP(inst, led2_pulse_amplitude),		\
		.led3_pa = DT_INST_PROP(inst, led3_pulse_amplitude),		\
		.led_pilot_pa = DT_INST_PROP(inst, led_pilot_pulse_amplitude),	\
	}

#define MAXM86161_LED_DEFINE(inst)				\
	{							\
		.led_pa_cfg = MAXM86161_LED_PA_DEFINE(inst),	\
		.led_seq = MAXM86161_LED_SEQ_DEFINE(inst),	\
		.led_rge_cfg = MAXM86161_LED_RGE_DEFINE(inst),	\
	}

#define MAXM86161_DEFINE(inst)									\
	IF_ENABLED(CONFIG_MAXM86161_STREAM,							\
		   (MAXM86161_RTIO_DEFINE(inst)));						\
	static struct maxm86161_data maxm86161_data_##inst = {					\
	    IF_ENABLED(CONFIG_MAXM86161_STREAM, (						\
							.rtio_ctx = &maxm86161_rtio_ctx_##inst,	\
							.iodev = &maxm86161_iodev_##inst,))	\
		.led_state = {									\
		.chan_pos = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1},				\
	    }};											\
	static const struct maxm86161_config							\
	    maxm86161_config_##inst = {								\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		.fifo_cfg = MAXM86161_FIFO_DEFINE(inst),					\
		.ppg_cfg = MAXM86161_PPG_DEFINE(inst),						\
		.pf_cfg = MAXM86161_PICKET_FENCE_DEFINE(inst),					\
		.led_cfg = MAXM86161_LED_DEFINE(inst),						\
		.sys_cfg = MAXM86161_SYS_DEFINE(inst),						\
												\
		IF_ENABLED(CONFIG_MAXM86161_TRIGGER, (						\
		.interrupt_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, interrupt_gpios, {0})))};	\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &maxm86161_init, NULL,				\
				     &maxm86161_data_##inst,					\
				     &maxm86161_config_##inst, POST_KERNEL,			\
				     CONFIG_SENSOR_INIT_PRIORITY, &maxm86161_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAXM86161_DEFINE)
