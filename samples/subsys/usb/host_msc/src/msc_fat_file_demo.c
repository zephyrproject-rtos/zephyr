/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Simple FatFs on USB MSC: mount PC-formatted stick, list root, write nava.txt.
 */

#include "usb_host_dump.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <ff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/disk_access.h>

LOG_MODULE_REGISTER(msc_fat_file_demo, LOG_LEVEL_INF);

#if IS_ENABLED(CONFIG_USBH_MSC_DISK)
#define MSC_FAT_DISK_NAME CONFIG_USBH_MSC_DISK_NAME
#else
#define MSC_FAT_DISK_NAME CONFIG_USB_HOST_MSC_SAMPLE_DISK_NAME
#endif

#define MSC_FAT_DEMO_FILE_NAME     "nava.txt"
#define MSC_FAT_DEMO_FILE_CONTENT  CONFIG_USB_HOST_MSC_SAMPLE_FAT_FILE_CONTENT
#define MSC_FAT_DEMO_MOUNT_POINT   "/" MSC_FAT_DISK_NAME ":"
#define MSC_FAT_DEMO_ROOT_LIST_MAX 16

static FATFS fat_fs;
static struct fs_mount_t fat_mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};

static int msc_fat_list_root(const char *mount_point)
{
	struct fs_dir_t dir;
	struct fs_dirent ent;
	unsigned int count = 0;
	int ret;

	fs_dir_t_init(&dir);
	ret = fs_opendir(&dir, mount_point);
	if (ret != 0) {
		LOG_ERR("msc_fat: fs_opendir %s failed: %d", mount_point, ret);
		return ret;
	}

	LOG_INF("msc_fat: root listing %s", mount_point);
	while (count < MSC_FAT_DEMO_ROOT_LIST_MAX) {
		ret = fs_readdir(&dir, &ent);
		if (ret != 0) {
			LOG_ERR("msc_fat: fs_readdir failed: %d", ret);
			(void)fs_closedir(&dir);
			return ret;
		}
		if (ent.name[0] == '\0') {
			break;
		}

		LOG_INF("msc_fat:   %s %s (%zu B)",
			(ent.type == FS_DIR_ENTRY_DIR) ? "[DIR]" : "[FILE]", ent.name, ent.size);
		count++;
	}

	if (count >= MSC_FAT_DEMO_ROOT_LIST_MAX) {
		LOG_INF("msc_fat:   … (listing capped at %u entries)", MSC_FAT_DEMO_ROOT_LIST_MAX);
	}

	ret = fs_closedir(&dir);
	if (ret != 0) {
		LOG_WRN("msc_fat: fs_closedir failed: %d", ret);
	}

	return 0;
}

int usb_host_msc_sample_fat_mount_volume(void)
{
	int ret;

	ret = disk_access_ioctl(MSC_FAT_DISK_NAME, DISK_IOCTL_CTRL_INIT, NULL);
	if (ret != 0) {
		LOG_ERR("msc_fat: disk init failed: %d", ret);
		return ret;
	}

	fat_mp.mnt_point = MSC_FAT_DEMO_MOUNT_POINT;
	ret = fs_mount(&fat_mp);
	if (ret == 0) {
		return 0;
	}

	LOG_WRN("msc_fat: fs_mount %s failed: %d", MSC_FAT_DEMO_MOUNT_POINT, ret);

#if IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_FAT_MKFS_ON_FAIL)
	{
		static MKFS_PARM mkfs_cfg = {
			.fmt = FM_FAT32 | FM_SFD,
			.n_fat = 1,
			.align = 0,
			.n_root = CONFIG_FS_FATFS_MAX_ROOT_ENTRIES,
			.au_size = 0,
		};
		char disk_id[16];

		LOG_WRN("msc_fat: mkfs requested — formatting volume (destructive)");

		ret = snprintf(disk_id, sizeof(disk_id), "%s:", MSC_FAT_DISK_NAME);
		if (ret < 0 || ret >= (int)sizeof(disk_id)) {
			return -ENOMEM;
		}

		ret = fs_mkfs(FS_FATFS, (uintptr_t)disk_id, &mkfs_cfg, 0);
		if (ret != 0) {
			LOG_ERR("msc_fat: fs_mkfs failed: %d", ret);
			return ret;
		}

		ret = fs_mount(&fat_mp);
		if (ret != 0) {
			LOG_ERR("msc_fat: fs_mount after mkfs failed: %d", ret);
			return ret;
		}

		LOG_INF("msc_fat: mkfs + mount %s ok", MSC_FAT_DEMO_MOUNT_POINT);
		return 0;
	}
#else
	LOG_ERR("msc_fat: no mount (use PC-formatted FAT stick or enable "
		"USB_HOST_MSC_SAMPLE_FAT_MKFS_ON_FAIL)");
	return ret;
#endif
}

#if IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_FAT_READBACK_VERIFY)
static int msc_fat_readback_verify(const char *path)
{
	struct fs_file_t file;
	char buf[64];
	ssize_t nread;
	int ret;

	fs_file_t_init(&file);
	ret = fs_open(&file, path, FS_O_READ);
	if (ret != 0) {
		LOG_ERR("msc_fat: readback fs_open %s failed: %d", path, ret);
		return ret;
	}

	nread = fs_read(&file, buf, sizeof(buf) - 1U);
	ret = fs_close(&file);
	if (ret != 0) {
		LOG_ERR("msc_fat: readback fs_close failed: %d", ret);
		return ret;
	}
	if (nread < 0) {
		LOG_ERR("msc_fat: readback fs_read failed: %zd", nread);
		return (int)nread;
	}

	buf[(size_t)nread] = '\0';
	if ((size_t)nread != strlen(MSC_FAT_DEMO_FILE_CONTENT) ||
	    memcmp(buf, MSC_FAT_DEMO_FILE_CONTENT, (size_t)nread) != 0) {
		LOG_ERR("msc_fat: readback mismatch (got %zd bytes): \"%s\"", nread, buf);
		return -EIO;
	}

	LOG_INF("msc_fat: readback verify OK (%zd bytes): \"%s\"", nread, buf);
	return 0;
}
#endif

int usb_host_msc_sample_fat_file_demo(void)
{
	struct fs_file_t file;
	char path[64];
	int ret;
	ssize_t written;

	fs_file_t_init(&file);

	ret = usb_host_msc_sample_fat_mount_volume();
	if (ret != 0) {
		return ret;
	}

	LOG_INF("msc_fat: mounted %s", MSC_FAT_DEMO_MOUNT_POINT);

	ret = msc_fat_list_root(MSC_FAT_DEMO_MOUNT_POINT);
	if (ret != 0) {
		(void)fs_unmount(&fat_mp);
		return ret;
	}

	ret = snprintf(path, sizeof(path), "%s/%s", MSC_FAT_DEMO_MOUNT_POINT,
		       MSC_FAT_DEMO_FILE_NAME);
	if (ret < 0 || ret >= (int)sizeof(path)) {
		(void)fs_unmount(&fat_mp);
		return -ENOMEM;
	}

	ret = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
	if (ret != 0) {
		LOG_ERR("msc_fat: fs_open %s failed: %d", path, ret);
		(void)fs_unmount(&fat_mp);
		return ret;
	}

	written = fs_write(&file, MSC_FAT_DEMO_FILE_CONTENT, strlen(MSC_FAT_DEMO_FILE_CONTENT));
	if (written < 0) {
		LOG_ERR("msc_fat: fs_write failed: %zd", written);
		(void)fs_close(&file);
		(void)fs_unmount(&fat_mp);
		return (int)written;
	}

	ret = fs_close(&file);
	if (ret != 0) {
		LOG_ERR("msc_fat: fs_close failed: %d", ret);
		(void)fs_unmount(&fat_mp);
		return ret;
	}

	LOG_INF("msc_fat: wrote %s (%zu bytes): \"%s\"", MSC_FAT_DEMO_FILE_NAME,
		strlen(MSC_FAT_DEMO_FILE_CONTENT), MSC_FAT_DEMO_FILE_CONTENT);

#if IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_FAT_READBACK_VERIFY)
	ret = msc_fat_readback_verify(path);
	if (ret != 0) {
		(void)fs_unmount(&fat_mp);
		return ret;
	}
#endif

#if IS_ENABLED(CONFIG_USB_HOST_MSC_SAMPLE_FAT_RELIST_AFTER_WRITE)
	ret = msc_fat_list_root(MSC_FAT_DEMO_MOUNT_POINT);
	if (ret != 0) {
		(void)fs_unmount(&fat_mp);
		return ret;
	}
#endif

	(void)disk_access_ioctl(MSC_FAT_DISK_NAME, DISK_IOCTL_CTRL_SYNC, NULL);

	ret = fs_unmount(&fat_mp);
	if (ret != 0) {
		LOG_WRN("msc_fat: fs_unmount failed: %d", ret);
		return ret;
	}

	LOG_INF("msc_fat: FS_VALIDATE OK");

	return 0;
}

int usb_host_msc_sample_fat_unmount_volume(void)
{
	return fs_unmount(&fat_mp);
}

const char *usb_host_msc_sample_fat_mount_point(void)
{
	return MSC_FAT_DEMO_MOUNT_POINT;
}
