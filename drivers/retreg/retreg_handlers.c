/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/syscall_handler.h>
#include <zephyr/drivers/retreg.h>

static inline int z_vrfy_retreg_read(const struct device *dev, uint32_t reg_idx,
				    void *data, size_t len)
{
	Z_OOPS(Z_SYSCALL_DRIVER_RETREG(dev, read));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(data, len));
	return z_impl_retreg_read((const struct device *)dev, reg_idx,
				 (void *)data,
				 len);
}
#include <syscalls/retreg_read_mrsh.c>

static inline int z_vrfy_retreg_write(const struct device *dev, uint32_t reg_idx,
				     const void *data, size_t len)
{
	Z_OOPS(Z_SYSCALL_DRIVER_RETREG(dev, write));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(data, len));
	return z_impl_retreg_write((const struct device *)dev, reg_idx,
				  (const void *)data, len);
}
#include <syscalls/retreg_write_mrsh.c>

static inline const struct retreg_layout *z_vrfy_get_retreg_layout(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_RETREG(dev, get_parameters));
	return z_impl_get_reteg_layou(dev);
}
#include <syscalls/retreg_retreg_layout_mrsh.c>

struct retreg_layout *z_impl_get_retreg_layout(const struct device *dev);
