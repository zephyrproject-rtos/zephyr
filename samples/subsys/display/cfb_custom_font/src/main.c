/*
 * Copyright (c) 2018 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <zephyr/sys/printk.h>

#include "cfb_font_dice.h"

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	struct cfb_display *disp;
	struct cfb_framebuffer *fb;
	int err;

	if (!device_is_ready(dev)) {
		printk("Display device not ready\n");
	}

	if (display_set_pixel_format(dev, PIXEL_FORMAT_MONO10) != 0) {
		if (display_set_pixel_format(dev, PIXEL_FORMAT_MONO01) != 0) {
			printk("Failed to set required pixel format");
			return 0;
		}
	}

	if (display_blanking_off(dev) != 0) {
		printk("Failed to turn off display blanking\n");
		return 0;
	}

	disp = cfb_display_alloc(dev);
	if (!disp) {
		printk("Could not allocate framebuffer\n");
		return 0;
	}

	fb = cfb_display_get_framebuffer(disp);

	err = cfb_clear(fb, true);
	if (err) {
		printk("Could not clear framebuffer (err %d)\n", err);
	}

	err = cfb_print(fb, "123456", 0, 0);
	if (err) {
		printk("Could not display custom font (err %d)\n", err);
	}

	err = cfb_finalize(fb);
	if (err) {
		printk("Could not finalize framebuffer (err %d)\n", err);
	}
	return 0;
}
