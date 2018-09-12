/*
 * Copyright (c) 2017 dXplore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/display/ili9340.h>
#include <stdio.h>
#include <string.h>
#include <zephyr.h>

#define SYS_LOG_DOMAIN "main"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#include <logging/sys_log.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

/* This example will update each 500ms one of the LCD corners
 * wit a rectangular bitmap.
 * The color of the bit map is changed for every
 * iteration and is picked out of a set of red, green and blue.
 */
void main(void)
{
	struct device *dev;

	/* size of the rectangle */
	const size_t w = 40;
	const size_t h = 20;
	const size_t buf_size = 3 * w * h;
	u8_t *buf;

	/* xy coordinates where to place rectangles*/
	const size_t x0 = 0;
	const size_t y0 = 0;
	const size_t x1 = SCREEN_WIDTH - w;
	const size_t y1 = 0;
	const size_t x2 = SCREEN_WIDTH - w;
	const size_t y2 = SCREEN_HEIGHT - h;
	const size_t x3 = 0;
	const size_t y3 = SCREEN_HEIGHT - h;

	size_t color = 0;
	size_t cnt = 0;
	int h_step;

	dev = device_get_binding("ILI9340");

	if (dev == NULL) {
		SYS_LOG_ERR("device not found.  Aborting test.");
		return;
	}

	/* Allocate rectangular buffer for corner data */
	buf = k_malloc(buf_size);

	if (buf == NULL) {
		SYS_LOG_ERR("Could not allocate memory.  Aborting test.");
		return;
	}

	/* Clear ili9340 frame buffer before enabling LCD, reuse corner buffer
	 */
	(void)memset(buf, 0, buf_size);
	h_step = (w * h) / SCREEN_WIDTH;

	for (int idx = 0; idx < SCREEN_HEIGHT; idx += h_step) {
		ili9340_write_bitmap(dev, 0, idx, SCREEN_WIDTH, h_step, buf);
	}

	ili9340_display_on(dev);

	while (1) {
		/* Update the color of the rectangle buffer and write the buffer
		 * to  one of the corners
		 */
		(void)memset(buf, 0, buf_size);
		for (size_t idx = color; idx < buf_size; idx += 3) {
			*(buf + idx) = 255;
		}
		switch (cnt % 4) {
		case 0:
			ili9340_write_bitmap(dev, x0, y0, w, h, buf);
			break;
		case 1:
			ili9340_write_bitmap(dev, x1, y1, w, h, buf);
			break;
		case 2:
			ili9340_write_bitmap(dev, x2, y2, w, h, buf);
			break;
		case 3:
			ili9340_write_bitmap(dev, x3, y3, w, h, buf);
			break;
		}
		++cnt;
		++color;
		if (color > 2) {
			color = 0;
		}
		k_sleep(500);
	}
}
