/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <drivers/ipm.h>

static inline int z_vrfy_ipm_send(const struct device *dev, int wait,
				  uint32_t channel, struct ipm_msg *msg)
{
	Z_OOPS(Z_SYSCALL_DRIVER_IPM(dev, send));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(msg, sizeof(struct ipm_msg)));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(msg->data, msg->size));
	return z_impl_ipm_send((const struct device *)dev, wait, msg);
}
#include <syscalls/ipm_send_mrsh.c>

static inline int z_vrfy_ipm_max_data_size_get(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_IPM(dev, max_data_size_get));
	return z_impl_ipm_max_data_size_get((const struct device *)dev);
}
#include <syscalls/ipm_max_data_size_get_mrsh.c>

static inline uint32_t z_vrfy_ipm_max_id_val_get(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_IPM(dev, max_id_val_get));
	return z_impl_ipm_max_id_val_get((const struct device *)dev);
}
#include <syscalls/ipm_max_id_val_get_mrsh.c>

static inline uint32_t z_vrfy_ipm_max_channels_get(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_IPM(dev, max_channels_get));
	return z_impl_ipm_max_channels_get((const struct device *)dev);
}
#include <syscalls/ipm_max_channels_get_mrsh.c>

static inline int z_vrfy_ipm_set_enabled(const struct device *dev, int enable)
{
	Z_OOPS(Z_SYSCALL_DRIVER_IPM(dev, set_enabled));
	return z_impl_ipm_set_enabled((const struct device *)dev, enable);
}
#include <syscalls/ipm_set_enabled_mrsh.c>

static inline int z_vrfy_ipm_configure_channel(const struct device *dev,
					       uint32_t channel,
					       void *conf)
{
	Z_OOPS(Z_SYSCALL_DRIVER_IPM(dev, configure_channel));
	return z_impl_ipm_configure_channel((const struct device *)dev, channel, conf);
}
#include <syscalls/ipm_configure_channel_mrsh.c>
