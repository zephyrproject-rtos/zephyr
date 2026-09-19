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
 * @brief Write a 32-bit register to a TMC device via I2C
 *
 * Performs a write transaction to a TMC motor controller register over I2C.
 * The TMC I2C protocol uses: [REG_ADDR (1 byte)] [DATA (4 bytes MSB first)]
 *
 * @param i2c      Pointer to I2C device tree spec
 * @param reg_addr Register address to write (7-bit, no read/write bit)
 * @param reg_val  32-bit register value to write
 * @return         0 on success, negative errno on failure
 */
int tmc_i2c_write_register(const struct i2c_dt_spec *i2c, uint8_t reg_addr, uint32_t reg_val);

/**
 * @brief Read a 32-bit register from a TMC device via I2C
 *
 * Performs a read transaction from a TMC motor controller register over I2C.
 * The TMC I2C protocol uses:
 *   Write: [REG_ADDR (1 byte)]
 *   Read:  [DATA (4 bytes MSB first)]
 *
 * @param i2c      Pointer to I2C device tree spec
 * @param reg_addr Register address to read (7-bit, no address mask needed)
 * @param reg_val  Pointer to store the 32-bit register value
 * @return         0 on success, negative errno on failure
 */
int tmc_i2c_read_register(const struct i2c_dt_spec *i2c, uint8_t reg_addr, uint32_t *reg_val);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_BUS_I2C_H_ */
