/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/syscall_handler.h>

static inline int z_vrfy_sensor_attr_set(const struct device *dev,
					 enum sensor_channel chan,
					 enum sensor_attribute attr,
					 const struct sensor_value *val)
{
	Z_OOPS(Z_SYSCALL_DRIVER_SENSOR(dev, attr_set));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(val, sizeof(struct sensor_value)));
	return z_impl_sensor_attr_set((const struct device *)dev, chan, attr,
				      (const struct sensor_value *)val);
}
#include <syscalls/sensor_attr_set_mrsh.c>

static inline int z_vrfy_sensor_attr_get(const struct device *dev,
					 enum sensor_channel chan,
					 enum sensor_attribute attr,
					 struct sensor_value *val)
{
	Z_OOPS(Z_SYSCALL_DRIVER_SENSOR(dev, attr_get));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(val, sizeof(struct sensor_value)));
	return z_impl_sensor_attr_get((const struct device *)dev, chan, attr,
				      (struct sensor_value *)val);
}
#include <syscalls/sensor_attr_get_mrsh.c>

static inline int z_vrfy_sensor_sample_fetch(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_SENSOR(dev, sample_fetch));
	return z_impl_sensor_sample_fetch((const struct device *)dev);
}
#include <syscalls/sensor_sample_fetch_mrsh.c>

static inline int z_vrfy_sensor_sample_fetch_chan(const struct device *dev,
						  enum sensor_channel type)
{
	Z_OOPS(Z_SYSCALL_DRIVER_SENSOR(dev, sample_fetch));
	return z_impl_sensor_sample_fetch_chan((const struct device *)dev,
					       type);
}
#include <syscalls/sensor_sample_fetch_chan_mrsh.c>

static inline int z_vrfy_sensor_channel_get(const struct device *dev,
					    enum sensor_channel chan,
					    struct sensor_value *val)
{
	Z_OOPS(Z_SYSCALL_DRIVER_SENSOR(dev, channel_get));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(val, sizeof(struct sensor_value)));
	return z_impl_sensor_channel_get((const struct device *)dev, chan,
					 (struct sensor_value *)val);
}
#include <syscalls/sensor_channel_get_mrsh.c>
