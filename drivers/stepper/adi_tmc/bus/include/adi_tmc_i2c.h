/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Cherrence Sarip <Cherrence.sarip@analog.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_BUS_I2C_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_BUS_I2C_H_

#include <zephyr/drivers/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

int tmc_i2c_write_register(const struct i2c_dt_spec *i2c, uint8_t reg_addr, uint32_t reg_val);
int tmc_i2c_read_register(const struct i2c_dt_spec *i2c, uint8_t reg_addr, uint32_t *reg_val);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_BUS_I2C_H_ */
