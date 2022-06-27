/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/syscall_handler.h>
#include <zephyr/drivers/flash.h>

static inline int z_vrfy_flash_read(const struct device *dev, off_t offset,
				    void *data, size_t len)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, read));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(data, len));
	return z_impl_flash_read((const struct device *)dev, offset,
				 (void *)data,
				 len);
}
#include <syscalls/flash_read_mrsh.c>

static inline int z_vrfy_flash_write(const struct device *dev, off_t offset,
				     const void *data, size_t len)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, write));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(data, len));
	return z_impl_flash_write((const struct device *)dev, offset,
				  (const void *)data, len);
}
#include <syscalls/flash_write_mrsh.c>

static inline size_t z_vrfy_flash_get_write_block_size(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_FLASH));
	return z_impl_flash_get_write_block_size(dev);
}
#include <syscalls/flash_get_write_block_size_mrsh.c>

static inline const struct flash_parameters *z_vrfy_flash_get_parameters(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, get_parameters));
	return z_impl_flash_get_parameters(dev);
}
#include <syscalls/flash_get_parameters_mrsh.c>

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static inline int z_vrfy_flash_get_page_info_by_offs(const struct device *dev,
						     off_t offs,
						     struct flash_pages_info *info)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, page_layout));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(info, sizeof(struct flash_pages_info)));
	return z_impl_flash_get_page_info_by_offs((const struct device *)dev,
						  offs,
						  (struct flash_pages_info *)info);
}
#include <syscalls/flash_get_page_info_by_offs_mrsh.c>

static inline int z_vrfy_flash_get_page_info_by_idx(const struct device *dev,
						    uint32_t idx,
						    struct flash_pages_info *info)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, page_layout));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(info, sizeof(struct flash_pages_info)));
	return z_impl_flash_get_page_info_by_idx((const struct device *)dev,
						 idx,
						 (struct flash_pages_info *)info);
}
#include <syscalls/flash_get_page_info_by_idx_mrsh.c>

static inline size_t z_vrfy_flash_get_page_count(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, page_layout));
	return z_impl_flash_get_page_count((const struct device *)dev);
}
#include <syscalls/flash_get_page_count_mrsh.c>

#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#ifdef CONFIG_FLASH_JESD216_API

static inline int z_vrfy_flash_sfdp_read(const struct device *dev,
					 off_t offset,
					 void *data, size_t len)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, sfdp_read));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(data, len));
	return z_impl_flash_sfdp_read(dev, offset, data, len);
}
#include <syscalls/flash_sfdp_read.c>

static inline int z_vrfy_flash_read_jedec_id(const struct device *dev,
					     uint8_t *id)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, read_jedec_id));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(id, 3));
	return z_impl_flash_read_jedec_id(dev, id);
}
#include <syscalls/flash_sfdp_jedec_id.c>

#endif /* CONFIG_FLASH_JESD216_API */
