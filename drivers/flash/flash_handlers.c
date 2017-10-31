/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <flash.h>

_SYSCALL_HANDLER(flash_read, dev, offset, data, len)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_FLASH);
	_SYSCALL_MEMORY_WRITE(data, len);
	return _impl_flash_read((struct device *)dev, offset, (void *)data,
				len);
}

_SYSCALL_HANDLER(flash_write, dev, offset, data, len)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_FLASH);
	_SYSCALL_MEMORY_READ(data, len);
	return _impl_flash_write((struct device *)dev, offset,
				 (const void *)data, len);
}

_SYSCALL_HANDLER(flash_write_protection_set, dev, enable)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_FLASH);
	return _impl_flash_write_protection_set((struct device *)dev, enable);
}

_SYSCALL_HANDLER1_SIMPLE(flash_get_write_block_size, K_OBJ_DRIVER_FLASH,
			 struct device *);

#ifdef CONFIG_FLASH_PAGE_LAYOUT
_SYSCALL_HANDLER(flash_get_page_info_by_offs, dev, offs, info)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_FLASH);
	_SYSCALL_MEMORY_WRITE(info, sizeof(struct flash_pages_info));
	return _impl_flash_get_page_info_by_offs((struct device *)dev, offs,
					(struct flash_pages_info *)info);
}

_SYSCALL_HANDLER(flash_get_page_info_by_idx, dev, idx, info)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_FLASH);
	_SYSCALL_MEMORY_WRITE(info, sizeof(struct flash_pages_info));
	return _impl_flash_get_page_info_by_idx((struct device *)dev, idx,
					(struct flash_pages_info *)info);
}

_SYSCALL_HANDLER1_SIMPLE(flash_get_page_count, K_OBJ_DRIVER_FLASH,
			 struct device *);
#endif
