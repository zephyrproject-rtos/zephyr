/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2019-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sample_usbd.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_msc.h>
#include <zephyr/fs/fs.h>
#include <stdio.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#if CONFIG_DISK_DRIVER_FLASH
#include <zephyr/storage/flash_map.h>
#endif

#if CONFIG_FAT_FILESYSTEM_ELM
#include <ff.h>
#define FAT_VOLUME_LABEL_MAX_LEN	11 /* Max characters in a FAT volume label */
#define FAT_MNT_POINT_MAX_LEN		11 /* Max characters in a FAT mount point */
#define FAT_MNT_POINT_LEN			5
#define FAT_SET_LABEL_CMD_MAX_CHARS		\
	(FAT_VOLUME_LABEL_MAX_LEN + FAT_MNT_POINT_MAX_LEN + 1) /* +1 for ':' */

#endif

#if CONFIG_FILE_SYSTEM_LITTLEFS
#include <zephyr/fs/littlefs.h>
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
#endif

#if !defined(CONFIG_DISK_DRIVER_FLASH) && \
	!defined(CONFIG_DISK_DRIVER_RAM) && \
	!defined(CONFIG_DISK_DRIVER_SDMMC)
#error No supported disk driver enabled
#endif

#define STORAGE_PARTITION		storage_partition
#define STORAGE_PARTITION_ID		FIXED_PARTITION_ID(STORAGE_PARTITION)

static struct fs_mount_t fs_mnt;

static struct usbd_context *sample_usbd;

#if CONFIG_DISK_DRIVER_RAM
USBD_DEFINE_MSC_LUN(ram, "RAM", "Zephyr", "RAMDisk", "0.00");
#endif

#if CONFIG_DISK_DRIVER_FLASH
USBD_DEFINE_MSC_LUN(nand, "NAND", "Zephyr", "FlashDisk", "0.00");
#endif

#if CONFIG_DISK_DRIVER_SDMMC
USBD_DEFINE_MSC_LUN(sd, "SD", "Zephyr", "SD", "0.00");
#endif

static int setup_flash(struct fs_mount_t *mnt)
{
	int rc = 0;
#if CONFIG_DISK_DRIVER_FLASH
	unsigned int id;
	const struct flash_area *pfa;

	mnt->storage_dev = (void *)STORAGE_PARTITION_ID;
	id = STORAGE_PARTITION_ID;

	rc = flash_area_open(id, &pfa);
	printk("Area %u at 0x%x on %s for %u bytes\n",
	       id, (unsigned int)pfa->fa_off, pfa->fa_dev->name,
	       (unsigned int)pfa->fa_size);

	if (rc < 0 && IS_ENABLED(CONFIG_APP_WIPE_STORAGE)) {
		printk("Erasing flash area ... ");
		rc = flash_area_flatten(pfa, 0, pfa->fa_size);
		printk("%d\n", rc);
	}

	if (rc < 0) {
		flash_area_close(pfa);
	}
#endif
	return rc;
}

static int mount_app_fs(struct fs_mount_t *mnt)
{
	int rc;

#if CONFIG_FAT_FILESYSTEM_ELM
	static FATFS fat_fs;

	mnt->type = FS_FATFS;
	mnt->fs_data = &fat_fs;
	if (IS_ENABLED(CONFIG_DISK_DRIVER_RAM)) {
		mnt->mnt_point = "/RAM:";
	} else if (IS_ENABLED(CONFIG_DISK_DRIVER_SDMMC)) {
		mnt->mnt_point = "/SD:";
	} else {
		mnt->mnt_point = "/NAND:";
	}

#elif CONFIG_FILE_SYSTEM_LITTLEFS
	mnt->type = FS_LITTLEFS;
	mnt->mnt_point = "/lfs";
	mnt->fs_data = &storage;
#endif
	rc = fs_mount(mnt);

	return rc;
}

static void setup_disk(void)
{
	struct fs_mount_t *mp = &fs_mnt;
	struct fs_dir_t dir;
	struct fs_statvfs sbuf;
	int rc;

	fs_dir_t_init(&dir);

	if (IS_ENABLED(CONFIG_DISK_DRIVER_FLASH)) {
		rc = setup_flash(mp);
		if (rc < 0) {
			LOG_ERR("Failed to setup flash area");
			return;
		}
	}

	if (!IS_ENABLED(CONFIG_FILE_SYSTEM_LITTLEFS) &&
	    !IS_ENABLED(CONFIG_FAT_FILESYSTEM_ELM)) {
		LOG_INF("No file system selected");
		return;
	}

	rc = mount_app_fs(mp);
	if (rc < 0) {
		LOG_ERR("Failed to mount filesystem");
		return;
	}

	/* Allow log messages to flush to avoid interleaved output */
	k_sleep(K_MSEC(50));

	printk("Mount %s: %d\n", fs_mnt.mnt_point, rc);

#if CONFIG_FAT_FILESYSTEM_ELM
	/* Set volume label to replace "NO NAME" default */
	char fatfs_setlabel_cmd[FAT_SET_LABEL_CMD_MAX_CHARS];
	char fatfs_getlabel_result[FAT_VOLUME_LABEL_MAX_LEN];
	char vol_id[FAT_MNT_POINT_LEN];
	DWORD vsn;
	const char *mnt = mp->mnt_point;
	FRESULT res;

	/* Extract volume ID by skipping leading '/' and trailing ':' */
	if (mnt[0] == '/') {
		mnt++;
	}
	size_t len = strlen(mnt);

	if (len > 0 && mnt[len - 1] == ':') {
		len--;
	}
	if (len >= sizeof(vol_id)) {
		len = sizeof(vol_id) - 1;
	}

	strncpy(vol_id, mnt, len);
	vol_id[len] = '\0';

	/* Format: "MNT_POINT:LABEL" (e.g., "NAND:ZEPHYR_NAND") */
	snprintf(fatfs_setlabel_cmd, sizeof(fatfs_setlabel_cmd), "%s:ZEPHYR_%s", vol_id, vol_id);
	LOG_INF("Attempting to set volume label: %s (vol_id='%s')", fatfs_setlabel_cmd, vol_id);

	res = f_setlabel(fatfs_setlabel_cmd);
	if (res != FR_OK) {
		LOG_WRN("Failed to set volume label: error %d", res);
		goto label_done;
	}

	LOG_INF("Volume label successfully set to: %s", fatfs_setlabel_cmd);

	res = f_getlabel(vol_id, fatfs_getlabel_result, &vsn);
	if (res != FR_OK) {
		LOG_WRN("Failed to get volume label: error %d", res);
		goto label_done;
	}

	LOG_INF("Verified volume label: '%s' (VSN: 0x%08X)", fatfs_getlabel_result,
		(unsigned int)vsn);

label_done:
#endif

	rc = fs_statvfs(mp->mnt_point, &sbuf);
	if (rc < 0) {
		printk("FAIL: statvfs: %d\n", rc);
		return;
	}

	printk("%s: bsize = %lu ; frsize = %lu ;"
	       " blocks = %lu ; bfree = %lu\n",
	       mp->mnt_point,
	       sbuf.f_bsize, sbuf.f_frsize,
	       sbuf.f_blocks, sbuf.f_bfree);

	rc = fs_opendir(&dir, mp->mnt_point);
	printk("%s opendir: %d\n", mp->mnt_point, rc);

	if (rc < 0) {
		LOG_ERR("Failed to open directory");
	}

	while (rc >= 0) {
		struct fs_dirent ent = { 0 };

		rc = fs_readdir(&dir, &ent);
		if (rc < 0) {
			LOG_ERR("Failed to read directory entries");
			break;
		}
		if (ent.name[0] == 0) {
			printk("End of files\n");
			break;
		}
		printk("  %c %zu %s\n",
		       (ent.type == FS_DIR_ENTRY_FILE) ? 'F' : 'D',
		       ent.size,
		       ent.name);
	}

	(void)fs_closedir(&dir);

	return;
}

int main(void)
{
	int ret;

	setup_disk();

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	ret = usbd_enable(sample_usbd);
	if (ret) {
		LOG_ERR("Failed to enable device support");
		return ret;
	}

	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return 0;
	}

	LOG_INF("The device is put in USB mass storage mode");

	return 0;
}
