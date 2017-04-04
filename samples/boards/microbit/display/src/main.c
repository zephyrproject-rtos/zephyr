/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <board.h>
#include <gpio.h>
#include <device.h>

#include <display/mb_display.h>

static struct mb_image smiley = MB_IMAGE({ 0, 1, 0, 1, 0 },
					 { 0, 1, 0, 1, 0 },
					 { 0, 0, 0, 0, 0 },
					 { 1, 0, 0, 0, 1 },
					 { 0, 1, 1, 1, 0 });

void main(void)
{
	struct mb_display *disp = mb_display_get();
	int x, y;

	/* Display countdown from '9' to '0' */
	mb_display_string(disp, K_SECONDS(1), "9876543210");

	k_sleep(K_SECONDS(11));

	/* Iterate through all pixels one-by-one */
	for (y = 0; y < 5; y++) {
		for (x = 0; x < 5; x++) {
			struct mb_image pixel = {};
			pixel.row[y] = BIT(x);
			mb_display_image(disp, &pixel, K_MSEC(250));
			k_sleep(K_MSEC(300));
		}
	}

	/* Show a smiley-face */
	mb_display_image(disp, &smiley, K_SECONDS(2));
	k_sleep(K_SECONDS(2));

	/* Show some scrolling text ("Hello Zephyr!") */
	mb_display_print(disp, "Hello Zephyr!");
}
