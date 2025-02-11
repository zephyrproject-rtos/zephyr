/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Würth Elektronic WSEN-ITDS 3-axis accel sensor driver
 *
 * Copyright (c) 2020 Linumiz
 * Author: Saravanan Sekar <saravanan@linumiz.com>
 */

#include <zephyr/init.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include "itds.h"

#define DT_DRV_COMPAT we_wsen_itds
#define ITDS_TEMP_CONST 62500

LOG_MODULE_REGISTER(ITDS, CONFIG_SENSOR_LOG_LEVEL);

static const struct itds_odr itds_odr_map[ITDS_ODR_MAX] = {
	{0}, {1, 600}, {12, 500}, {25}, {50}, {100}, {200},
	{400}, {800}, {1600}
};

static const int16_t itds_sensitivity_scale[][ITDS_ACCL_RANGE_END] = {
	{976, 1952, 3904, 7808},

	/* high performance mode */
	{244, 488, 976, 1952}
};

static int itds_get_odr_for_index(const struct device *dev,
				  enum itds_odr_const idx,
				  uint16_t *freq, uint16_t *mfreq)
{
	struct itds_device_data *ddata = dev->data;
	int start, end;
	bool hp_mode;

	hp_mode = !!(ddata->op_mode & ITDS_OP_MODE_HIGH_PERF);
	if (hp_mode) {
		start = ITDS_ODR_12_5;
		end = ITDS_ODR_1600;
	} else {
		start = ITDS_ODR_1_6;
		end = ITDS_ODR_200;
	}

	if (idx < start || idx > end) {
		LOG_ERR("invalid odr for the operating mode");
		return -EINVAL;
	}

	*freq = itds_odr_map[idx].freq;
	*mfreq = itds_odr_map[idx].mfreq;

	return 0;
}

static int itds_accl_odr_set(const struct device *dev, uint16_t freq,
			     uint16_t mfreq)
{
	struct itds_device_data *ddata = dev->data;
	const struct itds_device_config *cfg = dev->config;
	int start, end, i;
	bool hp_mode;

	hp_mode = !!(ddata->op_mode & ITDS_OP_MODE_HIGH_PERF);
	if (hp_mode) {
		start = ITDS_ODR_12_5;
		end = ITDS_ODR_1600;
	} else {
		start = ITDS_ODR_1_6;
		end = ITDS_ODR_200;
	}

	for (i = start; i <= end; i++) {
		if ((freq == itds_odr_map[i].freq) &&
		    (mfreq == itds_odr_map[i].mfreq)) {

			return i2c_reg_update_byte_dt(&cfg->i2c,
					 ITDS_REG_CTRL1, ITDS_MASK_ODR, i << 4);
		}
	}

	LOG_ERR("invalid odr, not in range");
	return -EINVAL;
}

static int itds_accl_range_set(const struct device *dev, int32_t range)
{
	struct itds_device_data *ddata = dev->data;
	const struct itds_device_config *cfg = dev->config;
	int i, ret;
	bool hp_mode;

	for (i = 0; i < ITDS_ACCL_RANGE_END; i++) {
		if (range <= (2 << i)) {
			break;
		}
	}

	if (i == ITDS_ACCL_RANGE_END) {
		LOG_ERR("Accl out of range");
		return -EINVAL;
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c, ITDS_REG_CTRL6,
				  ITDS_MASK_SCALE, i << 4);
	if (ret) {
		LOG_ERR("Accl set full scale failed %d", ret);
		return ret;
	}

	hp_mode = !!(ddata->op_mode & ITDS_OP_MODE_HIGH_PERF);
	ddata->scale = itds_sensitivity_scale[hp_mode][i];

	return 0;
}

static int itds_attr_set(const struct device *dev, enum sensor_channel chan,
			 enum sensor_attribute attr,
			 const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ACCEL_X &&
	    chan != SENSOR_CHAN_ACCEL_Y &&
	    chan != SENSOR_CHAN_ACCEL_Z &&
	    chan != SENSOR_CHAN_ACCEL_XYZ) {
		LOG_ERR("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return itds_accl_range_set(dev, sensor_ms2_to_g(val));

	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return itds_accl_odr_set(dev, val->val1, val->val2 / 1000);

	default:
		LOG_ERR("Accel attribute not supported.");
		return -ENOTSUP;
	}
}

static int itds_fetch_temperature(struct itds_device_data *ddata,
				 const struct itds_device_config *cfg)
{
	uint8_t rval;
	int16_t temp_raw = 0;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c,
				ITDS_REG_STATUS_DETECT, &rval);
	if (ret) {
		return ret;
	}

	if (!(rval & ITDS_EVENT_DRDY_T)) {
		return -EAGAIN;
	}

	ret = i2c_burst_read_dt(&cfg->i2c, ITDS_REG_TEMP_L,
			     (uint8_t *)&temp_raw, sizeof(uint16_t));
	if (ret) {
		return ret;
	}

	ddata->temperature = sys_le16_to_cpu(temp_raw);

	return 0;
}

static int itds_fetch_accel(struct itds_device_data *ddata,
			    const struct itds_device_config *cfg)
{
	size_t i, ret;
	uint8_t rval;

	ret = i2c_reg_read_byte_dt(&cfg->i2c,
				ITDS_REG_STATUS, &rval);
	if (ret) {
		return ret;
	}

	if (!(rval & ITDS_EVENT_DRDY)) {
		return -EAGAIN;
	}

	ret = i2c_burst_read_dt(&cfg->i2c, ITDS_REG_X_OUT_L,
			     (uint8_t *)ddata->samples,
			     sizeof(uint16_t) * ITDS_SAMPLE_SIZE);
	if (ret) {
		return ret;
	}

	/* convert samples to cpu endianness */
	for (i = 0; i < ITDS_SAMPLE_SIZE; i += 2) {
		int16_t *sample =	(int16_t *) &ddata->samples[i];

		*sample = sys_le16_to_cpu(*sample);
		if (ddata->op_mode & ITDS_OP_MODE_NORMAL ||
		    ddata->op_mode & ITDS_OP_MODE_HIGH_PERF) {
			*sample = *sample >> 2;
		} else {
			*sample = *sample >> 4;
		}
		LOG_DBG("itds sample %d %X\n", i, *sample);
	}

	return 0;
}

static int itds_sample_fetch(const struct device *dev,
			     enum sensor_channel chan)
{
	struct itds_device_data *ddata = dev->data;
	const struct itds_device_config *cfg = dev->config;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		return itds_fetch_accel(ddata, cfg);

	case SENSOR_CHAN_DIE_TEMP:
		return itds_fetch_temperature(ddata, cfg);

	case SENSOR_CHAN_ALL:
		return itds_fetch_accel(ddata, cfg) ||
		       itds_fetch_temperature(ddata, cfg);

	default:
		return -EINVAL;
	}
}

static inline void itds_accl_channel_get(const struct device *dev,
					 enum sensor_channel chan,
					 struct sensor_value *val)
{
	int i;
	struct itds_device_data *ddata = dev->data;
	uint8_t ofs_start, ofs_stop;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		ofs_start = ofs_stop = 0U;
		break;
	case SENSOR_CHAN_ACCEL_Y:
		ofs_start = ofs_stop = 1U;
		break;
	case SENSOR_CHAN_ACCEL_Z:
		ofs_start = ofs_stop = 2U;
		break;
	default:
		ofs_start = 0U; ofs_stop = 2U;
		break;
	}

	for (i = ofs_start; i <= ofs_stop ; i++, val++) {
		int64_t dval;

		/* Sensitivity is exposed in ug/LSB */
		/* Convert to m/s^2 */
		dval = (int64_t)((ddata->samples[i] * ddata->scale * SENSOR_G) /
				1000000LL);
		val->val1 = (int32_t)(dval / 1000000);
		val->val2 = (int32_t)(dval % 1000000);
	}
}

static int itds_temp_channel_get(const struct device *dev,
				 struct sensor_value *val)
{
	int32_t temp_processed;
	struct itds_device_data *ddata = dev->data;

	temp_processed = (ddata->temperature >> 4) * ITDS_TEMP_CONST;

	val->val1 = ITDS_TEMP_OFFSET;
	val->val2 = temp_processed;

	return 0;
}

static int itds_channel_get(const struct device *dev,
			    enum sensor_channel chan,
			    struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		itds_accl_channel_get(dev, chan, val);
		return 0;

	case SENSOR_CHAN_DIE_TEMP:
		return itds_temp_channel_get(dev, val);

	default:
		LOG_ERR("Channel not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int itds_init(const struct device *dev)
{
	struct itds_device_data *ddata = dev->data;
	const struct itds_device_config *cfg = dev->config;
	int ret;
	uint16_t freq, mfreq;
	uint8_t rval;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c,
				ITDS_REG_DEV_ID, &rval);
	if (ret) {
		LOG_ERR("device init fail: %d", ret);
		return ret;
	}

	if (rval != ITDS_DEVICE_ID) {
		LOG_ERR("device ID mismatch: %x", rval);
		return -EIO;
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c, ITDS_REG_CTRL2,
				  ITDS_MASK_BDU_INC_ADD, ITDS_MASK_BDU_INC_ADD);
	if (ret) {
		LOG_ERR("unable to set block data update %d", ret);
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, ITDS_REG_WAKEUP_EVENT, 0);
	if (ret) {
		LOG_ERR("disable wakeup event fail %d", ret);
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c, ITDS_REG_CTRL1,
				     ITDS_MASK_MODE, 1 << cfg->def_op_mode);
	if (ret) {
		LOG_ERR("set operating mode fail %d", ret);
		return ret;
	}

	ddata->op_mode = 1 << cfg->def_op_mode;

	ret = itds_get_odr_for_index(dev, cfg->def_odr, &freq, &mfreq);
	if (ret) {
		LOG_ERR("odr not in range for operating mode %d", ret);
		return ret;
	}

	ret = itds_accl_odr_set(dev, freq, mfreq);
	if (ret) {
		LOG_ERR("odr not in range for operating mode %d", ret);
		return ret;
	}

#ifdef CONFIG_ITDS_TRIGGER
	ret = itds_trigger_mode_init(dev);
	if (ret) {
		LOG_ERR("trigger mode init failed %d", ret);
		return ret;
	}
#endif
	return 0;
}

static const struct sensor_driver_api itds_api = {
	.attr_set = itds_attr_set,
#ifdef CONFIG_ITDS_TRIGGER
	.trigger_set = itds_trigger_set,
#endif
	.sample_fetch = itds_sample_fetch,
	.channel_get = itds_channel_get,
};

#ifdef CONFIG_ITDS_TRIGGER
#define WSEN_ITDS_CFG_IRQ(inst)						\
	.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, { 0 }),
#else
#define WSEN_ITDS_CFG_IRQ(inst)
#endif

#define WSEN_ITDS_INIT(idx)						\
									\
static struct itds_device_data itds_data_##idx;				\
									\
static const struct itds_device_config itds_config_##idx = {		\
	.i2c = I2C_DT_SPEC_INST_GET(idx),				\
	.def_odr = DT_INST_ENUM_IDX(idx, odr),				\
	.def_op_mode = DT_INST_ENUM_IDX(idx, op_mode),			\
	WSEN_ITDS_CFG_IRQ(idx)						\
};									\
									\
SENSOR_DEVICE_DT_INST_DEFINE(idx, itds_init, NULL,			\
		    &itds_data_##idx, &itds_config_##idx,		\
		    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,		\
		    &itds_api);						\

DT_INST_FOREACH_STATUS_OKAY(WSEN_ITDS_INIT)
