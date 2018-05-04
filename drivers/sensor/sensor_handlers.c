/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sensor.h>
#include <syscall_handler.h>

Z_SYSCALL_HANDLER(sensor_attr_set, dev, chan, attr, val)
{
	Z_OOPS(Z_SYSCALL_DRIVER_SENSOR(dev, attr_set));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(val, sizeof(struct sensor_value)));
	return _impl_sensor_attr_set((struct device *)dev, chan, attr,
				     (const struct sensor_value *)val);
}

Z_SYSCALL_HANDLER(sensor_sample_sample_fetch, dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_SENSOR(dev, sample_fetch));
	return _impl_sensor_sample_fetch((struct device *)dev);
}

Z_SYSCALL_HANDLER(sensor_sample_fetch_chan, dev, type)
{
	Z_OOPS(Z_SYSCALL_DRIVER_SENSOR(dev, sample_fetch));
	return _impl_sensor_sample_fetch_chan((struct device *)dev, type);
}

Z_SYSCALL_HANDLER(sensor_channel_get, dev, chan, val)
{
	Z_OOPS(Z_SYSCALL_DRIVER_SENSOR(dev, channel_get));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(val, sizeof(struct sensor_value)));
	return _impl_sensor_channel_get((struct device *)dev, chan,
					(struct sensor_value *)val);
}
