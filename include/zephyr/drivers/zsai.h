/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for ZSAI drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ZSAI_H_
#define ZEPHYR_INCLUDE_DRIVERS_ZSAI_H_

/**
 * @brief ZSAI internal Interface
 * @defgroup zsai_internal_interface ZSAI internal Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/zsai_api.h>
#include <zephyr/drivers/zsai_aux.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @}
 */

/**
 * @brief ZSAI Interface
 * @defgroup zsai_interface ZSAI Interface
 * @ingroup io_interfaces
 * @{
 */

/**
 * @addtogroup zsai_interface
 * @{
 */

/**
 *  @brief  Read data from zsai
 *
 *  All zsai drivers support reads without alignment restrictions on
 *  the read offset, the read size, or the destination address.
 *
 *  @param  dev             : zsai dev
 *  @param  data            : Buffer to store read data
 *  @param  offset          : Offset (byte aligned) to read
 *  @param  size            : Number of bytes to read.
 *
 *  @return  0 on success, negative errno code on fail.
 */
__syscall int zsai_read(const struct device *dev, void *data, size_t offset,
			size_t size);

static inline int z_impl_zsai_read(const struct device *dev, void *data,
				   size_t offset, size_t size)
{
	const struct zsai_driver_api *api = ZSAI_API_PTR(dev);

	return api->read(dev, data, offset, size);
}

/**
 *  @brief  Write buffer into zsai memory.
 *
 *  All zsai drivers support a source buffer located either in RAM or
 *  SoC zsai, without alignment restrictions on the source address.
 *  Write size and offset must be multiples of the minimum write block size
 *  supported by the driver.
 *
 *  Any necessary write protection management is performed by the driver
 *  write implementation itself.
 *
 *  @param  dev             : zsai device
 *  @param  data            : data to write
 *  @param  offset          : starting offset for the write
 *  @param  size            : Number of bytes to write
 *
 *  @return  0 on success, negative errno code on fail.
 */
__syscall int zsai_write(const struct device *dev, const void *data,
			 size_t offset, size_t size);

static inline int z_impl_zsai_write(const struct device *dev, const void *data,
				    size_t offset, size_t size)
{
	const struct zsai_driver_api *api = ZSAI_API_PTR(dev);
	int rc;

	rc = api->write(dev, data, offset, size);

	return rc;
}

/**
 * @brief Erase part of a device.
 *
 * Device needs to support erase procedure, otherwise -ENOTSUP will be returned..
 *
 * @param	dev	: device to erase.
 * @param	range	: page range.
 *
 * @return 0 on success, -ENOTSUP when erase not supported; other negative errno
 * number in case of failure.
 */
__syscall int zsai_erase_range(const struct device *dev,
			       const struct zsai_ioctl_range *range);

static inline int z_impl_zsai_erase_range(const struct device *dev,
					  const struct zsai_ioctl_range *range)
{
#if IS_ENABLED(CONFIG_ZSAI_DEVICE_REQUIRES_ERASE)
	const struct zsai_driver_api *api = ZSAI_API_PTR(dev);

	return api->erase_range(dev, range);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(range);

	return -ENOTSUP;
#endif
}


/**
 * @brief IOCTL invocation at syscall level
 *
 * This can be called by user directly although it has been designed to be called by
 * zsai_ioctl, which will decide whether userspace or syscall space call for
 * IOCTL ID is needed.
 *
 * @param dev		: zsai device
 * @param id		: IOCTL operation ID
 * @param in		: input structure or identifier of sub-operation
 * @param in_out	: additional parameter or pointer to output buffer.
 */
__syscall int zsai_ioctl(const struct device *dev, uint32_t id,
			      const uintptr_t in, uintptr_t in_out);

static inline int z_impl_zsai_ioctl(const struct device *dev, uint32_t id,
					 const uintptr_t in, uintptr_t in_out)
{
	const struct zsai_driver_api *api = ZSAI_API_PTR(dev);

	return api->sys_ioctl(dev, id, in, in_out);
}

int zsai_ioctl(const struct device *dev, uint32_t id, const uintptr_t in,
	       uintptr_t in_out);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/zsai.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_ZSAI_H_ */
