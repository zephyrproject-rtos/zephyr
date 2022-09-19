/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "img.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/ztest.h>

#include <lvgl.h>

#define IMG_FILE_PATH "/mnt/img.bin"

#define LVGL_PARTITION		storage_partition
#define LVGL_PARTITION_ID	FIXED_PARTITION_ID(LVGL_PARTITION)

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);

static struct fs_mount_t mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &cstorage,
	.storage_dev = (void *)LVGL_PARTITION_ID,
	.mnt_point = "/mnt"
};

ZTEST(lvgl_screen, test_get_default_screen)
{
	zassert_not_null(lv_scr_act(), "No default screen");
}

ZTEST(lvgl_screen, test_add_delete_screen)
{
	lv_obj_t *default_screen = lv_scr_act();

	zassert_not_null(default_screen, "No default screen");

	lv_obj_t *new_screen = lv_obj_create(NULL);

	zassert_not_null(new_screen, "Failed to create new screen");

	lv_scr_load(new_screen);

	lv_task_handler();

	lv_obj_t *act_screen = lv_scr_act();

	zassert_equal_ptr(act_screen, new_screen, "New screen not active");

	lv_obj_del(new_screen);

	lv_scr_load(default_screen);

	lv_task_handler();

	act_screen = lv_scr_act();
	zassert_equal_ptr(act_screen, default_screen,
			"Default screen not active");

}
ZTEST_USER(lvgl_fs, test_add_img)
{
	lv_obj_t *img = lv_img_create(lv_scr_act());

	zassert_not_null(img, "Failed to create image");

	lv_img_set_src(img, IMG_FILE_PATH);
	lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

	lv_task_handler();
}


void *setup_fs(void)
{
	struct fs_file_t img;
	struct fs_dirent info;
	int ret;
	const lv_img_dsc_t *c_img = get_lvgl_img();

	ret = fs_mount(&mnt);
	if (ret < 0) {
		TC_PRINT("Failed to mount file system: %d\n", ret);
		ztest_test_fail();
		return NULL;
	}

	ret = fs_stat(IMG_FILE_PATH, &info);
	if ((ret == 0) && (info.type == FS_DIR_ENTRY_FILE)) {
		return NULL;
	}

	fs_file_t_init(&img);
	ret = fs_open(&img, IMG_FILE_PATH, FS_O_CREATE | FS_O_WRITE);
	if (ret < 0) {
		TC_PRINT("Failed to open image file: %d\n", ret);
		ztest_test_fail();
		return NULL;
	}

	ret = fs_write(&img, &c_img->header, sizeof(lv_img_header_t));
	if (ret < 0) {
		TC_PRINT("Failed to write image file header: %d\n", ret);
		ztest_test_fail();
		return NULL;
	}

	ret = fs_write(&img, c_img->data, c_img->data_size);
	if (ret < 0) {
		TC_PRINT("Failed to write image file data: %d\n", ret);
		ztest_test_fail();
		return NULL;
	}

	ret = fs_close(&img);
	if (ret < 0) {
		TC_PRINT("Failed to close image file: %d\n", ret);
		ztest_test_fail();
		return NULL;
	}
	return NULL;
}

void teardown_fs(void *data)
{
	return;
}

ZTEST_SUITE(lvgl_screen, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(lvgl_fs, NULL, setup_fs, NULL, NULL, teardown_fs);
