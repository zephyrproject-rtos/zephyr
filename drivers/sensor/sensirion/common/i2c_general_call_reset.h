/*
 * Copyright (c) 2026 Sensirion
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef I2C_GENERAL_CALL_RESET_H
#define I2C_GENERAL_CALL_RESET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <zephyr/drivers/i2c.h>

/**
 * Send a general call reset on the i2c bus.
 *
 * This function is used in drivers that implement a call reset.
 *
 * @warning This will reset all attached I2C devices on the bus which support
 *          general call reset.
 *
 * @param i2c_spec  Sensor I2C specification
 *
 * @return  0 on success, an error code otherwise
 */
int sensirion_i2c_general_call_reset(const struct i2c_dt_spec *i2c_spec);

#ifdef __cplusplus
}
#endif

#endif /* I2C_GENERAL_CALL_RESET_H */
