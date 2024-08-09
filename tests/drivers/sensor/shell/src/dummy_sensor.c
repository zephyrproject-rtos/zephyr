/*
 * Copyright (c) 2024 THE ARC GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dummy_sensor.h"
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dummy_sensor, LOG_LEVEL_DBG);
#define DT_DRV_COMPAT dummy_sensor

static int dummy_sensor_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan);

	/* Just return success due to dummy sensor driver here. */
	return 0;
}

static int get_data_index(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_LIGHT:
		return 0;
	case SENSOR_CHAN_PROX:
		return 1;
	default:
		return -ENOTSUP;
	}
}

static int dummy_sensor_channel_get(const struct device *dev, enum sensor_channel chan,
				    struct sensor_value *val)
{
	struct dummy_sensor_data *data = dev->data;
	int data_idx = get_data_index(chan);

	if (data_idx < 0) {
		return data_idx;
	}
	val->val1 = data->val[data_idx].val1;
	val->val2 = data->val[data_idx].val2;

	return 0;
}

int dummy_sensor_set_value(const struct device *dev, enum sensor_channel chan,
			   struct sensor_value *val)
{
	struct dummy_sensor_data *data = dev->data;
	int data_idx = get_data_index(chan);

	if (data_idx < 0) {
		return data_idx;
	}
	data->val[data_idx].val1 = val->val1;
	data->val[data_idx].val2 = val->val2;
	return 0;
}

static int dummy_sensor_init(const struct device *dev)
{
	struct dummy_sensor_data *data = dev->data;
	const struct dummy_sensor_config *config = dev->config;
	/* i2c should be null for dummy driver */
	const struct device *i2c = device_get_binding(config->i2c_name);

	if (i2c != NULL) {
		LOG_ERR("Should be Null for %s device!", config->i2c_name);
		return -1;
	}

	/* initialize the channels value for dummy driver */
	for (int i = 0; i < SENSOR_CHANNEL_NUM; i++) {
		data->val[i].val1 = i;
		data->val[i].val2 = i * i;
	}

	return 0;
}

static const struct sensor_driver_api dummy_sensor_api = {
	.sample_fetch = &dummy_sensor_sample_fetch,
	.channel_get = &dummy_sensor_channel_get,
	.trigger_set = NULL,
	.attr_set = NULL,
	.attr_get = NULL,
	.get_decoder = NULL,
	.submit = NULL,
};

#define DUMMY_SENSOR_INIT(inst)                                                                    \
	static struct dummy_sensor_data dummy_sensor_data_##inst;                                  \
                                                                                                   \
	static const struct dummy_sensor_config dummy_sensor_config_##inst = {                     \
		.i2c_name = DT_INST_PROP(inst, i2c_name),                                          \
		.i2c_address = DT_INST_PROP(inst, i2c_address)};                                   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, dummy_sensor_init, NULL, &dummy_sensor_data_##inst,            \
			      &dummy_sensor_config_##inst, POST_KERNEL,                            \
			      CONFIG_SENSOR_INIT_PRIORITY, &dummy_sensor_api);

DT_INST_FOREACH_STATUS_OKAY(DUMMY_SENSOR_INIT)
