/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_PCA9422_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_PCA9422_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mdf_interface_pca9422 MFD PCA9422 Interface
 * @ingroup mfd_interfaces
 * @{
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>

typedef void (*child_isr_t)(const struct device *dev);

enum {
	PCA9422_DEV_REG,
	PCA9422_DEV_CHG,
	PCA9422_DEV_FG,
	PCA9422_DEV_MAX,
};

/**
 * @brief Set child interrupt handler of pca9422
 *
 * @param dev pca9422 mfd device
 * @param child_dev pca9422 child device
 * @param child_idx index in enum type of child device
 * @param handler interrupt handler of child device
 */
void mfd_pca9422_set_irqhandler(const struct device *dev, const struct device *child_dev,
				uint8_t child_idx, child_isr_t handler);

/**
 * @brief Read multiple registers from pca9422
 *
 * @param dev pca9422 mfd device
 * @param reg Register start address
 * @param value Pointer that stores the received data
 * @param len Number of bytes to read
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_burst_read_dt())
 */
int mfd_pca9422_reg_burst_read(const struct device *dev, uint8_t reg, uint8_t *value, size_t len);

/**
 * @brief Read single register from pca9422
 *
 * @param dev pca9422 mfd device
 * @param reg Register address
 * @param value Pointer that stores the received data
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_reg_read_byte_dt())
 */
int mfd_pca9422_reg_read_byte(const struct device *dev, uint8_t reg, uint8_t *value);

/**
 * @brief Write multiple registers to pca9422
 *
 * @param dev pca9422 mfd device
 * @param reg Register start address
 * @param value Pointer that stores the write data
 * @param len Number of bytes to write
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_burst_write_dt())
 */
int mfd_pca9422_reg_burst_write(const struct device *dev, uint8_t reg, uint8_t *value, size_t len);

/**
 * @brief Write single register to pca9422
 *
 * @param dev pca9422 mfd device
 * @param reg Register address
 * @param value data to write
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_reg_write_byte_dt())
 */
int mfd_pca9422_reg_write_byte(const struct device *dev, uint8_t reg, uint8_t value);

/**
 * @brief Update selected bits in pca9422 register
 *
 * @param dev pca9422 mfd device
 * @param reg Register address
 * @param mask mask of bits to be modified
 * @param value data to write
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_reg_update_byte_dt())
 */
int mfd_pca9422_reg_update_byte(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t value);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_PCA9422_H_ */
