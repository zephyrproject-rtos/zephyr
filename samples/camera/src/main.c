/*
 * Copyright (c) 2019 Ioannis <gon1332> Konstantelias
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include "ucam3.h"

void main(void)
{
	int ret;
	image_config_t img_conf;

	/* Initialize the camera */
	ret = ucam3_create();
	if (ret) {
		printk("Error on camera create\n");
		return;
	}

	/* Minimal delay before synchronising */
	k_sleep(800);

	/* Synchronize with the camera */
	ret = ucam3_sync();
	if (ret) {
		printk("Error on camera sync\n");
		return;
	}

	/* Minimal delay after synchronising */
	k_sleep(1000);

	/* Configure image settings */
	img_conf.format = FMT_JPEG;
	img_conf.size.jpeg = JPEG_160_128;

	ret = ucam3_initial(&img_conf);
	if (ret) {
		printk("Error on camera initial\n");
		return;
	}

	/* Take a snapshot */
	ret = ucam3_snapshot(&img_conf);
	if (ret) {
		printk("Error on camera snapshot\n");
		return;
	}

	printk("Success\n");

	while (1) {
		continue;
	}
}
