/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

#include <stdint.h>
#include <stddef.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*flash_api_read)(struct device *dev, size_t offset,
			      void *data, size_t len);
typedef int (*flash_api_write)(struct device *dev, size_t offset,
			       const void *data, size_t len);
typedef int (*flash_api_erase)(struct device *dev, size_t offset,
			       size_t size);
typedef int (*flash_api_write_protection)(struct device *dev, bool enable);

struct flash_driver_api {
	flash_api_read read;
	flash_api_write write;
	flash_api_erase erase;
	flash_api_write_protection write_protection;
};

/**
 *  @brief  Read data from flash
 *
 *  @param  dev             : flash dev
 *  @param  offset          : Offset (byte aligned) to read
 *  @param  data            : Buffer to store read data
 *  @param  len             : Number of bytes to read.
 *
 *  @return  DEV_OK on success else DEV_* code
 */
static inline int flash_read(struct device *dev, size_t offset, void *data,
			     size_t len)
{
	struct flash_driver_api *api = (struct flash_driver_api *)dev->driver_api;

	return api->read(dev, offset, data, len);
}

/**
 *  @brief  Write buffer into flash memory.
 *
 *  @param  dev             : flash device
 *  @param  offset          : starting offset for the write
 *  @param  data            : data to write
 *  @param  len             : Number of bytes to write
 *
 *  @return  DEV_OK on success else DEV_* code
 */
static inline int flash_write(struct device *dev, size_t offset,
			      const void *data, size_t len)
{
	struct flash_driver_api *api = (struct flash_driver_api *)dev->driver_api;

	return api->write(dev, offset, data, len);
}

/**
 *  @brief  Erase part or all of a flash memory
 *
 *  Acceptable values of erase size are subject to hardware-specific
 *  multiples of sector size.
 *
 *  @param  dev             : flash device
 *  @param  offset          : erase area starting offset
 *  @param  size            : size of area to be erased
 *
 *  @return  DEV_OK on success else DEV_* code
 */
static inline int flash_erase(struct device *dev, size_t offset, size_t size)
{
	struct flash_driver_api *api = (struct flash_driver_api *)dev->driver_api;

	return api->erase(dev, offset, size);
}

/**
 *  @brief  Enable or disable write protection for a flash memory
 *
 *  @param  dev             : flash device
 *  @param  enable          : enable or disable flash write protection
 *
 *  @return  DEV_OK on success else DEV_* code
 */
static inline int flash_write_protection_set(struct device *dev, bool enable)
{
	struct flash_driver_api *api = (struct flash_driver_api *)dev->driver_api;

	return api->write_protection(dev, enable);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FLASH_H_ */
