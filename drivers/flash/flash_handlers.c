/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/flash.h>

static inline int z_vrfy_flash_read(const struct device *dev, k_off_t offset,
				    void *data, size_t len)
{
	K_OOPS(K_SYSCALL_DRIVER_FLASH(dev, read));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(data, len));
	return z_impl_flash_read((const struct device *)dev, offset,
				 (void *)data,
				 len);
}
#include <zephyr/syscalls/flash_read_mrsh.c>

static inline int z_vrfy_flash_write(const struct device *dev, k_off_t offset,
				     const void *data, size_t len)
{
	K_OOPS(K_SYSCALL_DRIVER_FLASH(dev, write));
	K_OOPS(K_SYSCALL_MEMORY_READ(data, len));
	return z_impl_flash_write((const struct device *)dev, offset,
				  (const void *)data, len);
}
#include <zephyr/syscalls/flash_write_mrsh.c>

static inline int z_vrfy_flash_erase(const struct device *dev, k_off_t offset,
				     size_t size)
{
	K_OOPS(K_SYSCALL_DRIVER_FLASH(dev, erase));
	return z_impl_flash_erase((const struct device *)dev, offset, size);
}
#include <zephyr/syscalls/flash_erase_mrsh.c>

static inline size_t z_vrfy_flash_get_write_block_size(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_FLASH));
	return z_impl_flash_get_write_block_size(dev);
}
#include <zephyr/syscalls/flash_get_write_block_size_mrsh.c>

static inline const struct flash_parameters *z_vrfy_flash_get_parameters(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_FLASH(dev, get_parameters));
	return z_impl_flash_get_parameters(dev);
}
#include <zephyr/syscalls/flash_get_parameters_mrsh.c>

int z_vrfy_flash_fill(const struct device *dev, uint8_t val, k_off_t offset,
		      size_t size)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_FLASH));
	return z_impl_flash_fill(dev, val, offset, size);
}
#include <zephyr/syscalls/flash_fill_mrsh.c>

int z_vrfy_flash_flatten(const struct device *dev, k_off_t offset, size_t size)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_FLASH));
	return z_impl_flash_flatten(dev, offset, size);
}

#include <zephyr/syscalls/flash_flatten_mrsh.c>

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static inline int z_vrfy_flash_get_page_info_by_offs(const struct device *dev,
						     k_off_t offs,
						     struct flash_pages_info *info)
{
	K_OOPS(K_SYSCALL_DRIVER_FLASH(dev, page_layout));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(info, sizeof(struct flash_pages_info)));
	return z_impl_flash_get_page_info_by_offs((const struct device *)dev,
						  offs,
						  (struct flash_pages_info *)info);
}
#include <zephyr/syscalls/flash_get_page_info_by_offs_mrsh.c>

static inline int z_vrfy_flash_get_page_info_by_idx(const struct device *dev,
						    uint32_t idx,
						    struct flash_pages_info *info)
{
	K_OOPS(K_SYSCALL_DRIVER_FLASH(dev, page_layout));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(info, sizeof(struct flash_pages_info)));
	return z_impl_flash_get_page_info_by_idx((const struct device *)dev,
						 idx,
						 (struct flash_pages_info *)info);
}
#include <zephyr/syscalls/flash_get_page_info_by_idx_mrsh.c>

static inline size_t z_vrfy_flash_get_page_count(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_FLASH(dev, page_layout));
	return z_impl_flash_get_page_count((const struct device *)dev);
}
#include <zephyr/syscalls/flash_get_page_count_mrsh.c>

#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#ifdef CONFIG_FLASH_JESD216_API

static inline int z_vrfy_flash_sfdp_read(const struct device *dev,
					 k_off_t offset,
					 void *data, size_t len)
{
	K_OOPS(K_SYSCALL_DRIVER_FLASH(dev, sfdp_read));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(data, len));
	return z_impl_flash_sfdp_read(dev, offset, data, len);
}
#include <zephyr/syscalls/flash_sfdp_read_mrsh.c>

static inline int z_vrfy_flash_read_jedec_id(const struct device *dev,
					     uint8_t *id)
{
	K_OOPS(K_SYSCALL_DRIVER_FLASH(dev, read_jedec_id));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(id, 3));
	return z_impl_flash_read_jedec_id(dev, id);
}
#include <zephyr/syscalls/flash_read_jedec_id_mrsh.c>

#endif /* CONFIG_FLASH_JESD216_API */

#ifdef CONFIG_FLASH_EX_OP_ENABLED

static inline int z_vrfy_flash_ex_op(const struct device *dev, uint16_t code,
				     const uintptr_t in, void *out)
{
	K_OOPS(K_SYSCALL_DRIVER_FLASH(dev, ex_op));

	/*
	 * If the code is a vendor code, then ex_op function have to perform
	 * verification. Zephyr codes should be verified here, but currently
	 * there are no Zephyr extended codes yet.
	 */

	return z_impl_flash_ex_op(dev, code, in, out);
}
#include <zephyr/syscalls/flash_ex_op_mrsh.c>

#endif /* CONFIG_FLASH_EX_OP_ENABLED */
