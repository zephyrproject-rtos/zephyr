/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <drivers/flash.h>

static inline int z_vrfy_flash_read(struct device *dev, off_t offset,
				    void *data, size_t len)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, read));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(data, len));
	return z_impl_flash_read((struct device *)dev, offset, (void *)data,
				len);
}
#include <syscalls/flash_read_mrsh.c>

static inline int z_vrfy_flash_write(struct device *dev, off_t offset,
				    const void *data, size_t len)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, write));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(data, len));
	return z_impl_flash_write((struct device *)dev, offset,
				 (const void *)data, len);
}
#include <syscalls/flash_write_mrsh.c>

static inline int z_vrfy_flash_write_protection_set(struct device *dev,
						    bool enable)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, write_protection));
	return z_impl_flash_write_protection_set((struct device *)dev, enable);
}
#include <syscalls/flash_write_protection_set_mrsh.c>

static inline size_t z_vrfy_flash_get_write_block_size(struct device *dev)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_FLASH));
	return z_impl_flash_get_write_block_size(dev);
}
#include <syscalls/flash_get_write_block_size_mrsh.c>

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static inline int z_vrfy_flash_get_page_info_by_offs(struct device *dev,
					      off_t offs,
					      struct flash_pages_info *info)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, page_layout));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(info, sizeof(struct flash_pages_info)));
	return z_impl_flash_get_page_info_by_offs((struct device *)dev, offs,
					(struct flash_pages_info *)info);
}
#include <syscalls/flash_get_page_info_by_offs_mrsh.c>

static inline int z_vrfy_flash_get_page_info_by_idx(struct device *dev,
					 u32_t idx,
					 struct flash_pages_info *info)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, page_layout));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(info, sizeof(struct flash_pages_info)));
	return z_impl_flash_get_page_info_by_idx((struct device *)dev, idx,
					(struct flash_pages_info *)info);
}
#include <syscalls/flash_get_page_info_by_idx_mrsh.c>

static inline size_t z_vrfy_flash_get_page_count(struct device *dev);
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, page_layout));
	return z_impl_flash_get_page_count((struct device *)dev);
}
#include <syscalls/flash_get_page_count_mrsh.c>

#endif
