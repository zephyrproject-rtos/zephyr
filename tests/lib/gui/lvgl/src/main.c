/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "img.h"

#include <zephyr.h>
#include <device.h>
#include <fs/fs.h>
#include <fs/littlefs.h>
#include <ztest.h>

#include <lvgl.h>

#define IMG_FILE_PATH "/mnt/img.bin"

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);

static struct fs_mount_t mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &cstorage,
	.storage_dev = (void *)FLASH_AREA_ID(storage),
	.mnt_point = "/mnt"
};

void test_get_default_screen(void)
{
	zassert_not_null(lv_scr_act(), "No default screen");
}

void test_add_delete_screen(void)
{
	lv_obj_t *default_screen = lv_scr_act();

	zassert_not_null(default_screen, "No default screen");

	lv_obj_t *new_screen = lv_obj_create(NULL, NULL);

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
void test_add_img(void)
{
	lv_obj_t *img = lv_img_create(lv_scr_act(), NULL);

	zassert_not_null(img, "Failed to create image");

	lv_img_set_src(img, IMG_FILE_PATH);
	lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, 0);

	lv_task_handler();
}


void setup_fs(void)
{
	struct fs_file_t img;
	struct fs_dirent info;
	int ret;
	const lv_img_dsc_t *c_img = get_lvgl_img();

	ret = fs_mount(&mnt);
	if (ret < 0) {
		TC_PRINT("Failed to mount file system: %d\n", ret);
		ztest_test_fail();
		return;
	}

	ret = fs_stat(IMG_FILE_PATH, &info);
	if ((ret == 0) && (info.type == FS_DIR_ENTRY_FILE)) {
		return;
	}

	ret = fs_open(&img, IMG_FILE_PATH, FS_O_CREATE | FS_O_WRITE);
	if (ret < 0) {
		TC_PRINT("Failed to open image file: %d\n", ret);
		ztest_test_fail();
		return;
	}

	ret = fs_write(&img, &c_img->header, sizeof(lv_img_header_t));
	if (ret < 0) {
		TC_PRINT("Failed to write image file header: %d\n", ret);
		ztest_test_fail();
		return;
	}

	ret = fs_write(&img, c_img->data, c_img->data_size);
	if (ret < 0) {
		TC_PRINT("Failed to write image file data: %d\n", ret);
		ztest_test_fail();
		return;
	}

	ret = fs_close(&img);
	if (ret < 0) {
		TC_PRINT("Failed to close image file: %d\n", ret);
		ztest_test_fail();
		return;
	}
}

void teardown_fs(void)
{
}

void test_main(void)
{

	ztest_test_suite(lvgl_screen, ztest_unit_test(test_get_default_screen),
			ztest_unit_test(test_add_delete_screen));
	ztest_test_suite(lvgl_fs, ztest_user_unit_test_setup_teardown(
				test_add_img, setup_fs, teardown_fs));

	ztest_run_test_suite(lvgl_screen);
	ztest_run_test_suite(lvgl_fs);
}
