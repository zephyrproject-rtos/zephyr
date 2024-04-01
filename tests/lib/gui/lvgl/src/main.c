/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "img.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/ztest.h>
#include <lvgl_zephyr.h>

#include <lvgl.h>

#ifdef CONFIG_FS_LITTLEFS_BLK_DEV

#ifdef CONFIG_DISK_DRIVER_SDMMC
#define DISK_NAME "SD"
#elif defined(CONFIG_DISK_DRIVER_MMC)
#define DISK_NAME "SD2"
#else
#error "No disk device defined, is your board supported?"
#endif /* CONFIG_DISK_DRIVER_SDMMC */

#define IMG_FILE_PATH "/"DISK_NAME":/img.bin"

struct fs_littlefs lfsfs;

static struct fs_mount_t mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &lfsfs,
	.storage_dev = (void *)DISK_NAME,
	.mnt_point = "/"DISK_NAME":",
	.flags = FS_MOUNT_FLAG_USE_DISK_ACCESS,
};

#else /* CONFIG_FS_LITTLEFS_BLK_DEV */

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
#endif /* CONFIG_FS_LITTLEFS_BLK_DEV */

static const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

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

	lv_obj_t *act_screen = lv_scr_act();

	zassert_equal_ptr(act_screen, new_screen, "New screen not active");

	lv_scr_load(default_screen);

	lv_obj_del(new_screen);

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
}

void *setup_lvgl(void)
{
	int ret;

#if CONFIG_LV_COLOR_DEPTH_1 == 1
	display_set_pixel_format(display_dev, PIXEL_FORMAT_MONO10);
#elif CONFIG_LV_COLOR_DEPTH_8 == 1 || CONFIG_LV_COLOR_DEPTH_24 == 1
	/* No 8bit display pixel format not supported */
	display_set_pixel_format(display_dev, PIXEL_FORMAT_RGB_888);
#elif CONFIG_LV_COLOR_DEPTH_16 == 1
	display_set_pixel_format(display_dev, PIXEL_FORMAT_RGB_565);
#elif CONFIG_LV_COLOR_DEPTH_32 == 1
	display_set_pixel_format(display_dev, PIXEL_FORMAT_ARGB_8888);
#else
#error "No display pixel format defined, is your board supported?"
#endif

	ret = lvgl_init();
	zassert_equal(ret, 0, "Failed to initialize lvgl");

	return NULL;
}

void *setup_fs(void)
{
	struct fs_file_t img;
	struct fs_dirent info;
	int ret;
	const lv_image_dsc_t *c_img = get_lvgl_img();

	setup_lvgl();

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

	ret = fs_write(&img, &c_img->header, sizeof(lv_image_header_t));
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

void teardown_lvgl(void *data)
{
	lv_deinit();
}

ZTEST_SUITE(lvgl_screen, NULL, setup_lvgl, NULL, NULL, teardown_lvgl);
ZTEST_SUITE(lvgl_fs, NULL, setup_fs, NULL, NULL, teardown_lvgl);
