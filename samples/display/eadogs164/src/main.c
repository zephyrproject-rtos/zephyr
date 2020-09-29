/*
 * Copyright (c) 2020
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <display/display_eadogs164.h>
#include <stdio.h>
#include <drivers/display.h>


void main(void)
{

	const struct device *dev = device_get_binding("EADOGS164");

	if (dev == NULL) {
		printf("Could not get EADOGS164 device\n");
		return;
	}
	int rc;
	struct display_buffer_descriptor desc;

	desc.buf_size = 11;
	char *buf_w = "HELLO WORLD";
	char buf_r[13];

	buf_r[12] = '\0';
	while (true) {
		rc = display_set_orientation(dev,
				DISPLAY_ORIENTATION_ORIENTATION_ROTATED_180);
		 /* set bottom view DISPLAY_ORIENTATION_ROTATED_180 */
		rc = display_write(dev, 0, 0, &desc, buf_w);
		rc = display_read(dev, 0, 0, &desc, buf_r);
		printk("data written: %s data read: %s\n", buf_w, buf_r+1);
		k_sleep(K_MSEC(5000));
	}
}
