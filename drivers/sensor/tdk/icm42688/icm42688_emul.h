/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_SENSOR_ICM42688_ICM42688_EMUL_H
#define DRIVERS_SENSOR_ICM42688_ICM42688_EMUL_H

#include <zephyr/drivers/emul.h>

/**
 * @brief Set one or more register values
 *
 * @param target The target emulator to modify
 * @param reg_addr The starting address of the register to modify
 * @param in One or more bytes to write to the registers
 * @param count The number of bytes to write
 */
void icm42688_emul_set_reg(const struct emul *target, uint8_t reg_addr, const uint8_t *in,
			   size_t count);

/**
 * @brief Get the values of one or more register values
 *
 * @param target The target emulator to read
 * @param reg_addr The starting address of the register to read
 * @param out Buffer to write the register values into
 * @param count The number of bytes to read
 */
void icm42688_emul_get_reg(const struct emul *target, uint8_t reg_addr, uint8_t *out, size_t count);

#endif /* DRIVERS_SENSOR_ICM42688_ICM42688_EMUL_H */
