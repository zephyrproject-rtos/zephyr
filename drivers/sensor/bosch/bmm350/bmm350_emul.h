/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMM350_BMM350_EMUL_H_
#define ZEPHYR_DRIVERS_SENSOR_BMM350_BMM350_EMUL_H_

#include <zephyr/drivers/emul.h>
#include <stdint.h>

/**
 * @brief Set the raw (uncompensated) magnetometer sample registers.
 *
 * @param target Pointer to the emulator instance.
 * @param x Raw X-axis value (signed 24-bit range).
 * @param y Raw Y-axis value (signed 24-bit range).
 * @param z Raw Z-axis value (signed 24-bit range).
 */
void bmm350_emul_set_mag_raw(const struct emul *target, int32_t x, int32_t y, int32_t z);

/**
 * @brief Set the raw (uncompensated) temperature sample register.
 *
 * @param target Pointer to the emulator instance.
 * @param temp Raw temperature value (signed 24-bit range).
 */
void bmm350_emul_set_temp_raw(const struct emul *target, int32_t temp);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMM350_BMM350_EMUL_H_ */
