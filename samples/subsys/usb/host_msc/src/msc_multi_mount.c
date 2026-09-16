/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Independent FatFs mounts for multiple USB MSC volumes. The stock ``fs mount fat``
 * shell command tracks only one FAT mount; use ``msc mount USB`` / ``msc mount USB1``
 * here so each stick can stay mounted concurrently.
 */

#include "msc_multi_mount.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <ff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usb_msc_disk.h>

LOG_MODULE_REGISTER(msc_multi_mount, LOG_LEVEL_INF);

#ifndef CONFIG_USBH_MSC_LUN_SLOTS
#define MSC_FS_VOL_SLOTS 4
#else
#define MSC_FS_VOL_SLOTS CONFIG_USBH_MSC_LUN_SLOTS
#endif

struct msc_fs_vol {
	bool mounted;
	char disk_name[16];
	char mnt_point[20];
	FATFS fat_fs;
	struct fs_mount_t mp;
};

static struct msc_fs_vol msc_vols[MSC_FS_VOL_SLOTS];

static struct msc_fs_vol *msc_fs_find_disk(const char *disk_name)
{
	for (size_t i = 0U; i < ARRAY_SIZE(msc_vols); i++) {
		if (msc_vols[i].mounted &&
		    strncmp(msc_vols[i].disk_name, disk_name, sizeof(msc_vols[i].disk_name)) == 0) {
			return &msc_vols[i];
		}
	}

	return NULL;
}

static struct msc_fs_vol *msc_fs_find_free(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(msc_vols); i++) {
		if (!msc_vols[i].mounted) {
			return &msc_vols[i];
		}
	}

	return NULL;
}

static int msc_fs_disk_init(const char *disk_name)
{
	int err;

	err = disk_access_ioctl(disk_name, DISK_IOCTL_CTRL_INIT, NULL);
	if (err != 0) {
		LOG_ERR("Disk \"%s\" init failed: %d", disk_name, err);
	}

	return err;
}

static void msc_fs_vol_clear(struct msc_fs_vol *vol)
{
	int err;

	if (vol == NULL || !vol->mounted) {
		if (vol != NULL) {
			(void)memset(vol, 0, sizeof(*vol));
		}
		return;
	}

	err = fs_unmount_path(vol->mnt_point);
	if (err != 0) {
		LOG_WRN("MSC FAT unmount %s failed: %d (dropping slot)", vol->mnt_point, err);
	} else {
		LOG_INF("MSC FAT unmounted %s", vol->mnt_point);
	}

	(void)memset(vol, 0, sizeof(*vol));
}

int msc_fs_mount_disk(const char *disk_name)
{
	struct msc_fs_vol *vol;
	struct fs_dirent ent;
	int err;

	if (disk_name == NULL || disk_name[0] == '\0') {
		return -EINVAL;
	}

	vol = msc_fs_find_disk(disk_name);
	if (vol != NULL) {
		if (fs_stat(vol->mnt_point, &ent) == 0) {
			return -EALREADY;
		}

		LOG_WRN("MSC FAT stale mount on %s — remounting", vol->mnt_point);
		msc_fs_vol_clear(vol);
	}

	vol = msc_fs_find_free();
	if (vol == NULL) {
		return -ENOMEM;
	}

	err = msc_fs_disk_init(disk_name);
	if (err != 0) {
		return err;
	}

	(void)strncpy(vol->disk_name, disk_name, sizeof(vol->disk_name) - 1U);
	vol->disk_name[sizeof(vol->disk_name) - 1U] = '\0';
	(void)snprintk(vol->mnt_point, sizeof(vol->mnt_point), "/%s:", disk_name);

	vol->mp.type = FS_FATFS;
	vol->mp.fs_data = &vol->fat_fs;
	vol->mp.mnt_point = vol->mnt_point;
	vol->mp.storage_dev = vol->disk_name;

	err = fs_mount(&vol->mp);
	if (err != 0) {
		(void)memset(vol, 0, sizeof(*vol));
		return err;
	}

	vol->mounted = true;
	LOG_INF("MSC FAT mounted at %s", vol->mnt_point);

	return 0;
}

int msc_fs_unmount_disk(const char *disk_name)
{
	struct msc_fs_vol *vol;

	if (disk_name == NULL || disk_name[0] == '\0') {
		return -EINVAL;
	}

	vol = msc_fs_find_disk(disk_name);
	if (vol == NULL) {
		return -ENOENT;
	}

	msc_fs_vol_clear(vol);

	return 0;
}

void msc_fs_unmount_udev(struct usb_device *udev)
{
	char disk_name[16];

	if (udev == NULL) {
		return;
	}

	for (uint8_t lun = 0U; lun < 16U; lun++) {
		if (usb_msc_disk_get_volume_name(udev, lun, disk_name, sizeof(disk_name)) >= 0) {
			(void)msc_fs_unmount_disk(disk_name);
		}
	}
}

static void msc_fs_strip_disk_name(const char *arg, char *disk_name, size_t buflen)
{
	const char *start = arg;

	if (arg == NULL || disk_name == NULL || buflen == 0U) {
		return;
	}

	if (start[0] == '/') {
		start++;
	}

	(void)strncpy(disk_name, start, buflen - 1U);
	disk_name[buflen - 1U] = '\0';

	char *colon = strchr(disk_name, ':');

	if (colon != NULL) {
		*colon = '\0';
	}
}

static int cmd_msc_mount(const struct shell *sh, size_t argc, char **argv)
{
	char disk_name[16];
	int err;

	ARG_UNUSED(argc);

	msc_fs_strip_disk_name(argv[1], disk_name, sizeof(disk_name));
	err = msc_fs_mount_disk(disk_name);
	if (err == -EALREADY) {
		shell_print(sh, "/%s: already mounted", disk_name);
		return 0;
	}
	if (err != 0) {
		shell_error(sh, "msc mount %s failed: %d", disk_name, err);
		return err;
	}

	shell_print(sh, "Mounted /%s: — use fs ls /%s:", disk_name, disk_name);
	return 0;
}

static int cmd_msc_umount(const struct shell *sh, size_t argc, char **argv)
{
	char disk_name[16];
	int err;

	ARG_UNUSED(argc);

	msc_fs_strip_disk_name(argv[1], disk_name, sizeof(disk_name));
	err = msc_fs_unmount_disk(disk_name);
	if (err == -ENOENT) {
		shell_warn(sh, "/%s: not mounted", disk_name);
		return 0;
	}
	if (err != 0) {
		shell_error(sh, "msc umount %s failed: %d", disk_name, err);
		return err;
	}

	shell_print(sh, "Unmounted /%s:", disk_name);
	return 0;
}

static int cmd_msc_mounts(const struct shell *sh, size_t argc, char **argv)
{
	unsigned int count = 0U;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	for (size_t i = 0U; i < ARRAY_SIZE(msc_vols); i++) {
		if (msc_vols[i].mounted) {
			shell_print(sh, "  %s", msc_vols[i].mnt_point);
			count++;
		}
	}

	if (count == 0U) {
		shell_print(sh, "No MSC FAT volumes mounted (try: msc mount USB)");
	}

	return 0;
}

static int cmd_msc_write(const struct shell *sh, size_t argc, char **argv)
{
	struct fs_file_t file;
	char text[256];
	size_t len = 0U;
	int err;

	if (argc < 3) {
		shell_error(sh, "Usage: msc write <path> <text...>");
		return -EINVAL;
	}

	for (int i = 2; i < (int)argc; i++) {
		if (i > 2 && len + 1U < sizeof(text)) {
			text[len++] = ' ';
		}

		const size_t arg_len = strlen(argv[i]);
		const size_t copy = MIN(arg_len, sizeof(text) - len - 1U);

		if (copy == 0U) {
			break;
		}

		memcpy(text + len, argv[i], copy);
		len += copy;
	}
	text[len] = '\0';

	fs_file_t_init(&file);
	err = fs_open(&file, argv[1], FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC);
	if (err != 0) {
		shell_error(sh, "msc write: open %s failed: %d (is volume mounted?)", argv[1], err);
		return err;
	}

	err = fs_write(&file, text, len);
	if (err < 0) {
		shell_error(sh, "msc write: write failed: %d", err);
		(void)fs_close(&file);
		return err;
	}

	for (int pass = 0; pass < 5; pass++) {
		err = fs_sync(&file);
		if (err == 0) {
			break;
		}
		if (err != -EIO && err != -EAGAIN) {
			break;
		}
		k_msleep(50 * (pass + 1));
	}

	err = fs_close(&file);
	if (err != 0) {
		shell_warn(sh, "msc write: close failed: %d — data may still be readable", err);
		shell_print(sh, "Wrote %u bytes to %s (close incomplete)", (unsigned int)len,
			    argv[1]);
		return 0;
	}

	shell_print(sh, "Wrote %u bytes to %s", (unsigned int)len, argv[1]);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	msc_subcmds,
	SHELL_CMD_ARG(mount, NULL, "Mount MSC volume <USB|USB1|/USB:>", cmd_msc_mount, 2, 0),
	SHELL_CMD_ARG(umount, NULL, "Unmount MSC volume <USB|USB1|/USB:>", cmd_msc_umount, 2, 0),
	SHELL_CMD(mounts, NULL, "List MSC FAT mount points", cmd_msc_mounts),
	SHELL_CMD_ARG(write, NULL, "Write text file <path> <text...>", cmd_msc_write, 3, 255),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(msc, &msc_subcmds, "USB MSC multi-volume FatFs mount", NULL);
