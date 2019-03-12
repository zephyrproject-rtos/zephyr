/*
 * Copyright (c) 2019 Ioannis <gon1332> Konstantelias
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include "ucam3.h"

#define MAX_PICTURE_SIZE (1 << 14)
#define NUM_PICTURES 200

static u8_t picture[MAX_PICTURE_SIZE];

struct camera {
	image_config_t img_conf;
};

static struct camera cam_info;

static int camera_create(void)
{
	int ret;

	ret = ucam3_create();
	if (ret) {
		printk("Error on ucam3 create\n");
		return -1;
	}

	/* Minimal delay after power-up and before synchronising */
	k_sleep(STARTUP_DELAY);

	return 0;
}

static void camera_configure(image_config_t *img_conf)
{
	memcpy(&cam_info.img_conf, img_conf, sizeof(image_config_t));
}

static int camera_take_snapshot(u8_t *data, u32_t *size)
{
	int ret;

	/* Synchronize with the camera */
	ret = ucam3_sync();
	if (ret) {
		printk("Error on ucam3 sync\n");
		return -1;
	}

	/* Set picture configurations */
	ret = ucam3_initial(&cam_info.img_conf);
	if (ret) {
		printk("Error on ucam3 initial\n");
		return -1;
	}

	/* Set package size */
	ret = ucam3_set_package_size(512);
	if (ret) {
		printk("Error on ucam3 set package size\n");
		return -1;
	}

	/* Take a snapshot */
	ret = ucam3_snapshot(&cam_info.img_conf);
	if (ret) {
		printk("Error on ucam3 snapshot\n");
		return -1;
	}

	/* 'First photo' delay: Time recommended for the camera to settle before the
	 * first photo should be taken. Section 13. Specifications and Ratings
	 */
	k_sleep(FIRST_PHOTO_DELAY);

	/* Get the snapshot */
	ret = ucam3_get_picture(data, size);
	if (ret) {
		printk("Error on ucam3 get picture\n");
		return -1;
	}

	return 0;
}

void main(void)
{
	int ret;
	image_config_t img_conf;
	u32_t size;
	u8_t i;

	ret = camera_create();
	if (ret) {
		printk("Error on camera create\n");
		return;
	}

	img_conf.format = FMT_JPEG;
	img_conf.size.jpeg = JPEG_640_480;
	camera_configure(&img_conf);

	for (i = 1; i <= NUM_PICTURES; i++) {
		printk("%3d/%d: ", i, NUM_PICTURES);

		/* Take a snapshot */
		ret = camera_take_snapshot(picture, &size);
		if (ret) {
			printk("Error on camera take snapshot\n");
			break;
		}
		printk("Took snapshot of %u bytes.\n", size);

		k_sleep(1000);
	}

	while (1) {
		continue;
	}
}
