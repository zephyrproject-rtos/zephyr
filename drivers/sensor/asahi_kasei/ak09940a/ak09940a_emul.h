/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ASAHI_KASEI_AK09940A_AK09940A_EMUL_H_
#define ZEPHYR_DRIVERS_SENSOR_ASAHI_KASEI_AK09940A_AK09940A_EMUL_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/drivers/emul.h>

/**
 * @brief Set one or more register values
 *
 * @param target The target emulator to modify
 * @param reg_addr The starting address of the registers to modify
 * @param val One or more bytes to write to the registers
 * @param count The number of bytes to write
 */
void ak09940a_emul_set_reg(const struct emul *target, uint8_t reg_addr, const uint8_t *val,
			   size_t count);

/**
 * @brief Get the values of one or more registers
 *
 * @param target The target emulator to read
 * @param reg_addr The starting address of the registers to read
 * @param val Buffer to write the register values into
 * @param count The number of bytes to read
 */
void ak09940a_emul_get_reg(const struct emul *target, uint8_t reg_addr, uint8_t *val, size_t count);

/**
 * @brief Reset the emulator registers to their power-on values
 *
 * @param target The target emulator to reset
 */
void ak09940a_emul_reset(const struct emul *target);

#endif /* ZEPHYR_DRIVERS_SENSOR_ASAHI_KASEI_AK09940A_AK09940A_EMUL_H_ */
