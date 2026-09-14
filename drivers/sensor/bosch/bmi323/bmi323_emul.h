/*
 * Copyright (c) 2026 Chaogui Deng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMI323_BMI323_EMUL_H_
#define ZEPHYR_DRIVERS_SENSOR_BMI323_BMI323_EMUL_H_

#include <stdint.h>

#include <zephyr/drivers/emul.h>

/**
 * @brief Set the raw accelerometer sample registers.
 *
 * @param target Pointer to the emulator instance.
 * @param x Raw X-axis sample.
 * @param y Raw Y-axis sample.
 * @param z Raw Z-axis sample.
 */
void bmi323_emul_set_accel_raw(const struct emul *target, int16_t x, int16_t y, int16_t z);

/**
 * @brief Set the raw gyroscope sample registers.
 *
 * @param target Pointer to the emulator instance.
 * @param x Raw X-axis sample.
 * @param y Raw Y-axis sample.
 * @param z Raw Z-axis sample.
 */
void bmi323_emul_set_gyro_raw(const struct emul *target, int16_t x, int16_t y, int16_t z);

/**
 * @brief Set the raw die-temperature sample register.
 *
 * @param target Pointer to the emulator instance.
 * @param temperature Raw temperature sample.
 */
void bmi323_emul_set_temperature_raw(const struct emul *target, int16_t temperature);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMI323_BMI323_EMUL_H_ */
