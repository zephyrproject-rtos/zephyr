/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefanov <mail@andrestefanov.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVER_STEPPER_STEP_DIR_INTERFACE_H_
#define ZEPHYR_DRIVER_STEPPER_STEP_DIR_INTERFACE_H_


#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper.h>

/**
 * @brief Common step direction stepper config.
 *
 * This structure **must** be placed first in the driver's config structure.
 */
struct stepper_interface_step_dir {
	/** Step pin specification */
	const struct gpio_dt_spec step_pin;
	/** Direction pin specification */
	const struct gpio_dt_spec dir_pin;

	/* Invert direction pin logic */
	/* If true, setting direction to STEPPER_DIRECTION_POSITIVE will set the pin low */
	/* If false, setting direction to STEPPER_DIRECTION_POSITIVE will set the pin high */
	const bool invert_direction;

	/* Step on both rising and falling edges for 2x speed */
	const bool dual_edge_step;  
};

#define STEPPER_INTERFACE_STEP_DIR_DT_INST_DEFINE(inst)                                            \
	static struct stepper_interface_step_dir stepper_interface_step_dir_##inst = {             \
		.step_pin = GPIO_DT_SPEC_INST_GET(inst, step_gpios),                               \
		.dir_pin = GPIO_DT_SPEC_INST_GET(inst, dir_gpios),                                 \
		.invert_direction = DT_INST_PROP_OR(inst, invert_direction, false),                \
		.dual_edge_step = DT_INST_PROP_OR(inst, dual_edge_step, false),                    \
	};

#define STEPPER_INTERFACE_STEP_DIR_DT_INST_GET(inst) (&stepper_interface_step_dir_##inst)

/**
 * @brief Initialize the step direction interface.
 *
 * This function configures the step and direction GPIO pins for the stepper motor.
 * It checks if the pins are ready and configures them as outputs.
 *
 * @param interface Pointer to the stepper interface structure.
 * @return 0 on success, or a negative error code on failure.
 */
static inline int step_dir_interface_init(const struct stepper_interface_step_dir *interface)
{
	int ret;

	if (!gpio_is_ready_dt(&interface->step_pin)) {
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&interface->dir_pin)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&interface->step_pin, GPIO_OUTPUT);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&interface->dir_pin, GPIO_OUTPUT);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/**
 * @brief Step the stepper motor.
 *
 * This function toggles the step pin to create a step pulse. It handles both
 * single-edge and dual-edge stepping modes.
 *
 * @param interface Pointer to the stepper interface structure.
 */
static inline void step_dir_interface_step(const struct stepper_interface_step_dir *interface)
{
	if (interface->dual_edge_step) {
		/* Dual-edge mode: single toggle creates both rising and falling edge */
		gpio_pin_toggle_dt(&interface->step_pin);
	} else {
		/* Single-edge mode: toggle twice to create complete pulse (rising + falling) */
		gpio_pin_toggle_dt(&interface->step_pin);
		gpio_pin_toggle_dt(&interface->step_pin);
	}
}

/**
 * @brief Set the direction for the stepper motor.
 *
 * This function sets the direction pin to either high or low based on the
 * specified direction. It also respects the invert_direction flag.
 *
 * @param interface Pointer to the stepper interface structure.
 * @param direction The desired direction (STEPPER_DIRECTION_POSITIVE or STEPPER_DIRECTION_NEGATIVE).
 */
static inline void step_dir_interface_set_dir(const struct stepper_interface_step_dir *interface,
					       const enum stepper_direction direction)
{
	/* Optimized for speed: no error checking, no return value */
	bool pin_high = (direction == STEPPER_DIRECTION_POSITIVE);
	
	/* XOR with invert_direction for efficient boolean inversion */
	pin_high ^= interface->invert_direction;
	
	gpio_pin_set_dt(&interface->dir_pin, pin_high);
}

#endif /* ZEPHYR_DRIVER_STEPPER_STEP_DIR_INTERFACE_H_ */
