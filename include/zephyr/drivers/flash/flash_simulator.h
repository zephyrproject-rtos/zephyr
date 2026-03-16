/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ZEPHYR_INCLUDE_DRIVERS__FLASH_SIMULATOR_H__
#define __ZEPHYR_INCLUDE_DRIVERS__FLASH_SIMULATOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>
#include <zephyr/types.h>
#include <sys/types.h>

/**
 * @file
 * @brief Flash simulator specific API.
 *
 * Extension for flash simulator.
 */

/**
 * @brief Flash simulator parameters structure
 *
 * Auxiliary structure containing flash simulator parameters to be used by callbacks
 * that can modify the behavior of the flash simulator driver.
 */
struct flash_simulator_params {
	size_t memory_size; /**< @brief The total size of the memory */
	size_t base_offset; /**< @brief The base offset of the flash simulator */
	size_t erase_unit;  /**< @brief The erase unit size */
	size_t prog_unit;   /**< @brief The program unit size */
	bool explicit_erase; /**< @brief Whether explicit erase is required */
	uint8_t erase_value; /**< @brief The value used to represent erased memory */
};

/**
 * @brief Flash simulator write byte callback type
 *
 * Callback used to overwrite single byte write. The callback gets the requested data
 * and offset as the parameter. The offset is the same offset passed to the flash write.
 * The callback must return a value to be written at the specified offset or negative value
 * in case of error.
 *
 * @param dev Flash simulator device pointer.
 * @param offset Offset within the flash simulator memory.
 * @param data Data byte to be written.
 * @return Value to be written at the specified offset or negative value in case of error.
 */
typedef int (*flash_simulator_write_byte_cb_t)(const struct device *dev,
					       off_t offset,
					       uint8_t data);

/**
 * @brief Flash simulator erase unit callback type
 *
 * Callback used to overwrite unit erase operation. The callback gets the unit offset
 * as the parameter. The offset is the same offset passed to the flash erase. The callback
 * must return 0 in case of success or negative value in case of error.
 *
 * @note If this callback is set, the flash simulator driver will not perform any erase operation
 * by itself.
 *
 * @param dev Flash simulator device pointer.
 * @param unit_offset Offset within the flash simulator memory.
 * @return 0 in case of success or negative value in case of error.
 */
typedef int (*flash_simulator_erase_unit_cb_t)(const struct device *dev, off_t unit_offset);

/**
 * @brief Flash simulator callbacks structure
 *
 * Structure containing flash simulator operation callbacks.
 */
struct flash_simulator_cb {
	flash_simulator_write_byte_cb_t write_byte; /**< @brief Byte write callback */
	flash_simulator_erase_unit_cb_t erase_unit; /**< @brief Unit erase callback */
};

/**
 * @brief Get flash simulator parameters
 *
 * This function allows the caller to get the flash simulator parameters used by the driver.
 *
 * @param[in] dev flash simulator device pointer.
 * @return Pointer to the flash simulator parameters structure.
 */
__syscall const struct flash_simulator_params *flash_simulator_get_params(const struct device *dev);

/**
 * @brief Set flash simulator callbacks
 *
 * This function allows the caller to set custom callbacks for byte write and
 * unit erase operations performed by the flash simulator.
 *
 * @param[in] dev flash simulator device pointer.
 * @param[in] cb pointer to the structure containing the callbacks.
 */
__syscall void flash_simulator_set_callbacks(const struct device *dev,
					     const struct flash_simulator_cb *cb);

/**
 * @brief Obtain a pointer to the RAM buffer used but by the simulator
 *
 * This function allows the caller to get the address and size of the RAM buffer
 * in which the flash simulator emulates its flash memory content.
 *
 * @param[in]  dev flash simulator device pointer.
 * @param[out] mock_size size of the ram buffer.
 *
 * @retval pointer to the ram buffer
 */
__syscall void *flash_simulator_get_memory(const struct device *dev,
					   size_t *mock_size);
#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/flash_simulator.h>

#endif /* __ZEPHYR_INCLUDE_DRIVERS__FLASH_SIMULATOR_H__ */
