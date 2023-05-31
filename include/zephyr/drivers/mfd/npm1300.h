/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_NPM1300_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_NPM1300_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mdf_interface_npm1300 MFD NPM1300 Interface
 * @ingroup mfd_interfaces
 * @{
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>

/**
 * @brief Read multiple registers from npm1300
 *
 * @param dev npm1300 mfd device
 * @param base Register base address (bits 15..8 of 16-bit address)
 * @param offset Register offset address (bits 7..0 of 16-bit address)
 * @param data Pointer to buffer for received data
 * @param len Number of bytes to read
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_read_dt())
 */
int mfd_npm1300_reg_read_burst(const struct device *dev, uint8_t base, uint8_t offset,
			       void *data, size_t len);

/**
 * @brief Read single register from npm1300
 *
 * @param dev npm1300 mfd device
 * @param base Register base address (bits 15..8 of 16-bit address)
 * @param offset Register offset address (bits 7..0 of 16-bit address)
 * @param data Pointer to buffer for received data
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_read_dt())
 */
int mfd_npm1300_reg_read(const struct device *dev, uint8_t base, uint8_t offset, uint8_t *data);

/**
 * @brief Write single register to npm1300
 *
 * @param dev npm1300 mfd device
 * @param base Register base address (bits 15..8 of 16-bit address)
 * @param offset Register offset address (bits 7..0 of 16-bit address)
 * @param data data to write
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_dt())
 */
int mfd_npm1300_reg_write(const struct device *dev, uint8_t base, uint8_t offset, uint8_t data);

/**
 * @brief Write two registers to npm1300
 *
 * @param dev npm1300 mfd device
 * @param base Register base address (bits 15..8 of 16-bit address)
 * @param offset Register offset address (bits 7..0 of 16-bit address)
 * @param data1 first byte of data to write
 * @param data2 second byte of data to write
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_dt())
 */
int mfd_npm1300_reg_write2(const struct device *dev, uint8_t base, uint8_t offset, uint8_t data1,
			   uint8_t data2);

/**
 * @brief Update selected bits in npm1300 register
 *
 * @param dev npm1300 mfd device
 * @param base Register base address (bits 15..8 of 16-bit address)
 * @param offset Register offset address (bits 7..0 of 16-bit address)
 * @param data data to write
 * @param mask mask of bits to be modified
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_read_dt(), i2c_write_dt())
 */
int mfd_npm1300_reg_update(const struct device *dev, uint8_t base, uint8_t offset, uint8_t data,
			   uint8_t mask);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_NPM1300_H_ */
