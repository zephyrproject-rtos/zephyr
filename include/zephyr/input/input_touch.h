/*
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for touch events API.
 * @ingroup touch_events
 */

#ifndef ZEPHYR_INCLUDE_INPUT_INPUT_TOUCH_H_
#define ZEPHYR_INCLUDE_INPUT_INPUT_TOUCH_H_

/**
 * @defgroup touch_events Touchscreen Events
 * @since 3.7
 * @version 0.1.0
 * @ingroup input_interface
 * @{
 */

#include <zephyr/input/input.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Common touchscreen config.
 *
 * This structure **must** be placed first in the driver's config structure.
 *
 * see touchscreem-common.yaml for more details
 */
struct input_touchscreen_common_config {
	/** Horizontal resolution of the touch controller's own coordinate space */
	uint32_t screen_width;
	/** Vertical resolution of the touch controller's own coordinate space */
	uint32_t screen_height;
	/** X axis is inverted */
	bool inverted_x;
	/** Y axis is inverted */
	bool inverted_y;
	/** X and Y axes are swapped */
	bool swapped_x_y;
	/** Horizontal resolution of the panel to scale into, 0 to disable scaling */
	uint32_t output_width;
	/** Vertical resolution of the panel to scale into, 0 to disable scaling */
	uint32_t output_height;
};

/**
 * @brief Select an output dimension from the display phandle, else the node's own property.
 *
 * Prefers the width/height of the node referenced by the @c display phandle; falls back to the
 * node's own @c output-width / @c output-height when @c display is not set, and to 0 when
 * neither is present, which disables scaling.
 *
 * @param node_id The devicetree node identifier.
 * @param dim The display-controller dimension property (width or height).
 * @param prop The touchscreen fallback property (output_width or output_height).
 */
#define INPUT_TOUCH_OUTPUT_DIM(node_id, dim, prop)			\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, display),			\
		(DT_PROP(DT_PHANDLE(node_id, display), dim)),		\
		(DT_PROP_OR(node_id, prop, 0)))

/**
 * @brief Initialize common touchscreen config from devicetree
 *
 * @param node_id The devicetree node identifier.
 */
#define INPUT_TOUCH_DT_COMMON_CONFIG_INIT(node_id)			\
	{								\
		.screen_width = DT_PROP(node_id, screen_width),		\
		.screen_height = DT_PROP(node_id, screen_height),	\
		.inverted_x = DT_PROP(node_id, inverted_x),		\
		.inverted_y = DT_PROP(node_id, inverted_y),		\
		.swapped_x_y = DT_PROP(node_id, swapped_x_y),		\
		.output_width = INPUT_TOUCH_OUTPUT_DIM(node_id, width, output_width),	\
		.output_height = INPUT_TOUCH_OUTPUT_DIM(node_id, height, output_height),	\
	}

/**
 * @brief Initialize common touchscreen config from devicetree instance.
 *
 * @param inst Instance.
 */
#define INPUT_TOUCH_DT_INST_COMMON_CONFIG_INIT(inst) \
	INPUT_TOUCH_DT_COMMON_CONFIG_INIT(DT_DRV_INST(inst))

/**
 * @brief Validate the offset of the common config structure.
 *
 * @param config Name of the config structure.
 */
#define INPUT_TOUCH_STRUCT_CHECK(config) \
	BUILD_ASSERT(offsetof(config, common) == 0, \
		     "struct input_touchscreen_common_config must be placed first");

/**
 * @brief Common utility for reporting touchscreen position events.
 *
 * @param dev Touchscreen controller
 * @param x X coordinate as reported by the controller
 * @param y Y coordinate as reported by the controller
 * @param timeout Timeout for reporting the event
 */
void input_touchscreen_report_pos(const struct device *dev,
		uint32_t x, uint32_t y,
		k_timeout_t timeout);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_INPUT_INPUT_TOUCH_H_ */
