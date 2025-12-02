/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_MC146818_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_MC146818_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>

/**
 * @brief Read the value at the given offset in standard memory bank.
 *
 * @param dev mc146818 mfd device
 * @param offset memory offset to be read.
 * @retval value at the offset
 */
uint8_t mfd_mc146818_std_read(const struct device *dev, uint8_t offset);

/**
 * @brief Write the value at the given offset in standard memory bank.
 *
 * @param dev mc146818 mfd device
 * @param offset memory offset to be written.
 * @param value to be written at the offset
 */
void mfd_mc146818_std_write(const struct device *dev, uint8_t offset, uint8_t value);

/**
 * @brief Read the value at the given offset in extended memory bank.
 *
 * @param dev mc146818 mfd device
 * @param offset memory offset to be read.
 * @retval value at the offset
 */
uint8_t mfd_mc146818_ext_read(const struct device *dev, uint8_t offset);

/**
 * @brief Write the value at the given offset in extended memory bank.
 *
 * @param dev mc146818 mfd device
 * @param offset memory offset to be written.
 * @param value to be written at the offset
 */
void mfd_mc146818_ext_write(const struct device *dev, uint8_t offset, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_MC146818_H_ */
