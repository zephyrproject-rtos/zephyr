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

#ifndef ZEPHYR_INCLUDE_DRIVERS_FLASH_H_
#define ZEPHYR_INCLUDE_DRIVERS_FLASH_H_

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

/**
 * Flash memory parameters. Contents of this structure suppose to be
 * filled in during flash device initialization and stay constant
 * through a runtime.
 */
struct flash_parameters {
	const size_t write_block_size;
	uint8_t erase_value; /* Byte value of erased flash */
};

typedef int (*flash_api_read)(const struct device *dev, off_t offset,
			      void *data,
			      size_t len);
typedef int (*flash_api_write)(const struct device *dev, off_t offset,
			       const void *data, size_t len);
typedef int (*flash_api_erase)(const struct device *dev, off_t offset,
			       size_t size);
typedef int (*flash_api_write_protection)(const struct device *dev,
					  bool enable);
typedef const struct flash_parameters* (*flash_api_get_parameters)(const struct device *dev);

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
typedef void (*flash_api_pages_layout)(const struct device *dev,
				       const struct flash_pages_layout **layout,
				       size_t *layout_size);
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

typedef int (*flash_api_sfdp_read)(const struct device *dev, off_t offset,
				   void *data, size_t len);
typedef int (*flash_api_read_jedec_id)(const struct device *dev, uint8_t *id);

__subsystem struct flash_driver_api {
	flash_api_read read;
	flash_api_write write;
	flash_api_erase erase;
	flash_api_write_protection write_protection;
	flash_api_get_parameters get_parameters;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	flash_api_pages_layout page_layout;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
#if defined(CONFIG_FLASH_JESD216_API)
	flash_api_sfdp_read sfdp_read;
	flash_api_read_jedec_id read_jedec_id;
#endif /* CONFIG_FLASH_JESD216_API */
};

/**
 *  @brief  Read data from flash
 *
 *  All flash drivers support reads without alignment restrictions on
 *  the read offset, the read size, or the destination address.
 *
 *  @param  dev             : flash dev
 *  @param  offset          : Offset (byte aligned) to read
 *  @param  data            : Buffer to store read data
 *  @param  len             : Number of bytes to read.
 *
 *  @return  0 on success, negative errno code on fail.
 */
__syscall int flash_read(const struct device *dev, off_t offset, void *data,
			 size_t len);

static inline int z_impl_flash_read(const struct device *dev, off_t offset,
				    void *data,
				    size_t len)
{
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;

	return api->read(dev, offset, data, len);
}

/**
 *  @brief  Write buffer into flash memory.
 *
 *  All flash drivers support a source buffer located either in RAM or
 *  SoC flash, without alignment restrictions on the source address.
 *  Write size and offset must be multiples of the minimum write block size
 *  supported by the driver.
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
__syscall int flash_write(const struct device *dev, off_t offset,
			  const void *data,
			  size_t len);

static inline int z_impl_flash_write(const struct device *dev, off_t offset,
				     const void *data, size_t len)
{
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;

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
__syscall int flash_erase(const struct device *dev, off_t offset, size_t size);

static inline int z_impl_flash_erase(const struct device *dev, off_t offset,
				     size_t size)
{
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;

	return api->erase(dev, offset, size);
}

/**
 *  @brief  Enable or disable write protection for a flash memory
 *
 *  This API is required to be called before the invocation of write or erase
 *  API. Any calls to flash_write() or flash_erase() that do not first disable
 *  write protection using this function result in undefined behavior.
 *  Usage Example:
 *  @code
 *   flash_write_protection_set(flash_dev, false);
 *   flash_erase(flash_dev, page_offset, page_size);
 *
 *   flash_write_protection_set(flash_dev, false);
 *   flash_write(flash_dev, offset, data, sizeof(data));
 *
 *   flash_write_protection_set(flash_dev, true); // enable is recommended
 *  @endcode
 *
 *  Please note that on some flash components, the write protection is
 *  automatically turned on again by the device after the completion of each
 *  call to flash_write or flash_erase(). Therefore, portable programs must
 *  disable write protection using this function before each call to
 *  flash_erase() or flash_write().
 *
 *  For some flash devices, this function may implement a no-operation, as some
 *  flash hardware does not support write protection, or may not support it in
 *  a manner that is compatible with this API. For these drivers, this function
 *  always returns success.
 *
 *  @param  dev             : flash device
 *  @param  enable          : enable or disable flash write protection
 *
 *  @return  0 on success, negative errno code on fail.
 */
__syscall int flash_write_protection_set(const struct device *dev,
					 bool enable);

static inline int z_impl_flash_write_protection_set(const struct device *dev,
						    bool enable)
{
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;

	return api->write_protection(dev, enable);
}

struct flash_pages_info {
	off_t start_offset; /* offset from the base of flash address */
	size_t size;
	uint32_t index;
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
__syscall int flash_get_page_info_by_offs(const struct device *dev,
					  off_t offset,
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
__syscall int flash_get_page_info_by_idx(const struct device *dev,
					 uint32_t page_index,
					 struct flash_pages_info *info);

/**
 *  @brief  Get the total number of flash pages.
 *
 *  @param  dev flash device
 *
 *  @return  Number of flash pages.
 */
__syscall size_t flash_get_page_count(const struct device *dev);

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
void flash_page_foreach(const struct device *dev, flash_page_cb cb,
			void *data);
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#if defined(CONFIG_FLASH_JESD216_API)
/**
 * @brief Read data from Serial Flash Discoverable Parameters
 *
 * This routine reads data from a serial flash device compatible with
 * the JEDEC JESD216 standard for encoding flash memory
 * characteristics.
 *
 * Availability of this API is conditional on selecting
 * @c CONFIG_FLASH_JESD216_API and support of that functionality in
 * the driver underlying @p dev.
 *
 * @param dev device from which parameters will be read
 * @param offset address within the SFDP region containing data of interest
 * @param data where the data to be read will be placed
 * @param len the number of bytes of data to be read
 *
 * @retval 0 on success
 * @retval -ENOTSUP if the flash driver does not support SFDP access
 * @retval negative values for other errors.
 */
__syscall int flash_sfdp_read(const struct device *dev, off_t offset,
			      void *data, size_t len);

static inline int z_impl_flash_sfdp_read(const struct device *dev,
					 off_t offset,
					 void *data, size_t len)
{
	int rv = -ENOTSUP;
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;

	if (api->sfdp_read != NULL) {
		rv = api->sfdp_read(dev, offset, data, len);
	}
	return rv;
}

/**
 * @brief Read the JEDEC ID from a compatible flash device.
 *
 * @param dev device from which id will be read
 * @param id pointer to a buffer of at least 3 bytes into which id
 * will be stored
 *
 * @retval 0 on successful store of 3-byte JEDEC id
 * @retval -ENOTSUP if flash driver doesn't support this function
 * @retval negative values for other errors
 */
__syscall int flash_read_jedec_id(const struct device *dev, uint8_t *id);

static inline int z_impl_flash_read_jedec_id(const struct device *dev,
					     uint8_t *id)
{
	int rv = -ENOTSUP;
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;

	if (api->read_jedec_id != NULL) {
		rv = api->read_jedec_id(dev, id);
	}
	return rv;
}
#endif /* CONFIG_FLASH_JESD216_API */

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
__syscall size_t flash_get_write_block_size(const struct device *dev);

static inline size_t z_impl_flash_get_write_block_size(const struct device *dev)
{
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;

	return api->get_parameters(dev)->write_block_size;
}


/**
 *  @brief  Get pointer to flash_parameters structure
 *
 *  Returned pointer points to a structure that should be considered
 *  constant through a runtime, regardless if it is defined in RAM or
 *  Flash.
 *  Developer is free to cache the structure pointer or copy its contents.
 *
 *  @return pointer to flash_parameters structure characteristic for
 *          the device.
 */
__syscall const struct flash_parameters *flash_get_parameters(const struct device *dev);

static inline const struct flash_parameters *z_impl_flash_get_parameters(const struct device *dev)
{
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;

	return api->get_parameters(dev);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/flash.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_FLASH_H_ */
