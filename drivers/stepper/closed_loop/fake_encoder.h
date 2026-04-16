/*
 * Copyright (c) 2026 Contributors to the Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_CLOSED_LOOP_FAKE_ENCODER_H_
#define ZEPHYR_DRIVERS_STEPPER_CLOSED_LOOP_FAKE_ENCODER_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set the rotation value reported by the fake encoder.
 *
 * @param dev Pointer to the fake encoder device.
 * @param val Sensor value (val1 = degrees, val2 = microdegrees).
 */
void fake_encoder_set_rotation(const struct device *dev, const struct sensor_value *val);

/**
 * @brief Set the rotation value by whole degrees.
 *
 * @param dev Pointer to the fake encoder device.
 * @param degrees Rotation in degrees.
 */
void fake_encoder_set_rotation_degrees(const struct device *dev, int32_t degrees);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_CLOSED_LOOP_FAKE_ENCODER_H_ */
