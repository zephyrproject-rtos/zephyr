/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for FLASH drivers
 */

#ifndef __FLASH_H__
#define __FLASH_H__

/**
 * @brief FLASH Interface
 * @defgroup flash_interface FLASH Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
struct flash_pages_layout {
	size_t pages_count; /* count of pages sequence of the same size */
	size_t pages_size;
};
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

typedef int (*flash_api_read)(struct device *dev, off_t offset, void *data,
			      size_t len);
typedef int (*flash_api_write)(struct device *dev, off_t offset,
			       const void *data, size_t len);
typedef int (*flash_api_erase)(struct device *dev, off_t offset, size_t size);
typedef int (*flash_api_write_protection)(struct device *dev, bool enable);

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
/**
 * @brief Retrieve a flash device's layout.
 *
 * A flash device layout is a run-length encoded description of the
 * pages on the device. (Here, "page" means the smallest erasable
 * area on the flash device.)
 *
 * For flash memories which have uniform page sizes, this routine
 * returns an array of length 1, which specifies the page size and
 * number of pages in the memory.
 *
 * Layouts for flash memories with nonuniform page sizes will be
 * returned as an array with multiple elements, each of which
 * describes a group of pages that all have the same size. In this
 * case, the sequence of array elements specifies the order in which
 * these groups occur on the device.
 *
 * @param dev         Flash device whose layout to retrieve.
 * @param layout      The flash layout will be returned in this argument.
 * @param layout_size The number of elements in the returned layout.
 */
typedef void (*flash_api_pages_layout)(struct device *dev,
				       const struct flash_pages_layout **layout,
				       size_t *layout_size);
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

struct flash_driver_api {
	flash_api_read read;
	flash_api_write write;
	flash_api_erase erase;
	flash_api_write_protection write_protection;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	flash_api_pages_layout page_layout;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
	const size_t write_block_size;
};

/**
 *  @brief  Read data from flash
 *
 *  @param  dev             : flash dev
 *  @param  offset          : Offset (byte aligned) to read
 *  @param  data            : Buffer to store read data
 *  @param  len             : Number of bytes to read.
 *
 *  @return  0 on success, negative errno code on fail.
 */
__syscall int flash_read(struct device *dev, off_t offset, void *data,
			 size_t len);

static inline int _impl_flash_read(struct device *dev, off_t offset, void *data,
			     size_t len)
{
	const struct flash_driver_api *api = dev->driver_api;

	return api->read(dev, offset, data, len);
}

/**
 *  @brief  Write buffer into flash memory.
 *
 *  Prior to the invocation of this API, the flash_write_protection_set needs
 *  to be called first to disable the write protection.
 *
 *  @param  dev             : flash device
 *  @param  offset          : starting offset for the write
 *  @param  data            : data to write
 *  @param  len             : Number of bytes to write
 *
 *  @return  0 on success, negative errno code on fail.
 */
__syscall int flash_write(struct device *dev, off_t offset, const void *data,
			  size_t len);

static inline int _impl_flash_write(struct device *dev, off_t offset,
				    const void *data, size_t len)
{
	const struct flash_driver_api *api = dev->driver_api;

	return api->write(dev, offset, data, len);
}

/**
 *  @brief  Erase part or all of a flash memory
 *
 *  Acceptable values of erase size and offset are subject to
 *  hardware-specific multiples of page size and offset. Please check
 *  the API implemented by the underlying sub driver, for example by
 *  using flash_get_page_info_by_offs() if that is supported by your
 *  flash driver.
 *
 *  Prior to the invocation of this API, the flash_write_protection_set needs
 *  to be called first to disable the write protection.
 *
 *  @param  dev             : flash device
 *  @param  offset          : erase area starting offset
 *  @param  size            : size of area to be erased
 *
 *  @return  0 on success, negative errno code on fail.
 *
 *  @see flash_get_page_info_by_offs()
 *  @see flash_get_page_info_by_idx()
 */
__syscall int flash_erase(struct device *dev, off_t offset, size_t size);

static inline int _impl_flash_erase(struct device *dev, off_t offset,
				    size_t size)
{
	const struct flash_driver_api *api = dev->driver_api;

	return api->erase(dev, offset, size);
}

/**
 *  @brief  Enable or disable write protection for a flash memory
 *
 *  This API is required to be called before the invocation of write or erase
 *  API. Please note that on some flash components, the write protection is
 *  automatically turned on again by the device after the completion of each
 *  write or erase calls. Therefore, on those flash parts, write protection needs
 *  to be disabled before each invocation of the write or erase API. Please refer
 *  to the sub-driver API or the data sheet of the flash component to get details
 *  on the write protection behavior.
 *
 *  @param  dev             : flash device
 *  @param  enable          : enable or disable flash write protection
 *
 *  @return  0 on success, negative errno code on fail.
 */
__syscall int flash_write_protection_set(struct device *dev, bool enable);

static inline int _impl_flash_write_protection_set(struct device *dev,
						   bool enable)
{
	const struct flash_driver_api *api = dev->driver_api;

	return api->write_protection(dev, enable);
}

struct flash_pages_info {
	off_t start_offset; /* offset from the base of flash address */
	size_t size;
	u32_t index;
};

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
/**
 *  @brief  Get the size and start offset of flash page at certain flash offset.
 *
 *  @param  dev flash device
 *  @param  offset Offset within the page
 *  @param  info Page Info structure to be filled
 *
 *  @return  0 on success, -EINVAL if page of the offset doesn't exist.
 */
__syscall int flash_get_page_info_by_offs(struct device *dev, off_t offset,
					  struct flash_pages_info *info);

/**
 *  @brief  Get the size and start offset of flash page of certain index.
 *
 *  @param  dev flash device
 *  @param  page_index Index of the page. Index are counted from 0.
 *  @param  info Page Info structure to be filled
 *
 *  @return  0 on success, -EINVAL  if page of the index doesn't exist.
 */
__syscall int flash_get_page_info_by_idx(struct device *dev, u32_t page_index,
					 struct flash_pages_info *info);

/**
 *  @brief  Get the total number of flash pages.
 *
 *  @param  dev flash device
 *
 *  @return  Number of flash pages.
 */
__syscall size_t flash_get_page_count(struct device *dev);

/**
 * @brief Callback type for iterating over flash pages present on a device.
 *
 * The callback should return true to continue iterating, and false to halt.
 *
 * @param info Information for current page
 * @param data Private data for callback
 * @return True to continue iteration, false to halt iteration.
 * @see flash_page_foreach()
 */
typedef bool (*flash_page_cb)(const struct flash_pages_info *info, void *data);

/**
 * @brief Iterate over all flash pages on a device
 *
 * This routine iterates over all flash pages on the given device,
 * ordered by increasing start offset. For each page, it invokes the
 * given callback, passing it the page's information and a private
 * data object.
 *
 * @param dev Device whose pages to iterate over
 * @param cb Callback to invoke for each flash page
 * @param data Private data for callback function
 */
void flash_page_foreach(struct device *dev, flash_page_cb cb, void *data);
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

/**
 *  @brief  Get the minimum write block size supported by the driver
 *
 *  The write block size supported by the driver might differ from the write
 *  block size of memory used because the driver might implements write-modify
 *  algorithm.
 *
 *  @param  dev flash device
 *
 *  @return  write block size in bytes.
 */
__syscall size_t flash_get_write_block_size(struct device *dev);

static inline size_t _impl_flash_get_write_block_size(struct device *dev)
{
	const struct flash_driver_api *api = dev->driver_api;

	return api->write_block_size;
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/flash.h>

#endif /* _FLASH_H_ */
