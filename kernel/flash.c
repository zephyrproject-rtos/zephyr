/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>
#include <syscall_handler.h>
#include <flash.h>

int _impl_flash_read(struct device *dev, off_t offset, void *data,
		     size_t len)
{
	const struct flash_driver_api *api = dev->driver_api;

	return api->read(dev, offset, data, len);
}

int _impl_flash_write(struct device *dev, off_t offset,
		      const void *data, size_t len)
{
	const struct flash_driver_api *api = dev->driver_api;

	return api->write(dev, offset, data, len);
}

int _impl_flash_erase(struct device *dev, off_t offset,
		      size_t size)
{
	const struct flash_driver_api *api = dev->driver_api;

	return api->erase(dev, offset, size);
}

int _impl_flash_write_protection_set(struct device *dev,
				     bool enable)
{
	const struct flash_driver_api *api = dev->driver_api;

	return api->write_protection(dev, enable);
}

size_t _impl_flash_get_write_block_size(struct device *dev)
{
	const struct flash_driver_api *api = dev->driver_api;

	return api->write_block_size;
}
