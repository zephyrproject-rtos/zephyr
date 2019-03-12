/*
 * Copyright (c) 2019 Ioannis <gon1332> Konstantelias
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include "ucam3.h"

#define MAX_PICTURE_SIZE (1 << 14)

static u8_t picture[MAX_PICTURE_SIZE];

void main(void)
{
	int ret;
	image_config_t img_conf;
	u32_t size;

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

	/* Configure image settings */
	img_conf.format = FMT_JPEG;
	img_conf.size.jpeg = JPEG_640_480;

	ret = ucam3_initial(&img_conf);
	if (ret) {
		printk("Error on camera initial\n");
		return;
	}

	/* Set package size */
	ret = ucam3_set_package_size(512);
	if (ret) {
		printk("Error on camera set package size\n");
		return;
	}

	/* Take a snapshot */
	ret = ucam3_snapshot(&img_conf);
	if (ret) {
		printk("Error on camera snapshot\n");
		return;
	}

	/* 'First photo' delay: Time recommended for the camera to settle before the
	 * first photo should be taken. Section 13. Specifications and Ratings
	 */
	k_sleep(2000);

	/* Get the snapshot */
	ret = ucam3_get_picture(picture, &size);
	if (ret) {
		printk("Error on camera get picture\n");
		return;
	}

	printk("Success\n");

	while (1) {
		continue;
	}
}
