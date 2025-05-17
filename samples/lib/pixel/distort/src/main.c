/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/sys/util.h>
#include <zephyr/pixel/distort.h>
#include <zephyr/pixel/print.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#include "checkerboard.h"

#define WIDTH_OUT 120
#define HEIGHT_OUT 120

extern const unsigned char distorted_raw8[];
extern unsigned int distorted_raw8_len;

static uint8_t corrected_raw8[WIDTH_OUT * HEIGHT_OUT * 1];

int main(void)
{
	struct pixel_buf sbuf = {
		.data = (uint8_t *)distorted_raw8,
		.width = distort_input_width,
		.height = distort_input_height
	};
	struct pixel_buf dbuf = {
		.data = corrected_raw8,
		.width = WIDTH_OUT,
		.height = HEIGHT_OUT
	};

	LOG_INF("input image, distorted, %zu bytes:", sizeof(sbuf.data));
	pixel_print_buffer_256color(distorted_raw8, distorted_raw8_len, sbuf.width, sbuf.height,
				    VIDEO_PIX_FMT_GREY);

	LOG_INF("output image, corrected, %zu bytes:", sizeof(corrected_raw8));
	pixel_distort_raw8frame_to_raw8frame(&sbuf, &dbuf, &pixel_distort_grid);
	pixel_print_buffer_256color(corrected_raw8, sizeof(corrected_raw8), dbuf.width, dbuf.height,
				    VIDEO_PIX_FMT_GREY);

	return 0;
}
