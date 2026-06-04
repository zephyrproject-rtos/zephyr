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

#ifndef ZEPHYR_INCLUDE_INPUT_TOUCH_H_
#define ZEPHYR_INCLUDE_INPUT_TOUCH_H_

/**
 * @defgroup touch_events Touchscreen Events
 * @since 3.7
 * @version 0.1.0
 * @ingroup input_interface
 * @{
 */

#include <zephyr/input/input.h>
#include <zephyr/sys/util_macro.h>

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
	/** Horizontal resolution of touchscreen */
	uint32_t screen_width;
	/** Vertical resolution of touchscreen */
	uint32_t screen_height;
	/** X axis is inverted */
	bool inverted_x;
	/** Y axis is inverted */
	bool inverted_y;
	/** X and Y axes are swapped */
	bool swapped_x_y;
#if defined(CONFIG_INPUT_TOUCHSCREEN_COMMON_CALIBRATION)
	uint32_t raw_x_min;
	uint32_t raw_x_max;
	uint32_t raw_y_min;
	uint32_t raw_y_max;
#endif
};

/**
 * @brief Initialize common touchscreen config from devicetree
 *
 * @param node_id The devicetree node identifier.
 */
#define INPUT_TOUCH_DT_COMMON_CONFIG_INIT(node_id)				\
	{									\
		.screen_width = DT_PROP(node_id, screen_width),			\
		.screen_height = DT_PROP(node_id, screen_height),		\
		.inverted_x = DT_PROP(node_id, inverted_x),			\
		.inverted_y = DT_PROP(node_id, inverted_y),			\
		.swapped_x_y = DT_PROP(node_id, swapped_x_y),			\
		IF_ENABLED(CONFIG_INPUT_TOUCHSCREEN_COMMON_CALIBRATION, (	\
			.raw_x_min = DT_INST_PROP_OR(node_id, raw_x_min, 0),	\
			.raw_x_max = DT_INST_PROP_OR(node_id, raw_x_max, 4095),	\
			.raw_y_min = DT_INST_PROP_OR(node_id, raw_y_min, 0),	\
			.raw_y_max = DT_INST_PROP_OR(node_id, raw_y_max, 4095),	\
	     ))									\
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

#if defined(CONFIG_INPUT_TOUCHSCREEN_COMMON_CALIBRATION)
/**
 * @brief Common utility for reporting calibrated touchscreen position events.
 *
 * @param dev Touchscreen controller
 * @param x X coordinate as reported by the controller
 * @param y Y coordinate as reported by the controller
 */
void input_touchscreen_report_pos_calibrated(const struct device *dev,
				   uint32_t x,
				   uint32_t y);
#endif

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_INPUT_TOUCH_H_ */
