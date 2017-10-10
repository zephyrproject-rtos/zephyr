/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sensor.h>
#include <syscall_handler.h>

u32_t _handler_sensor_attr_set(u32_t dev, u32_t chan, u32_t attr,
			       u32_t val, u32_t arg5, u32_t arg6, void *ssf)
{
	_SYSCALL_ARG4;

	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SENSOR, ssf);
	_SYSCALL_MEMORY_READ(val, sizeof(struct sensor_value), ssf);
	return _impl_sensor_attr_set((struct device *)dev, chan, attr,
				     (const struct sensor_value *)val);
}

u32_t _handler_sensor_sample_fetch(u32_t dev, u32_t arg2, u32_t arg3,
				   u32_t arg4, u32_t arg5, u32_t arg6,
				   void *ssf)
{
	_SYSCALL_ARG1;

	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SENSOR, ssf);
	return _impl_sensor_sample_fetch((struct device *)dev);
}


u32_t _handler_sensor_sample_fetch_chan(u32_t dev, u32_t type, u32_t arg3,
					u32_t arg4, u32_t arg5, u32_t arg6,
					void *ssf)
{
	_SYSCALL_ARG2;

	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SENSOR, ssf);
	return _impl_sensor_sample_fetch_chan((struct device *)dev, type);
}

u32_t _handler_sensor_channel_get(u32_t dev, u32_t chan, u32_t val,
				  u32_t arg4, u32_t arg5, u32_t arg6,
				  void *ssf)
{
	_SYSCALL_ARG3;

	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SENSOR, ssf);
	_SYSCALL_MEMORY_WRITE(val, sizeof(struct sensor_value), ssf);
	return _impl_sensor_channel_get((struct device *)dev, chan,
					(struct sensor_value *)val);
}

