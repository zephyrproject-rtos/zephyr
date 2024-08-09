/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL34X_I2C_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL34X_I2C_H_

#include <stdint.h>

#include <zephyr/device.h>

#define ADXL34X_CONFIG_I2C(i)                                                                      \
	.i2c = I2C_DT_SPEC_INST_GET(i), .bus_init = &adxl34x_i2c_init,                             \
	.bus_write = &adxl34x_i2c_write, .bus_read = &adxl34x_i2c_read,                            \
	.bus_read_buf = &adxl34x_i2c_read_buf

int adxl34x_i2c_init(const struct device *dev);
int adxl34x_i2c_write(const struct device *dev, uint8_t reg_addr, uint8_t reg_data);
int adxl34x_i2c_read(const struct device *dev, uint8_t reg_addr, uint8_t *reg_data);
int adxl34x_i2c_read_buf(const struct device *dev, uint8_t reg_addr, uint8_t *rx_buf, uint8_t size);

#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL34X_I2C_H_ */
