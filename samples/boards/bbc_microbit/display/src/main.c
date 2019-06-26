/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/gpio.h>
#include <device.h>

#include <display/mb_display.h>

static struct mb_image smiley = MB_IMAGE({ 0, 1, 0, 1, 0 },
					 { 0, 1, 0, 1, 0 },
					 { 0, 0, 0, 0, 0 },
					 { 1, 0, 0, 0, 1 },
					 { 0, 1, 1, 1, 0 });

static const struct mb_image scroll[] = {
	MB_IMAGE({ 1, 0, 0, 0, 0 },
		 { 1, 0, 0, 0, 1 },
		 { 1, 0, 0, 1, 0 },
		 { 1, 0, 1, 0, 0 },
		 { 1, 1, 0, 0, 0 }),
	MB_IMAGE({ 1, 0, 0, 0, 0 },
		 { 0, 1, 0, 0, 0 },
		 { 0, 0, 1, 0, 0 },
		 { 0, 0, 0, 1, 0 },
		 { 0, 0, 0, 0, 1 }),
	MB_IMAGE({ 0, 0, 0, 0, 1 },
		 { 0, 0, 1, 0, 1 },
		 { 0, 1, 0, 1, 1 },
		 { 1, 0, 0, 0, 1 },
		 { 0, 0, 0, 0, 1 }),
};

static const struct mb_image animation[] = {
	MB_IMAGE({ 0, 0, 0, 0, 0 },
		 { 0, 0, 0, 0, 0 },
		 { 0, 0, 1, 0, 0 },
		 { 0, 0, 0, 0, 0 },
		 { 0, 0, 0, 0, 0 }),
	MB_IMAGE({ 0, 0, 0, 0, 0 },
		 { 0, 1, 1, 1, 0 },
		 { 0, 1, 1, 1, 0 },
		 { 0, 1, 1, 1, 0 },
		 { 0, 0, 0, 0, 0 }),
	MB_IMAGE({ 1, 1, 1, 1, 1 },
		 { 1, 1, 1, 1, 1 },
		 { 1, 1, 0, 1, 1 },
		 { 1, 1, 1, 1, 1 },
		 { 1, 1, 1, 1, 1 }),
	MB_IMAGE({ 1, 1, 1, 1, 1 },
		 { 1, 0, 0, 0, 1 },
		 { 1, 0, 0, 0, 1 },
		 { 1, 0, 0, 0, 1 },
		 { 1, 1, 1, 1, 1 }),
};

void main(void)
{
	struct mb_display *disp = mb_display_get();
	int x, y;

	/* Note: the k_sleep() calls after mb_display_print() and
	 * mb_display_image() are not normally needed since the APIs
	 * are used in an asynchronous manner. The k_sleep() calls
	 * are used here so the APIs can be sequentially demonstrated
	 * through this single main function.
	 */

	/* Display countdown from '9' to '0' */
	mb_display_print(disp, MB_DISPLAY_MODE_SINGLE,
			 K_SECONDS(1), "9876543210");
	k_sleep(K_SECONDS(11));

	/* Iterate through all pixels one-by-one */
	for (y = 0; y < 5; y++) {
		for (x = 0; x < 5; x++) {
			struct mb_image pixel = {};
			pixel.row[y] = BIT(x);
			mb_display_image(disp, MB_DISPLAY_MODE_SINGLE,
					 K_MSEC(250), &pixel, 1);
			k_sleep(K_MSEC(300));
		}
	}

	/* Show a smiley-face */
	mb_display_image(disp, MB_DISPLAY_MODE_SINGLE, K_SECONDS(2),
			 &smiley, 1);
	k_sleep(K_SECONDS(2));

	/* Show a short scrolling animation */
	mb_display_image(disp, MB_DISPLAY_MODE_SCROLL, K_SECONDS(1),
			 scroll, ARRAY_SIZE(scroll));
	k_sleep(K_SECONDS(1) * (ARRAY_SIZE(scroll) + 2));

	/* Show a sequential animation */
	mb_display_image(disp, MB_DISPLAY_MODE_DEFAULT | MB_DISPLAY_FLAG_LOOP,
			 K_MSEC(150), animation, ARRAY_SIZE(animation));
	k_sleep(K_MSEC(150) * ARRAY_SIZE(animation) * 5);

	/* Show some scrolling text ("Hello Zephyr!") */
	mb_display_print(disp, MB_DISPLAY_MODE_DEFAULT | MB_DISPLAY_FLAG_LOOP,
			 K_MSEC(500), "Hello Zephyr!");
}
