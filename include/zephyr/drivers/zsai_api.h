/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for ZSAI drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ZSAI_API_H_
#define ZEPHYR_INCLUDE_DRIVERS_ZSAI_API_H_

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
#include <zephyr/drivers/zsai_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @}
 */

/**
 * @addtogroup zsai_internal_interface
 * @{
 */

/**
 * @brief Device read
 *
 * Read data from device to buffer.
 *
 * @param	dev		: device to read from.
 * @param	data		: pointer to buffer for data,
 * @param	offset		: offset on device to start write at,
 * @param	size		: size of data to write.
 *
 * @return 0 on success, -ENOTSUP when not supported. Negative errno type code in
 * case of other error.
 */
typedef int (*zsai_api_read)(const struct device *dev, void *data, size_t offset,
			     size_t size);
/**
 * @brief Device write
 *
 * @note Any necessary write protection management must be performed by
 * the driver, with the driver responsible for ensuring the "write-protect"
 * after the operation completes (successfully or not) matches the write-protect
 * state when the operation was started.
 *
 * @param	dev		: device to write to.
 * @param	data		: pointer to buffer with data.
 * @param	offset		: offset on device to start write at.
 * @param	size		: size of data to write.
 *
 * @note @p offset and @p size should be aligned to write-block-size of device.
 *
 * @return 0 on success, -ENOTSUP when not supported. Negative errno type code in
 * case of other error.
 */
typedef int (*zsai_api_write)(const struct device *dev, const void *data,
			      size_t offset, size_t size);

/**
 * @brief Erase device range
 *
 * Depending on device erase may require page alignment.
 *
 * @param	dev		: device to write to.
 * @param	range		: pointer to range description.
 *
 * @return 0 on success, -ENOTSUP when not supported. Negative errno type code in
 * case of other error.
 */
typedef int (*zsai_api_erase_range)(const struct device *dev,
				    const struct zsai_ioctl_range *range);

/**
 * @brief IOCTL handler requiring syscall level
 *
 * @note The system and user level IOCTL handlers use the same IDs for
 * IOCTL operations. Internal implementation should reject user level
 * IOCTL operation, by returning -ENOTSUP, when requested in syscall level handler.
 *
 * @param	dev		: device to perform operation on.
 * @param	id		: IOCTL operation ID
 * @param	in		: input structure or ID of sub-operation
 * @param	in_out		: additional parameter or pointer to output buffer.
 *
 * @return 0 on success, -ENOTSUP when not supported. Negative errno type code in
 * case of other error.
 */
typedef int (*zsai_api_sys_ioctl)(const struct device *dev, uint32_t id, const uintptr_t in,
				  uintptr_t in_out);

struct zsai_infoword {
	uint32_t erase_supported	: 1;
	uint32_t erase_required		: 1;
	uint32_t erase_bit_value	: 1;
	uint32_t uniform_page_size	: 1;
	uint32_t write_block_size	: 4;
};

struct zsai_device_generic_config {
	struct zsai_infoword infoword;
};

__subsystem struct zsai_driver_api {
	zsai_api_read read;
	zsai_api_write write;
#if IS_ENABLED(CONFIG_ZSAI_DEVICE_REQUIRES_ERASE)
	zsai_api_erase_range erase_range;
#endif
	zsai_api_sys_ioctl sys_ioctl;
};

#define ZSAI_API_PTR(dev) ((const struct zsai_driver_api *)dev->api)
#define ZSAI_DEV_CONFIG(dev) ((const struct zsai_device_generic_config *)dev->config)
#define ZSAI_DEV_INFOWORD(dev) (ZSAI_DEV_CONFIG(dev)->infoword)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_ZSAI_API_H_ */
