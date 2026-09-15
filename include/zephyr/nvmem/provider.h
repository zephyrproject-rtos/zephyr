/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup nvmem_provider_interface
 * @brief NVMEM provider driver API.
 */

#ifndef ZEPHYR_INCLUDE_NVMEM_PROVIDER_H_
#define ZEPHYR_INCLUDE_NVMEM_PROVIDER_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nvmem_provider_interface NVMEM providers
 * @brief Device driver API for native NVMEM cell providers.
 *
 * Devices whose purpose is to expose NVMEM cells can implement this driver
 * API instead of one of the memory device APIs the NVMEM subsystem also
 * dispatches to (BBRAM, EEPROM, flash and OTP).
 *
 * The cell address passed to the driver is the cell's devicetree reg address
 * and is opaque to the NVMEM subsystem; it does not have to be a byte offset
 * into a flat memory space. The byte offset within the cell is passed
 * separately and is never folded into the cell address.
 *
 * @since 4.5
 * @version 0.1.0
 * @ingroup nvmem_interface
 * @{
 */

/**
 * @def_driverbackendgroup{NVMEM provider,nvmem_provider_interface}
 * @{
 */

/**
 * @brief Callback API upon reading from an NVMEM cell.
 * See @a nvmem_cell_read() for argument description.
 *
 * @param dev NVMEM provider device instance.
 * @param addr Address of the NVMEM cell, as given by its devicetree reg address.
 * @param off Byte offset within the NVMEM cell.
 * @param data Buffer to store the read data.
 * @param len Number of bytes to read.
 */
typedef int (*nvmem_api_read)(const struct device *dev, uint32_t addr, size_t off, void *data,
			      size_t len);

/**
 * @brief Callback API upon writing to an NVMEM cell.
 * See @a nvmem_cell_write() for argument description.
 *
 * @param dev NVMEM provider device instance.
 * @param addr Address of the NVMEM cell, as given by its devicetree reg address.
 * @param off Byte offset within the NVMEM cell.
 * @param data Buffer containing the data to write.
 * @param len Number of bytes to write.
 */
typedef int (*nvmem_api_write)(const struct device *dev, uint32_t addr, size_t off,
			       const void *data, size_t len);

/** @driver_ops{NVMEM provider} */
__subsystem struct nvmem_driver_api {
	/** @driver_ops_mandatory @copybrief nvmem_api_read */
	nvmem_api_read read;
	/**
	 * @driver_ops_optional @copybrief nvmem_api_write
	 * May be NULL for read-only providers, writes then fail with -ENOSYS.
	 */
	nvmem_api_write write;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NVMEM_PROVIDER_H_ */
