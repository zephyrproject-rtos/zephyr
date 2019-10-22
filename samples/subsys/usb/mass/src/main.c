/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Sample to put the device in USB mass storage mode backed on a 16k RAMDisk. */

#include <zephyr.h>
#include <logging/log.h>
#ifndef CONFIG_USB_DEVICE_AUTO_ENABLE
#include <usb/usb_device.h>
#endif
LOG_MODULE_REGISTER(main);

#if CONFIG_DISK_ACCESS_FLASH && CONFIG_FAT_FILESYSTEM_ELM
#include <fs/fs.h>
#include <ff.h>

static FATFS fat_fs;
#define FATFS_MNTP	"/NAND:"

static struct fs_mount_t fatfs_mnt = {
	.type = FS_FATFS,
	.mnt_point = FATFS_MNTP,
	.fs_data = &fat_fs,
};
#endif

void main(void)
{
#ifndef CONFIG_USB_DEVICE_AUTO_ENABLE
	int ret;

	ret = usb_enable();
	if (ret < 0) {
		LOG_ERR("Failed to enable USB device");
	return;
	}
#endif

	LOG_INF("The device is put in USB mass storage mode.\n");

#if CONFIG_DISK_ACCESS_FLASH && CONFIG_FAT_FILESYSTEM_ELM
	int res = fs_mount(&fatfs_mnt);

	if (res < 0) {
		LOG_ERR("Mount failed.");
	}
#endif
}

