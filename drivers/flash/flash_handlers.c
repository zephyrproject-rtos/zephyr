/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <flash.h>

Z_SYSCALL_HANDLER(flash_read, dev, offset, data, len)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, read));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(data, len));
	return _impl_flash_read((struct device *)dev, offset, (void *)data,
				len);
}

Z_SYSCALL_HANDLER(flash_write, dev, offset, data, len)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, write));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(data, len));
	return _impl_flash_write((struct device *)dev, offset,
				 (const void *)data, len);
}

Z_SYSCALL_HANDLER(flash_write_protection_set, dev, enable)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, write_protection));
	return _impl_flash_write_protection_set((struct device *)dev, enable);
}

Z_SYSCALL_HANDLER1_SIMPLE(flash_get_write_block_size, K_OBJ_DRIVER_FLASH,
			  struct device *);

#ifdef CONFIG_FLASH_PAGE_LAYOUT
Z_SYSCALL_HANDLER(flash_get_page_info_by_offs, dev, offs, info)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, page_layout));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(info, sizeof(struct flash_pages_info)));
	return _impl_flash_get_page_info_by_offs((struct device *)dev, offs,
					(struct flash_pages_info *)info);
}

Z_SYSCALL_HANDLER(flash_get_page_info_by_idx, dev, idx, info)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, page_layout));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(info, sizeof(struct flash_pages_info)));
	return _impl_flash_get_page_info_by_idx((struct device *)dev, idx,
					(struct flash_pages_info *)info);
}

Z_SYSCALL_HANDLER(flash_get_page_count, dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FLASH(dev, page_layout));
	return _impl_flash_get_page_count((struct device *)dev);
}
#endif
