/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <display.h>
#include <stdio.h>
#include <string.h>
#include <zephyr.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

/* This example will update each 500ms one of the LCD corners
 * wit a rectangular bitmap.
 * The color of the bit map is changed for every
 * iteration and is picked out of a set of red, green and blue.
 */
void main(void)
{
	struct device *dev;

	struct display_capabilities capabilities;
	struct display_buffer_descriptor buf_desc;

	/* size of the rectangle */
	const size_t w = 40;
	const size_t h = 20;
	const size_t buf_size = 3 * w * h;
	u8_t *buf;

	/* xy coordinates where to place rectangles*/
	size_t x0, y0, x1, y1, x2, y2, x3, y3;

	size_t color = 0;
	size_t cnt = 0;
	int h_step;

	dev = device_get_binding("ILI9340");

	if (dev == NULL) {
		LOG_ERR("Device not found. Aborting test.");
		return;
	}

	display_get_capabilities(dev, &capabilities);

	x0 = 0;
	y0 = 0;
	x1 = capabilities.x_resolution - w;
	y1 = 0;
	x2 = capabilities.x_resolution - w;
	y2 = capabilities.y_resolution - h;
	x3 = 0;
	y3 = capabilities.y_resolution - h;


	/* Allocate rectangular buffer for corner data */
	buf = k_malloc(buf_size);

	if (buf == NULL) {
		LOG_ERR("Could not allocate memory. Aborting test.");
		return;
	}

	/* Clear ili9340 frame buffer before enabling LCD, reuse corner buffer
	 */
	(void)memset(buf, 0, buf_size);
	h_step = (w * h) / capabilities.x_resolution;

	buf_desc.buf_size = buf_size;
	buf_desc.pitch = capabilities.x_resolution;
	buf_desc.width = capabilities.x_resolution;
	buf_desc.height = h_step;

	for (int idx = 0; idx < capabilities.y_resolution; idx += h_step) {
		display_write(dev, 0, idx, &buf_desc,  buf);
	}

	display_blanking_off(dev);

	buf_desc.pitch = w;
	buf_desc.width = w;
	buf_desc.height = h;

	while (1) {
		/* Update the color of the rectangle buffer and write the buffer
		 * to  one of the corners
		 */
		(void)memset(buf, 0, buf_size);
		for (size_t idx = color; idx < buf_size; idx += 3) {
			*(buf + idx) = 255U;
		}
		switch (cnt % 4) {
		case 0:
			display_write(dev, x0, y0, &buf_desc, buf);
			break;
		case 1:
			display_write(dev, x1, y1, &buf_desc, buf);
			break;
		case 2:
			display_write(dev, x2, y2, &buf_desc, buf);
			break;
		case 3:
			display_write(dev, x3, y3, &buf_desc, buf);
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
