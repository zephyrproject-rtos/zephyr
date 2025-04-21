/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PIXEL_DISTORT_H_
#define ZEPHYR_INCLUDE_PIXEL_DISTORT_H_

/** Coordinates of a pixel in row and column */
struct pixel_pt {
	/** Position through the vertical axis, incrementing from top to bottom */
	int16_t row;
	/** Position through the horizontal axis incrementing from left to right */
	int16_t col;
};

/** Buffer of contiguous pixel data. */
struct pixel_buf {
	/** Pointer to the buffer pixel data in a format known by the user. */
	uint8_t *data;
	/** Width of the image in pixels (not bytes) */
	uint16_t width;
	/** Height of the imager in pixels */
	uint16_t height;
};

/** Grid of pixel coordinates. */
struct pixel_grid {
	/* Point coordinates */
	const struct pixel_pt *points;
	/* Number of columns in the grid */
	uint16_t width;
	/* Number of rows in the grid */
	uint16_t height;
};

void pixel_distort_raw8frame_to_raw8frame(struct pixel_buf *sbuf, struct pixel_buf *dbuf,
					  const struct pixel_grid *grid);

#endif /* ZEPHYR_INCLUDE_PIXEL_DISTORT_H_ */
