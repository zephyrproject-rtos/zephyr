/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefanov <mail@andrestefanov.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_COMMON_H_
#define ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_COMMON_H_

/**
 * @brief Stepper Driver APIs
 * @defgroup step_dir_stepper Stepper Driver APIs
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/drivers/stepper.h>

/**
 * @brief Common step direction stepper config.
 *
 * This structure **must** be placed first in the driver's config structure.
 */
struct stepper_interface_step_dir {
	const struct gpio_dt_spec en_pin;
	const struct gpio_dt_spec step_pin;
	const struct gpio_dt_spec dir_pin;

	const bool invert_direction;

	/* TODO: add MSx pins */
	/* TODO: add step callback for position calculation */
};

#define STEPPER_INTERFACE_STEP_DIR_DT_INST_DEFINE(inst)                                            \
	static struct stepper_interface_step_dir stepper_interface_step_dir_##inst = {             \
		.en_pin = GPIO_DT_SPEC_INST_GET(inst, en_gpios),                                   \
		.step_pin = GPIO_DT_SPEC_INST_GET(inst, step_gpios),                               \
		.dir_pin = GPIO_DT_SPEC_INST_GET(inst, dir_gpios),                                 \
	};

#define STEPPER_INTERFACE_STEP_DIR_DT_INST_GET(inst) (&stepper_interface_step_dir_##inst)

/**
 * @brief Initializes the step direction interface for a stepper motor.
 *
 * This function configures the GPIO pins associated with the step direction interface,
 * including the enable, step, and direction pins. It checks if the pins are ready
 * and sets them as output pins. If any of the pins cannot be configured, the function
 * returns an error code.
 *
 * @param dev A pointer to the device structure for the stepper motor instance.
 *
 * @return 0 on successful initialization, or a negative error code if any pin
 *         configuration fails or if the pins are not ready.
 */
int step_dir_interface_init(const struct device *dev);

/**
 * @brief Perform a single step for a step direction stepper motor.
 *
 * It uses the stepper motor interface configuration to determine the step pin
 * and optionally inverts the step signal based on the configuration.
 *
 * @param dev Pointer to the device structure representing the stepper motor instance.
 *            The device structure must contain valid configuration and interface data.
 *
 * @return 0 on success, or a negative error code if the operation fails (e.g., GPIO pin setting
 * error).
 */
int step_dir_interface_step(const struct device *dev);

/**
 * @brief Set the direction of the stepper motor.
 *
 * This function configures the direction of the stepper motor based on the provided
 * direction value using the associated GPIO pin. The direction can be set to either
 * positive or negative as defined by the stepper_direction enumeration.
 *
 * @param dev A pointer to the stepper motor device structure.
 * @param direction The desired stepping direction, either STEPPER_DIRECTION_POSITIVE
 *                  or STEPPER_DIRECTION_NEGATIVE.
 *
 * @return 0 if the direction is successfully set, or a negative error code on failure.
 */
int step_dir_interface_set_dir(const struct device *dev, enum stepper_direction direction);

/**
 * @brief Enable the step direction interface of a stepper motor controller.
 *
 * This function enables the stepper motor controller associated with the
 * specified device. It sets the enable pin to active, which typically allows
 * the stepper motor to receive control signals.
 *
 * @param dev Pointer to the device instance of the stepper motor controller.
 *            This should be a valid device structure that represents the
 *            stepper motor controller to be enabled.
 *
 * @return 0 on success; a negative error code on failure.
 */
int step_dir_interface_enable(const struct device *dev);

/**
 * @brief Disables the stepper motor controller.
 *
 * This function disables the stepper motor controller by setting the
 * enable pin to a disabled state, depending on the configuration.
 *
 * @param dev Pointer to the device structure for the stepper motor.
 *            This device must be properly initialized and configured.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_interface_disable(const struct device *dev);

/** @} */

#endif /* ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_COMMON_H_ */
