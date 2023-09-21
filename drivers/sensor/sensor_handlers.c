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

#ifdef CONFIG_SENSOR_ASYNC_API
static inline int z_vrfy_sensor_get_decoder(const struct device *dev,
					    const struct sensor_decoder_api **decoder)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SENSOR));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(decoder, sizeof(struct sensor_decoder_api)));
	return z_impl_sensor_get_decoder(dev, decoder);
}
#include <syscalls/sensor_get_decoder_mrsh.c>

static inline int z_vrfy_sensor_reconfigure_read_iodev(struct rtio_iodev *iodev,
						       const struct device *sensor,
						       const enum sensor_channel *channels,
						       size_t num_channels)
{
	Z_OOPS(Z_SYSCALL_OBJ(iodev, K_OBJ_RTIO_IODEV));
	Z_OOPS(Z_SYSCALL_OBJ(sensor, K_OBJ_DRIVER_SENSOR));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(channels, sizeof(enum sensor_channel) * num_channels));
	return z_impl_sensor_reconfigure_read_iodev(iodev, sensor, channels, num_channels);
}
#include <syscalls/sensor_reconfigure_read_iodev_mrsh.c>
#endif
