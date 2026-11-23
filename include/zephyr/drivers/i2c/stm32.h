/*
 * Copyright (c) 2023 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_STM32_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_STM32_H_

#include <zephyr/device.h>

enum i2c_stm32_mode {
	I2CSTM32MODE_I2C,
	I2CSTM32MODE_SMBUSHOST,
	I2CSTM32MODE_SMBUSDEVICE,
	I2CSTM32MODE_SMBUSDEVICEARP,
};

void i2c_stm32_set_smbus_mode(const struct device *dev, enum i2c_stm32_mode mode);

#ifdef CONFIG_SMBUS_STM32_SMBALERT
typedef void (*i2c_stm32_smbalert_cb_func_t)(const struct device *dev);

void i2c_stm32_smbalert_set_callback(const struct device *dev, i2c_stm32_smbalert_cb_func_t func,
				     const struct device *cb_dev);
void i2c_stm32_smbalert_enable(const struct device *dev);
void i2c_stm32_smbalert_disable(const struct device *dev);
#endif /* CONFIG_SMBUS_STM32_SMBALERT */

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_STM32_H_ */
