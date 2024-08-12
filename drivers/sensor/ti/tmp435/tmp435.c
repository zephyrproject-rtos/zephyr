/*
 * Copyright (c) 2024 Bittium Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp435

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "tmp435.h"

LOG_MODULE_REGISTER(TMP435, CONFIG_SENSOR_LOG_LEVEL);

static inline int tmp435_reg_read(const struct tmp435_config *cfg, uint8_t reg, uint8_t *buf,
				  uint32_t size)
{
	return i2c_burst_read_dt(&cfg->i2c, reg, buf, size);
}

static inline int tmp435_reg_write(const struct tmp435_config *cfg, uint8_t reg, uint8_t *buf,
				   uint32_t size)
{
	return i2c_burst_write_dt(&cfg->i2c, reg, buf, size);
}

static inline int tmp435_get_status(const struct tmp435_config *cfg, uint8_t *status)
{
	return tmp435_reg_read(cfg, TMP435_STATUS_REG, status, 1);
}

static int tmp435_one_shot(const struct device *dev)
{
	uint8_t data = 0;
	uint8_t status = 0;
	int ret = 0;
	const struct tmp435_config *cfg = dev->config;

	data = 1; /* write anything to start */
	ret = tmp435_reg_write(cfg, TMP435_ONE_SHOT_START_REG, &data, 1);
	for (uint16_t i = 0; i < TMP435_CONV_LOOP_LIMIT; i++) {
		ret = tmp435_get_status(cfg, &status);
		if (ret < 0) {
			LOG_DBG("Failed to read TMP435_STATUS_REG, ret:%d", ret);
		} else {
			if (status & TMP435_STATUS_REG_BUSY) {
				/* conversion not ready */
				k_msleep(10);
			} else {
				LOG_DBG("conv over, loops:%d status:%x", i, status);
				break;
			}
		}
	}
	return ret;
}

static int tmp435_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int ret = 0;
	uint8_t value = 0;
	int32_t temp = 0;
	const struct tmp435_config *cfg = dev->config;
	struct tmp435_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP &&
	    chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}
	tmp435_one_shot(dev); /* start conversion */
	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_DIE_TEMP)) {
		ret = tmp435_reg_read(cfg, TMP435_LOCAL_TEMP_H_REG, &value, sizeof(value));
		if (ret < 0) {
			LOG_ERR("Failed to read TMP435_LOCAL_TEMP_H_REG, ret:%d", ret);
			return ret;
		}
		temp = value;
		ret = tmp435_reg_read(cfg, TMP435_LOCAL_TEMP_L_REG, &value, sizeof(value));
		if (ret < 0) {
			LOG_ERR("Failed to read TMP435_LOCAL_TEMP_L_REG, ret:%d", ret);
			return ret;
		}
		if (value > TMP435_FRACTION_INC) {
			temp++;
		}
		data->temp_die = temp + tmp435_temp_offset;
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_AMBIENT_TEMP)) {
		if (!(cfg->external_channel)) {
			return 0; /* not enabled, just return */
		}
		ret = tmp435_reg_read(cfg, TMP435_REMOTE_TEMP_H_REG, &value, sizeof(value));
		if (ret < 0) {
			LOG_ERR("Failed to read TMP435_REMOTE_TEMP_H_REG ret:%d", ret);
			return ret;
		}
		temp = value;
		ret = tmp435_reg_read(cfg, TMP435_REMOTE_TEMP_L_REG, &value, sizeof(value));
		if (ret < 0) {
			LOG_ERR("Failed to read TMP435_REMOTE_TEMP_L_REG, ret:%d", ret);
			return ret;
		}
		if (value > TMP435_FRACTION_INC) {
			temp++;
		}
		data->temp_ambient = temp + tmp435_temp_offset;
	}
	return 0;
}

static int tmp435_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	int ret = 0;
	struct tmp435_data *data = dev->data;
	const struct tmp435_config *cfg = dev->config;

	switch (chan) {

	case SENSOR_CHAN_DIE_TEMP:
		val->val1 = data->temp_die;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		if (cfg->external_channel) {
			val->val1 = data->temp_ambient;
			val->val2 = 0;
		} else {
			ret = -ENOTSUP;
		}
		break;
	default:
		ret = -ENOTSUP;
		break;
	}
	return ret;
}

static const struct sensor_driver_api tmp435_driver_api = {
	.sample_fetch = tmp435_sample_fetch,
	.channel_get = tmp435_channel_get,
};

static int tmp435_init(const struct device *dev)
{
	uint8_t data = 0;
	int ret = 0;
	const struct tmp435_config *cfg = dev->config;

	if (!(i2c_is_ready_dt(&cfg->i2c))) {
		LOG_ERR("I2C dev not ready");
		return -ENODEV;
	}

	data = 1; /* write anything to reset */
	ret = tmp435_reg_write(cfg, TMP435_SOFTWARE_RESET_REG, &data, 1);
	if (ret < 0) {
		LOG_ERR("Failed to write TMP435_SOFTWARE_RESET_REG ret:%d", ret);
		return ret;
	}

	data = TMP435_CONF_REG_1_DATA;
	ret = tmp435_reg_write(cfg, TMP435_CONF_REG_1, &data, 1);
	if (ret < 0) {
		LOG_ERR("Failed to write TMP435_CONF_REG_1 ret:%d", ret);
		return ret;
	}

	data = TMP435_CONF_REG_2_DATA;
	if (cfg->external_channel) {
		data = data + TMP435_CONF_REG_2_REN;
	}
	if (cfg->resistance_correction) {
		data = data + TMP435_CONF_REG_2_RC;
	}
	ret = tmp435_reg_write(cfg, TMP435_CONF_REG_2, &data, 1);
	if (ret < 0) {
		LOG_ERR("Failed to write TMP435_CONF_REG_2 ret:%d", ret);
		return ret;
	}

	data = cfg->beta_compensation;
	ret = tmp435_reg_write(cfg, TMP435_BETA_RANGE_REG, &data, 1);
	if (ret < 0) {
		LOG_ERR("Failed to write TMP435_BETA_RANGE_REG ret:%d", ret);
		return ret;
	}
	return 0;
}

/*
 * Device creation macros
 */

#define TMP435_INST(inst)                                                                          \
	static struct tmp435_data tmp435_data_##inst;                                              \
	static const struct tmp435_config tmp435_config_##inst = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.external_channel = DT_INST_PROP(inst, external_channel),                          \
		.resistance_correction = DT_INST_PROP(inst, resistance_correction),                \
		.beta_compensation = DT_INST_PROP(inst, beta_compensation),                        \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, tmp435_init, NULL, &tmp435_data_##inst,                 \
				     &tmp435_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &tmp435_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TMP435_INST)
