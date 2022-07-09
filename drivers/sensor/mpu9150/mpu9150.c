/*
 * Copyright (c) 2022 Benjamin Björnsson <benjamin.bjornsson@gmail.com>.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_mpu9150

/* Init priority (CONFIG_SENSOR_INIT_PRIORITY - 1) to guarantee initialization before AK8975. */
#define MPU9150_INIT_PRIORITY 89

#include "mpu9150.h"

#include <devicetree.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MPU9150, CONFIG_SENSOR_LOG_LEVEL);

static int mpu9150_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	return -ENOTSUP;
}

static int mpu9150_attr_get(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, struct sensor_value *val)
{
	return -ENOTSUP;
}

static int mpu9150_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			       sensor_trigger_handler_t handler)
{
	return -ENOTSUP;
}

static int mpu9150_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	return -ENOTSUP;
}

static int mpu9150_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	return -ENOTSUP;
}

static const struct sensor_driver_api mpu9150_driver_api = {
	.attr_set = mpu9150_attr_set,
	.attr_get = mpu9150_attr_get,
	.trigger_set = mpu9150_trigger_set,
	.sample_fetch = mpu9150_sample_fetch,
	.channel_get = mpu9150_channel_get,
};

int mpu9150_init(const struct device *dev)
{
	const struct mpu9150_config *config = dev->config;
	int res;

	if (device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	/* wake up MPU9150 chip */
	res = i2c_reg_update_byte_dt(&config->i2c, MPU9150_REG_PWR_MGMT1, MPU9150_SLEEP_EN, 0);
	if (res < 0) {
		LOG_ERR("Failed to wake up MPU9150 chip.");
		return res;
	}

	if (config->ak8975_pass_through) {
		/* enable MPU9150 pass-though to have access to AK8975 */
		res = i2c_reg_update_byte_dt(&config->i2c, MPU9150_REG_BYPASS_CFG,
					     MPU9150_I2C_BYPASS_EN, MPU9150_I2C_BYPASS_EN);
		if (res < 0) {
			LOG_ERR("Failed to enable pass-through mode for AK8975.");
			return res;
		}
	}

	return 0;
}

#define MPU9150_DEFINE(inst)									\
	static const struct mpu9150_config mpu9150_config_##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		.ak8975_pass_through = DT_INST_NODE_HAS_PROP(inst, ak8975_pass_through),	\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, mpu9150_init, NULL,						\
			      NULL, &mpu9150_config_##inst, POST_KERNEL,			\
			      MPU9150_INIT_PRIORITY, &mpu9150_driver_api);			\

DT_INST_FOREACH_STATUS_OKAY(MPU9150_DEFINE)
