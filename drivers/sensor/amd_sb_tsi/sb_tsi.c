/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT amd_sb_tsi

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/logging/log.h>
#include "sb_tsi.h"

LOG_MODULE_REGISTER(AMD_SB_TSI, CONFIG_SENSOR_LOG_LEVEL);

struct sb_tsi_data {
	uint8_t sample_int;
	uint8_t sample_dec;
};

struct sb_tsi_config {
	struct i2c_dt_spec i2c;
};

static int sb_tsi_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct sb_tsi_data *data = dev->data;
	const struct sb_tsi_config *config = dev->config;
	enum pm_device_state pm_state;
	int res;

	(void)pm_device_state_get(dev, &pm_state);
	if (pm_state != PM_DEVICE_STATE_ACTIVE) {
		return -EIO;
	}

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	/*
	 * ReadOrder specifies the order for atomically reading the temp.
	 * The reset value is 0, which means reading Int latches Dec.
	 */
	res = i2c_reg_read_byte_dt(&config->i2c, SB_TSI_TEMP_INT, &data->sample_int);
	if (res) {
		return res;
	}

	res = i2c_reg_read_byte_dt(&config->i2c, SB_TSI_TEMP_DEC, &data->sample_dec);
	if (res) {
		return res;
	}

	return 0;
}

static int sb_tsi_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct sb_tsi_data *data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	val->val1 = data->sample_int;
	val->val2 = (data->sample_dec >> SB_TSI_TEMP_DEC_SHIFT) *
		    (1000000 / SB_TSI_TEMP_DEC_SCALE);

	return 0;
}

static DEVICE_API(sensor, sb_tsi_driver_api) = {
	.sample_fetch = sb_tsi_sample_fetch,
	.channel_get = sb_tsi_channel_get,
};

static int sb_tsi_init(const struct device *dev)
{
	const struct sb_tsi_config *config = dev->config;
	int res = 0;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME
	pm_device_init_suspended(dev);

	res = pm_device_runtime_enable(dev);
	if (res) {
		LOG_ERR("Failed to enable runtime power management");
	}
#endif

	return res;
}

#ifdef CONFIG_PM_DEVICE
static int sb_tsi_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_RESUME:
	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_SUSPEND:
		return 0;
	default:
		return -ENOTSUP;
	}
}
#endif

#define SB_TSI_INST(inst)								\
	static struct sb_tsi_data sb_tsi_data_##inst;					\
	static const struct sb_tsi_config sb_tsi_config_##inst = {			\
		.i2c = I2C_DT_SPEC_INST_GET(inst),					\
	};										\
	PM_DEVICE_DT_INST_DEFINE(inst, sb_tsi_pm_action);				\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, sb_tsi_init, PM_DEVICE_DT_INST_GET(inst),	\
			      &sb_tsi_data_##inst, &sb_tsi_config_##inst, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY, &sb_tsi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SB_TSI_INST)
