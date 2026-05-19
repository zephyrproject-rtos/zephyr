/**
 * @file drivers/stepper/adi_tmc/bus/include/adi_tmc_i2c.h
 *
 * @brief Private API for Trinamic I2C bus
 *
 */

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

/**
 * @brief TMC I2C INTERFACE
 * @ingroup io_priv_interfaces
 * @{
 *
 */

/**
 * @brief Write a 32-bit register to a TMC device via I2C.
 *
 * @param i2c I2C DT information of the bus.
 * @param reg_addr Register address to write.
 * @param reg_val 32-bit register value to write.
 *
 * @return 0 on success, negative errno on failure.
 */
int tmc_i2c_write_register(const struct i2c_dt_spec *i2c, uint8_t reg_addr, uint32_t reg_val);

/**
 * @brief Read a 32-bit register from a TMC device via I2C.
 *
 * @param i2c I2C DT information of the bus.
 * @param reg_addr Register address to read.
 * @param reg_val Pointer to store the 32-bit register value.
 *
 * @return 0 on success, negative errno on failure.
 */
int tmc_i2c_read_register(const struct i2c_dt_spec *i2c, uint8_t reg_addr, uint32_t *reg_val);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_BUS_I2C_H_ */
