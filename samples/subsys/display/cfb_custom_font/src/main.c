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

static struct cfb_display disp;
static uint8_t xferbuf[CONFIG_CFB_CUSTOM_FONT_SAMPLE_TRANSFER_BUFFER_SIZE];
static uint8_t cmdbuf[CONFIG_CFB_CUSTOM_FONT_SAMPLE_COMMAND_BUFFER_SIZE];

int main(void)
{
	const struct device *const display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	int err;

	if (!device_is_ready(display)) {
		printk("Display device not ready\n");
	}

	if (display_set_pixel_format(display, PIXEL_FORMAT_MONO10) != 0) {
		if (display_set_pixel_format(display, PIXEL_FORMAT_MONO01) != 0) {
			printk("Failed to set required pixel format");
			return 0;
		}
	}

	if (display_blanking_off(display) != 0) {
		printk("Failed to turn off display blanking\n");
		return 0;
	}

	err = cfb_display_init(&disp, display, xferbuf, sizeof(xferbuf), cmdbuf, sizeof(cmdbuf));
	if (err) {
		printk("Could not initialize framebuffer (err %d)\n", err);
	}

	err = cfb_clear(&disp.fb, true);
	if (err) {
		printk("Could not clear framebuffer (err %d)\n", err);
	}

	err = cfb_print(&disp.fb, "123456", 0, 0);
	if (err) {
		printk("Could not display custom font (err %d)\n", err);
	}

	err = cfb_finalize(&disp.fb);
	if (err) {
		printk("Could not finalize framebuffer (err %d)\n", err);
	}
	return 0;
}
