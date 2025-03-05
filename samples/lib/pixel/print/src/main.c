/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/sys/util.h>
#include <zephyr/pixel/print.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

static uint8_t rgb24frame[16 * 32 * 3];

void print_image(void)
{
	const uint8_t beg[] = {0x00, 0x70, 0xc5};
	const uint8_t end[] = {0x79, 0x29, 0xd2};

	/* Generate an image with a gradient of the two colors above */
	for (size_t i = 0, size = sizeof(rgb24frame); i + 3 <= size; i += 3) {
		rgb24frame[i + 0] = (beg[0] * (size - i) + end[0] * i) / size;
		rgb24frame[i + 1] = (beg[1] * (size - i) + end[1] * i) / size;
		rgb24frame[i + 2] = (beg[2] * (size - i) + end[2] * i) / size;
	}

	LOG_INF("Printing the gradient #%02x%02x%02x -> #%02x%02x%02x",
		beg[0], beg[1], beg[2], end[0], end[1], end[2]);

	LOG_INF("hexdump:");
	pixel_print_rgb24frame_hex(rgb24frame, sizeof(rgb24frame), 16, 32);

	LOG_INF("truecolor:");
	pixel_print_rgb24frame_truecolor(rgb24frame, sizeof(rgb24frame), 16, 32);

	LOG_INF("256color:");
	pixel_print_rgb24frame_256color(rgb24frame, sizeof(rgb24frame), 16, 32);
}

void print_histogram(void)
{
	static const uint16_t rgb24hist[] = {
		9, 4, 7, 1, 0, 5, 1, 0, 0, 2, 2, 3, 0, 1, 3, 0,
		7, 6, 5, 1, 1, 4, 2, 0, 1, 2, 3, 4, 1, 1, 2, 2,
		8, 4, 7, 4, 2, 3, 1, 2, 2, 2, 2, 2, 0, 0, 1, 1,
	};

	static const uint16_t y8hist[] = {
		8, 5, 6, 2, 1, 4, 1, 1, 1, 2, 3, 3, 1, 1, 2, 1,
	};

	LOG_INF("Printing a histogram of %zu RGB buckets", ARRAY_SIZE(rgb24hist));
	pixel_print_rgb24hist(rgb24hist, ARRAY_SIZE(rgb24hist), 8);

	LOG_INF("Printing a histogram of %zu Y (luma) buckets", ARRAY_SIZE(y8hist));
	pixel_print_y8hist(y8hist, ARRAY_SIZE(y8hist), 8);
}

int main(void)
{
	print_image();
	print_histogram();

	return 0;
}
