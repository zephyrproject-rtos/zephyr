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
/**
 * @brief Common step direction stepper config.
 *
 * This structure **must** be placed first in the driver's config structure.
 */
struct step_dir_stepper_common_config {
	uint32_t step_width_ns;
	bool dual_edge;
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
		.dual_edge = DT_PROP_OR(node_id, dual_edge_step, false),                           \
		.step_width_ns = DT_PROP(node_id, step_width_ns),                                  \
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

/** @} */

#endif /* ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_COMMON_H_ */
