/*
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/input/input_touch.h>

void input_touchscreen_report_pos(const struct device *dev,
		uint32_t x, uint32_t y,
		k_timeout_t timeout)
{
	const struct input_touchscreen_common_config *cfg = dev->config;

	__ASSERT(!cfg->inverted_y || cfg->screen_height > 0,
		 "Y coordinate inversion requires screen-height");
	__ASSERT(!cfg->inverted_x || cfg->screen_width > 0,
		 "X coordinate inversion requires screen-width");
	const uint32_t reported_x_code = cfg->swapped_x_y ? INPUT_ABS_Y : INPUT_ABS_X;
	const uint32_t reported_y_code = cfg->swapped_x_y ? INPUT_ABS_X : INPUT_ABS_Y;
	uint32_t reported_x = cfg->inverted_x ? cfg->screen_width - x : x;
	uint32_t reported_y = cfg->inverted_y ? cfg->screen_height - y : y;

	/* Optional rescale into the display's coordinate space. Swapping alone is
	 * only correct when the touchscreen's coordinate space and the display
	 * share an aspect ratio; on a rotated non-square panel the swapped axes
	 * still carry the other axis' range and must be scaled to fit.
	 *
	 * Each value keeps its own source range -- reported_x always spans
	 * screen-width -- while its target dimension follows the code it is
	 * reported under, which swapped-x-y has already chosen.
	 */
	if (cfg->output_width > 0 && cfg->output_height > 0) {
		const uint32_t dst_x = cfg->swapped_x_y ? cfg->output_height : cfg->output_width;
		const uint32_t dst_y = cfg->swapped_x_y ? cfg->output_width : cfg->output_height;

		__ASSERT(cfg->screen_width > 0 && cfg->screen_height > 0,
			 "coordinate scaling requires screen-width and screen-height");

		if (cfg->screen_width > 0 && cfg->screen_height > 0) {
			reported_x = reported_x * dst_x / cfg->screen_width;
			reported_y = reported_y * dst_y / cfg->screen_height;
		}
	}

	input_report_abs(dev, reported_x_code, reported_x, false, timeout);
	input_report_abs(dev, reported_y_code, reported_y, false, timeout);
}
