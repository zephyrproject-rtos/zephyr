/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Alexis Czezar C Torreno
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file stepper_tmcm3216.h
 * @brief Public API for ADI TMCM-3216 stepper motor controller
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_TMCM3216_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_TMCM3216_H_

#include <zephyr/device.h>
#include <zephyr/drivers/stepper.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Motor status structure for TMCM-3216
 */
struct tmcm3216_status {
	/** Position target reached */
	bool position_reached;
	/** Right limit switch active */
	bool right_endstop;
	/** Left limit switch active */
	bool left_endstop;
	/** Motor is currently moving */
	bool is_moving;
	/** Current position */
	int32_t actual_position;
	/** Current velocity */
	int32_t actual_velocity;
};

/**
 * @brief Set maximum velocity for TMCM-3216 motor
 *
 * @param dev Pointer to the stepper device
 * @param velocity Maximum velocity value
 * @return 0 on success, negative errno on failure
 */
int tmcm3216_set_max_velocity(const struct device *dev, uint32_t velocity);

/**
 * @brief Get maximum velocity for TMCM-3216 motor
 *
 * @param dev Pointer to the stepper device
 * @param velocity Pointer to store the maximum velocity value
 * @return 0 on success, negative errno on failure
 */
int tmcm3216_get_max_velocity(const struct device *dev, uint32_t *velocity);

/**
 * @brief Set maximum acceleration for TMCM-3216 motor
 *
 * @param dev Pointer to the stepper device
 * @param acceleration Maximum acceleration value
 * @return 0 on success, negative errno on failure
 */
int tmcm3216_set_max_acceleration(const struct device *dev, uint32_t acceleration);

/**
 * @brief Get actual velocity of TMCM-3216 motor
 *
 * @param dev Pointer to the stepper device
 * @param velocity Pointer to store the actual velocity value
 * @return 0 on success, negative errno on failure
 */
int tmcm3216_get_actual_velocity(const struct device *dev, int32_t *velocity);

/**
 * @brief Get motor status of TMCM-3216
 *
 * @param dev Pointer to the stepper device
 * @param status Pointer to store the motor status
 * @return 0 on success, negative errno on failure
 */
int tmcm3216_get_status(const struct device *dev, struct tmcm3216_status *status);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_TMCM3216_H_ */
