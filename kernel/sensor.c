/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>
#include <syscall_handler.h>
#include <sensor.h>

int _impl_sensor_attr_set(struct device *dev,
			  enum sensor_channel chan,
			  enum sensor_attribute attr,
			  const struct sensor_value *val)
{
	const struct sensor_driver_api *api = dev->driver_api;

	if (!api->attr_set) {
		return -ENOTSUP;
	}

	return api->attr_set(dev, chan, attr, val);
}

int _impl_sensor_sample_fetch(struct device *dev)
{
	const struct sensor_driver_api *api = dev->driver_api;

	return api->sample_fetch(dev, SENSOR_CHAN_ALL);
}

int _impl_sensor_sample_fetch_chan(struct device *dev,
				   enum sensor_channel type)
{
	const struct sensor_driver_api *api = dev->driver_api;

	return api->sample_fetch(dev, type);
}

int _impl_sensor_channel_get(struct device *dev,
			     enum sensor_channel chan,
			     struct sensor_value *val)
{
	const struct sensor_driver_api *api = dev->driver_api;

	return api->channel_get(dev, chan, val);
}
