/*
 * Copyright (c) 2021  Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_INA23X_COMMON_H_
#define ZEPHYR_DRIVERS_SENSOR_INA23X_COMMON_H_

#include <zephyr/drivers/i2c.h>

/**
 * @brief Macro used to test if the current's sign bit is set
 */
#define INA23X_CURRENT_SIGN_BIT        BIT(15)

/**
 * @brief Macro used to check if the current's LSB is 1mA
 */
#define INA23X_CURRENT_LSB_1MA         1

int ina23x_reg_read_24(const struct i2c_dt_spec *bus, uint8_t reg, uint32_t *val);
int ina23x_reg_read_16(const struct i2c_dt_spec *bus, uint8_t reg, uint16_t *val);
int ina23x_reg_write(const struct i2c_dt_spec *bus, uint8_t reg, uint16_t val);

#endif /* ZEPHYR_DRIVERS_SENSOR_INA23X_COMMON_H_ */
