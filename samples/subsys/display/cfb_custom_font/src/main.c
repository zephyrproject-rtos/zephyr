/*
 * Copyright (c) 2018 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <display/cfb.h>
#include <sys/printk.h>

#include "cfb_font_dice.h"

#if defined(CONFIG_SSD1306)
#define DISPLAY_NAME DT_LABEL(DT_INST(0, solomon_ssd1306fb))
#elif defined(CONFIG_SSD16XX)
#define DISPLAY_NAME DT_LABEL(DT_INST(0, solomon_ssd16xxfb))
#else
#error Unsupported board
#endif

const struct device *display;

void main(void)
{
	int err;

	display = device_get_binding(DISPLAY_NAME);
	if (!display) {
		printk("Could not get device binding for display device\n");
	}

	err = cfb_framebuffer_init(display);
	if (err) {
		printk("Could not initialize framebuffer (err %d)\n", err);
	}

	err = cfb_framebuffer_clear(display, true);
	if (err) {
		printk("Could not clear framebuffer (err %d)\n", err);
	}

	err = cfb_print(display, "123456", 0, 0);
	if (err) {
		printk("Could not display custom font (err %d)\n", err);
	}

	err = cfb_framebuffer_finalize(display);
	if (err) {
		printk("Could not finalize framebuffer (err %d)\n", err);
	}
}
