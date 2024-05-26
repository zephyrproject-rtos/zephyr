/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vnd_fake_sensor

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(fake_sensor);

static int init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		    const struct sensor_value *val)
{
	LOG_DBG("[%s] dev: %p, chan: %d, attr: %d, val1: %d, val2: %d", __func__, dev, chan, attr,
		val->val1, val->val2);

	return 0;
}

static int attr_get(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		    struct sensor_value *val)
{
	LOG_DBG("[%s] dev: %p, chan: %d, attr: %d", __func__, dev, chan, attr);

	val->val1 = chan;
	val->val2 = attr * 100000;

	return 0;
}

static int sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	LOG_DBG("[%s] dev: %p, chan: %d", __func__, dev, chan);

	return 0;
}

static int channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
	LOG_DBG("[%s] dev: %p, chan: %d", __func__, dev, chan);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		__fallthrough;
	case SENSOR_CHAN_GYRO_XYZ:
		__fallthrough;
	case SENSOR_CHAN_MAGN_XYZ:
		__fallthrough;
	case SENSOR_CHAN_POS_DXYZ:
		for (int i = 0; i < 3; i++, val++) {
			val->val1 = chan;
			val->val2 = 1;
		}
		break;
	default:
		val->val1 = chan;
		val->val2 = 1;
		break;
	}

	return 0;
}

static int trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	LOG_DBG("[%s - %s] dev: %p, trig->chan: %d, trig->type: %d, handler: %p", __func__,
		(handler == NULL) ? "off" : "on", dev, trig->chan, trig->type, handler);

	return 0;
}

static const struct sensor_driver_api api = {
	.attr_get = attr_get,
	.attr_set = attr_set,
	.sample_fetch = sample_fetch,
	.channel_get = channel_get,
	.trigger_set = trigger_set,
};

#define VND_SENSOR_INIT(n)                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(n, init, NULL, NULL, NULL, POST_KERNEL,                       \
				     CONFIG_SENSOR_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(VND_SENSOR_INIT)
