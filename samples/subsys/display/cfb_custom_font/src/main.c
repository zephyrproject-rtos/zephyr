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
#define DISPLAY_NODE DT_INST(0, solomon_ssd1306fb)
#elif defined(CONFIG_SSD16XX)
#define DISPLAY_NODE DT_INST(0, solomon_ssd16xxfb)
#else
#error Unsupported board
#endif

void main(void)
{
	const struct device *display = DEVICE_DT_GET(DISPLAY_NODE);
	int err;

	if (!device_is_ready(display)) {
		printk("Display device not ready\n");
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
