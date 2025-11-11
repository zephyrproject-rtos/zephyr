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
	const uint32_t reported_x_code = cfg->swapped_x_y ? INPUT_ABS_Y : INPUT_ABS_X;
	const uint32_t reported_y_code = cfg->swapped_x_y ? INPUT_ABS_X : INPUT_ABS_Y;
	const uint32_t reported_x = cfg->inverted_x ? cfg->screen_width - x : x;
	const uint32_t reported_y = cfg->inverted_y ? cfg->screen_height - y : y;

	input_report_abs(dev, reported_x_code, reported_x, false, timeout);
	input_report_abs(dev, reported_y_code, reported_y, false, timeout);
}
