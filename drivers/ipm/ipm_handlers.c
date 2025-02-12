/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/ipm.h>

static inline int z_vrfy_ipm_send(const struct device *dev, int wait,
				  uint32_t id,
				  const void *data, int size)
{
	K_OOPS(K_SYSCALL_DRIVER_IPM(dev, send));
	K_OOPS(K_SYSCALL_MEMORY_READ(data, size));
	return z_impl_ipm_send((const struct device *)dev, wait, id,
			       (const void *)data, size);
}
#include <zephyr/syscalls/ipm_send_mrsh.c>

static inline int z_vrfy_ipm_max_data_size_get(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_IPM(dev, max_data_size_get));
	return z_impl_ipm_max_data_size_get((const struct device *)dev);
}
#include <zephyr/syscalls/ipm_max_data_size_get_mrsh.c>

static inline uint32_t z_vrfy_ipm_max_id_val_get(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_IPM(dev, max_id_val_get));
	return z_impl_ipm_max_id_val_get((const struct device *)dev);
}
#include <zephyr/syscalls/ipm_max_id_val_get_mrsh.c>

static inline int z_vrfy_ipm_set_enabled(const struct device *dev, int enable)
{
	K_OOPS(K_SYSCALL_DRIVER_IPM(dev, set_enabled));
	return z_impl_ipm_set_enabled((const struct device *)dev, enable);
}
#include <zephyr/syscalls/ipm_set_enabled_mrsh.c>
