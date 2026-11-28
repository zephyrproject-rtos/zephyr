/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file stepper_tmc5241.h
 * @brief Public API for ADI TMC5241 stepper motor controller specific functions
 * @ingroup tmc5241_stepper_ctrl
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_TMC5241_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_TMC5241_H_

/**
 * @brief ADI TMC5241 Stepper Controller
 * @defgroup tmc5241_stepper_ctrl TMC5241 Stepper Controller
 * @ingroup stepper_interface_ext
 * @{
 */

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure TMC5241 Stepper StallGuard settings. TMC5241 only.
 *
 * @param dev Pointer to the stepper motor controller instance
 * @param sg_settings Pointer to a struct containing the required StallGuard parameters
 */
void tmc5241_stepper_ctrl_configure_stallguard(const struct device *dev,
					       const struct tmc_stallguard_settings *sg_settings);

/**
 * @brief Set the maximum velocity of the stepper motor. TMC5241 only.
 *
 * @param dev Pointer to the stepper driver instance
 * @param velocity Maximum velocity in microsteps per second.
 *
 * @retval -EIO General input / output error
 * @retval 0 Success
 */
int tmc5241_stepper_ctrl_set_max_velocity(const struct device *dev, uint32_t velocity);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_TMC5241_H_ */
