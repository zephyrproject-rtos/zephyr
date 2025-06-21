/*
 * Copyright 2024 Fabian Blatz <fabianblatz@gmail.com>
 *
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
struct step_dir_stepper_common_config {
	const struct gpio_dt_spec step_pin;
	const struct gpio_dt_spec dir_pin;
	bool dual_edge;
	bool invert_direction;
};

/**
 * @brief Initialize common step direction stepper config from devicetree instance.
 *        If the counter property is set, the timing source will be set to the counter timing
 *        source.
 *
 * @param node_id The devicetree node identifier.
 */
#define STEP_DIR_STEPPER_DT_COMMON_CONFIG_INIT(node_id)                                            \
	{                                                                                          \
		.step_pin = GPIO_DT_SPEC_GET(node_id, step_gpios),                                 \
		.dir_pin = GPIO_DT_SPEC_GET(node_id, dir_gpios),                                   \
		.dual_edge = DT_PROP_OR(node_id, dual_edge_step, false),                           \
		.invert_direction = DT_PROP(node_id, invert_direction),                            \
	}

/**
 * @brief Initialize common step direction stepper config from devicetree instance.
 * @param inst Instance.
 */
#define STEP_DIR_STEPPER_DT_INST_COMMON_CONFIG_INIT(inst)                                          \
	STEP_DIR_STEPPER_DT_COMMON_CONFIG_INIT(DT_DRV_INST(inst))

/**
 * @brief Validate the offset of the common data structures.
 *
 * @param config Name of the config structure.
 */
#define STEP_DIR_STEPPER_STRUCT_CHECK(config)                                                      \
	BUILD_ASSERT(offsetof(config, common) == 0,                                                \
		     "struct step_dir_stepper_common_config must be placed first");

/**
 * @brief Common function to initialize a step direction stepper device at init time.
 *
 * This function must be called at the end of the device init function.
 *
 * @param dev Step direction stepper device instance.
 *
 * @retval 0 If initialized successfully.
 * @retval -errno Negative errno in case of failure.
 */
int step_dir_stepper_common_init(const struct device *dev);

/**
 * @brief Common function to perform a step on a step direction stepper device.
 *
 * @param dev Step direction stepper device instance.
 *
 * @retval 0 If step performed successfully.
 * @retval -errno Negative errno in case of failure.
 */
int step_dir_stepper_common_step(const struct device *dev);

/**
 * @brief Common function to set the direction of a step direction stepper device.
 *
 * @param dev Step direction stepper device instance.
 * @param dir Direction to set.
 *
 * @retval 0 If direction set successfully.
 * @retval -errno Negative errno in case of failure.
 */
int step_dir_stepper_common_set_direction(const struct device *dev,
					  const enum stepper_direction dir);

/** @} */

#endif /* ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_COMMON_H_ */
