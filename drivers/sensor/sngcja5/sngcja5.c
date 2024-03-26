/*
 * Copyright (c) 2023 Panasonic Industrial Devices Europe GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT panasonic_sngcja5

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "sngcja5.h"

LOG_MODULE_REGISTER(SNGCJA5, CONFIG_SENSOR_LOG_LEVEL);

static int read_register_4(const struct i2c_dt_spec *dev, uint8_t addr, uint32_t *value)
{
	if (i2c_burst_read_dt(dev, addr, (uint8_t *)value, sizeof(*value)) < 0) {
		LOG_ERR("i2c_burst_read_dt() @ i2c failed");
		return -EIO;
	}

	*value = sys_le32_to_cpu(*value);

	return 0;
}

static int read_register_2(const struct i2c_dt_spec *dev, uint8_t addr, uint16_t *value)
{
	if (i2c_burst_read_dt(dev, addr, (uint8_t *)value, sizeof(*value)) < 0) {
		LOG_ERR("i2c_burst_read_dt() @ i2c failed");
		return -EIO;
	}

	*value = sys_le16_to_cpu(*value);

	return 0;
}

static int sngcja5_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct sngcja5_data *device_data = dev->data;
	const struct sngcja5_config *device_config = dev->config;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (read_register_4(&device_config->i2c, SNGCJA5_PM10_LL, &(device_data->pm1_0))) {
		return -EIO;
	}
	if (read_register_4(&device_config->i2c, SNGCJA5_PM25_LL, &(device_data->pm2_5))) {
		return -EIO;
	}
	if (read_register_4(&device_config->i2c, SNGCJA5_PM100_LL, &(device_data->pm10_0))) {
		return -EIO;
	}
	if (read_register_2(&device_config->i2c, SNGCJA5_05_L, &(device_data->pc0_5))) {
		return -EIO;
	}
	if (read_register_2(&device_config->i2c, SNGCJA5_10_L, &(device_data->pc1_0))) {
		return -EIO;
	}
	if (read_register_2(&device_config->i2c, SNGCJA5_25_L, &(device_data->pc2_5))) {
		return -EIO;
	}
	if (read_register_2(&device_config->i2c, SNGCJA5_50_L, &(device_data->pc5_0))) {
		return -EIO;
	}
	if (read_register_2(&device_config->i2c, SNGCJA5_75_L, &(device_data->pc7_5))) {
		return -EIO;
	}
	if (read_register_2(&device_config->i2c, SNGCJA5_100_L, &(device_data->pc10_0))) {
		return -EIO;
	}

	return 0;
}

static int sngcja5_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct sngcja5_data *device_data = dev->data;
	int32_t uval;

	__ASSERT_NO_MSG((chan == SENSOR_CHAN_PARTICLE_COUNT) || (chan == SENSOR_CHAN_PM_1_0) ||
			(chan == SENSOR_CHAN_PM_2_5) || (chan == SENSOR_CHAN_PM_10));
	switch (chan) {
	case SENSOR_CHAN_PARTICLE_COUNT:
		val[0].val1 = device_data->pc0_5;
		val[0].val2 = 0;
		val[1].val1 = device_data->pc1_0;
		val[1].val2 = 0;
		val[2].val1 = device_data->pc2_5;
		val[2].val2 = 0;
		val[3].val1 = device_data->pc5_0;
		val[3].val2 = 0;
		val[4].val1 = device_data->pc7_5;
		val[4].val2 = 0;
		val[5].val1 = device_data->pc10_0;
		val[5].val2 = 0;
		break;
	case SENSOR_CHAN_PM_1_0:
		uval = (int32_t)device_data->pm1_0;
		val->val1 = uval / SNGCJA5_SCALE_FACTOR;
		val->val2 = uval % SNGCJA5_SCALE_FACTOR;
		break;
	case SENSOR_CHAN_PM_2_5:
		uval = (int32_t)device_data->pm2_5;
		val->val1 = uval / SNGCJA5_SCALE_FACTOR;
		val->val2 = uval % SNGCJA5_SCALE_FACTOR;
		break;
	case SENSOR_CHAN_PM_10:
		uval = (int32_t)device_data->pm10_0;
		val->val1 = uval / SNGCJA5_SCALE_FACTOR;
		val->val2 = uval % SNGCJA5_SCALE_FACTOR;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

int sngcja5_init(const struct device *dev)
{
	const struct sngcja5_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus device is not ready");
		return -ENODEV;
	}

	if (i2c_configure(config->i2c.bus,
			  I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER)) {
		LOG_ERR("i2c_configure() failed");
		return -EIO;
	}

	return 0;
}

static const struct sensor_driver_api sngcja5_driver_api = {
	.sample_fetch = sngcja5_sample_fetch,
	.channel_get = sngcja5_channel_get,
};

#define SNGCJA5_DEFINE(inst)                                                                       \
	static struct sngcja5_data sngcja5_data_##inst;                                            \
                                                                                                   \
	static const struct sngcja5_config sngcja5_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, sngcja5_init, NULL, &sngcja5_data_##inst,               \
				     &sngcja5_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &sngcja5_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SNGCJA5_DEFINE)
