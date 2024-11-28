/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT fintek_f75303

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor/f75303.h>
#include "f75303.h"

#define F75303_SAMPLE_INT_SHIFT 3
#define F75303_SAMPLE_FRAC_MASK GENMASK(2, 0)
#define F75303_SAMPLE_MICROCELSIUS_PER_BIT 125000

LOG_MODULE_REGISTER(F75303, CONFIG_SENSOR_LOG_LEVEL);

static int f75303_fetch(const struct i2c_dt_spec *i2c,
			uint8_t off_h, uint8_t off_l, uint16_t *sample)
{
	uint8_t val_h;
	uint8_t val_l;
	int res;

	res = i2c_reg_read_byte_dt(i2c, off_h, &val_h);
	if (res) {
		return res;
	}

	res = i2c_reg_read_byte_dt(i2c, off_l, &val_l);
	if (res) {
		return res;
	}

	*sample = val_h << 3 | val_l >> 5;

	return 0;
}

static int f75303_fetch_local(const struct device *dev)
{
	struct f75303_data *data = dev->data;
	const struct f75303_config *config = dev->config;

	return f75303_fetch(&config->i2c,
			    F75303_LOCAL_TEMP_H,
			    F75303_LOCAL_TEMP_L,
			    &data->sample_local);
}

static int f75303_fetch_remote1(const struct device *dev)
{
	struct f75303_data *data = dev->data;
	const struct f75303_config *config = dev->config;

	return f75303_fetch(&config->i2c,
			    F75303_REMOTE1_TEMP_H,
			    F75303_REMOTE1_TEMP_L,
			    &data->sample_remote1);
}

static int f75303_fetch_remote2(const struct device *dev)
{
	struct f75303_data *data = dev->data;
	const struct f75303_config *config = dev->config;

	return f75303_fetch(&config->i2c,
			    F75303_REMOTE2_TEMP_H,
			    F75303_REMOTE2_TEMP_L,
			    &data->sample_remote2);
}

static int f75303_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	enum pm_device_state pm_state;
	int res;

	(void)pm_device_state_get(dev, &pm_state);
	if (pm_state != PM_DEVICE_STATE_ACTIVE) {
		return -EIO;
	}

	switch ((uint32_t)chan) {
	case SENSOR_CHAN_ALL:
		res = f75303_fetch_local(dev);
		if (res) {
			break;
		}
		res = f75303_fetch_remote1(dev);
		if (res) {
			break;
		}
		res = f75303_fetch_remote2(dev);
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		return f75303_fetch_local(dev);
	case SENSOR_CHAN_F75303_REMOTE1:
		return f75303_fetch_remote1(dev);
	case SENSOR_CHAN_F75303_REMOTE2:
		return f75303_fetch_remote2(dev);
	default:
		return -ENOTSUP;
	}

	return res;
}

static int f75303_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct f75303_data *data = dev->data;
	uint16_t sample;

	switch ((uint32_t)chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		sample = data->sample_local;
		break;
	case SENSOR_CHAN_F75303_REMOTE1:
		sample = data->sample_remote1;
		break;
	case SENSOR_CHAN_F75303_REMOTE2:
		sample = data->sample_remote2;
		break;
	default:
		return -ENOTSUP;
	}

	/*
	 * The reading is given in steps of 0.125 degrees celsius, i.e. the
	 * temperature in degrees celsius is equal to sample / 8.
	 */
	val->val1 = sample >> F75303_SAMPLE_INT_SHIFT;
	val->val2 = (sample & F75303_SAMPLE_FRAC_MASK) * F75303_SAMPLE_MICROCELSIUS_PER_BIT;

	return 0;
}

static DEVICE_API(sensor, f75303_driver_api) = {
	.sample_fetch = f75303_sample_fetch,
	.channel_get = f75303_channel_get,
};

static int f75303_init(const struct device *dev)
{
	const struct f75303_config *config = dev->config;
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
static int f75303_pm_action(const struct device *dev, enum pm_device_action action)
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

#define F75303_INST(inst)								\
	static struct f75303_data f75303_data_##inst;					\
	static const struct f75303_config f75303_config_##inst = {			\
		.i2c = I2C_DT_SPEC_INST_GET(inst),					\
	};										\
	PM_DEVICE_DT_INST_DEFINE(inst, f75303_pm_action);				\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, f75303_init, PM_DEVICE_DT_INST_GET(inst),	\
			      &f75303_data_##inst, &f75303_config_##inst, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY, &f75303_driver_api);

DT_INST_FOREACH_STATUS_OKAY(F75303_INST)
