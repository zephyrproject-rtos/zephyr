/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LTC4286_LTC4286_EMUL_H_
#define ZEPHYR_DRIVERS_SENSOR_LTC4286_LTC4286_EMUL_H_

#include <stdint.h>

/**
 * @brief Set a register value in the emulated LTC4286
 *
 * @param emul_data Emulator data pointer
 * @param reg_addr Register address
 * @param value Value to set
 * @return 0 on success, negative errno on error
 */
int ltc4286_emul_set_register(void *emul_data, uint8_t reg_addr, uint16_t value);

/**
 * @brief Get a register value from the emulated LTC4286
 *
 * @param emul_data Emulator data pointer
 * @param reg_addr Register address
 * @param value Pointer to store the value
 * @return 0 on success, negative errno on error
 */
int ltc4286_emul_get_register(void *emul_data, uint8_t reg_addr, uint16_t *value);

/**
 * @brief Reset the emulated LTC4286 to default values
 *
 * @param target Emulator target
 */
void ltc4286_emul_reset(const struct emul *target);

#endif /* ZEPHYR_DRIVERS_SENSOR_LTC4286_LTC4286_EMUL_H_ */
