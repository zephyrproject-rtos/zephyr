/*
 * Copyright (c) 2024 Gergo Vari <work@gergovari.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_DS3231_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_DS3231_H_

#include <zephyr/drivers/i2c.h>

/**
 * @brief Get specified number of registers from an I2C device
 * starting at the given register address.
 *
 * @param dev ds3231 mfd device
 * @param start_reg The register address to start at.
 * @param buf The buffer array pointer to store results in.
 * @param buf_size The amount of register values to return.
 * @retval 0 on success
 * @retval -errno in case of any bus error
 */
int mfd_ds3231_i2c_get_registers(const struct device *dev, uint8_t start_reg, uint8_t *buf,
				 const size_t buf_size);

/**
 * @brief Set a register on an I2C device at the given register address.
 *
 * @param dev ds3231 mfd device
 * @param start_reg The register address to set.
 * @param buf The value to write to the given address.
 * @param buf_size The size of the buffer to be written to the given address.
 * @retval 0 on success
 * @retval -errno in case of any bus error
 */
int mfd_ds3231_i2c_set_registers(const struct device *dev, uint8_t start_reg, const uint8_t *buf,
				 const size_t buf_size);

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_DS3231_H_ */
