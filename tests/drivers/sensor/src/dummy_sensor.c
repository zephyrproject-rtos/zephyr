/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dummy_sensor.h"
#include <drivers/sensor.h>
#include <device.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(dummy_sensor, LOG_LEVEL_DBG);
static struct dummy_sensor_data dummy_data;
/**
 * @brief config bus address at compile time
 */
static const struct dummy_sensor_config dummy_config = {
	.i2c_name = "dummy I2C",
	.i2c_address = 123
};

static int dummy_sensor_sample_fetch(struct device *dev,
				enum sensor_channel chan)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan);

	/* Just return success due to dummy sensor driver here. */
	return 0;
}

static int dummy_sensor_channel_get(struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct dummy_sensor_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_LIGHT:
		val->val1 = data->val[0].val1;
		val->val2 = data->val[0].val2;
		break;
	case SENSOR_CHAN_RED:
		val->val1 = data->val[1].val1;
		val->val2 = data->val[1].val2;
		break;
	case SENSOR_CHAN_GREEN:
		val->val1 = data->val[2].val1;
		val->val2 = data->val[2].val2;
		break;
	case SENSOR_CHAN_BLUE:
		val->val1 = data->val[3].val1;
		val->val2 = data->val[3].val2;
		break;
	case SENSOR_CHAN_PROX:
		val->val1 = data->val[4].val1;
		val->val2 = data->val[4].val2;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

/* return 0 for dummy driver to imitate interrupt */
static int dummy_init_interrupt(struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int dummy_sensor_init(struct device *dev)
{
	struct dummy_sensor_data *data = dev->data;
	const struct dummy_sensor_config *config = dev->config;
	/* i2c should be null for dummy driver */
	struct device *i2c = device_get_binding(config->i2c_name);

	if (i2c != NULL) {
		LOG_ERR("Should be Null for %s device!", config->i2c_name);
		return -1;
	}

	if (dummy_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt!");
		return -1;
	}

	/* initialize the channels value for dummy driver */
	for (int i = 0; i < SENSOR_CHANNEL_NUM; i++) {
		data->val[i].val1 = i;
		data->val[i].val2 = i*i;
	}

	return 0;
}

int dummy_sensor_attr_set(struct device *dev,
		      enum sensor_channel chan,
		      enum sensor_attribute attr,
		      const struct sensor_value *val)
{
	struct dummy_sensor_data *data = dev->data;

	if (chan == SENSOR_CHAN_PROX &&
	    attr == SENSOR_ATTR_UPPER_THRESH) {
		data->val[4].val1 = val->val1;
		data->val[4].val2 = val->val2;
		return 0;
	}

	return -ENOTSUP;
}

int dummy_sensor_attr_get(struct device *dev,
		      enum sensor_channel chan,
		      enum sensor_attribute attr,
		      struct sensor_value *val)
{
	struct dummy_sensor_data *data = dev->data;

	if (chan == SENSOR_CHAN_PROX &&
	    attr == SENSOR_ATTR_UPPER_THRESH) {
		val->val1 = data->val[4].val1;
		val->val2 = data->val[4].val2;
		return 0;
	}

	return -ENOTSUP;
}

int dummy_sensor_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct dummy_sensor_data *data = dev->data;

	switch (trig->type) {
	case SENSOR_TRIG_THRESHOLD:
	case SENSOR_TRIG_TIMER:
	case SENSOR_TRIG_DATA_READY:
	case SENSOR_TRIG_DELTA:
	case SENSOR_TRIG_NEAR_FAR:
		/* Use the same action to dummy above triggers */
		if (handler != NULL) {
			data->handler = handler;
			data->handler(dev, NULL);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api dummy_sensor_api = {
	.sample_fetch = &dummy_sensor_sample_fetch,
	.channel_get = &dummy_sensor_channel_get,
	.attr_set = dummy_sensor_attr_set,
	.attr_get = dummy_sensor_attr_get,
	.trigger_set = dummy_sensor_trigger_set,
};

DEVICE_DEFINE(dummy_sensor, DUMMY_SENSOR_NAME, &dummy_sensor_init,
		    NULL, &dummy_data, &dummy_config, APPLICATION,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &dummy_sensor_api);
