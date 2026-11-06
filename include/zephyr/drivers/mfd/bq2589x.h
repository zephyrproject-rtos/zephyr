/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mfd_interface_bq2589x
 * @brief Header file for the TI BQ2589X MFD driver.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_BQ2589X_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_BQ2589X_H_

#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mfd_interface_bq2589x MFD BQ2589X Interface
 * @ingroup mfd_interfaces
 * @brief TI BQ2589X battery charger interface.
 * @since 4.6
 * @version 0.1.0
 * @{
 */

/**
 * @brief Update selected bits in a BQ2589X register
 *
 * The child devices share some registers, so they do all read-modify-write
 * accesses through this function.
 *
 * @param dev BQ2589X MFD device
 * @param reg Register address
 * @param mask Mask of the bits to update
 * @param val New value of the bits, already shifted into place
 * @return 0 on success, negative errno value on failure (see i2c_reg_update_byte_dt()).
 */
int mfd_bq2589x_reg_update(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_BQ2589X_H_ */
