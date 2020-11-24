/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2019-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log.h>
#include <usb/usb_device.h>
#include <fs/fs.h>
#include <stdio.h>

LOG_MODULE_REGISTER(main);

#if CONFIG_DISK_ACCESS_FLASH
#include <storage/flash_map.h>
#define DISK_STORAGE_DEV	((void *)FLASH_AREA_ID(storage))
#else
#define DISK_STORAGE_DEV	(NULL)
#endif

#if CONFIG_FAT_FILESYSTEM_ELM
#include <ff.h>
#endif

#if CONFIG_FILE_SYSTEM_LITTLEFS
#include <fs/littlefs.h>
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
#endif

static struct fs_mount_t fs_mnt;

static int setup_flash(void *storage_dev)
{
	int rc = 0;
#if CONFIG_DISK_ACCESS_FLASH
	unsigned int id = (uintptr_t)storage_dev;
	const struct flash_area *pfa;

	rc = flash_area_open(id, &pfa);
	printk("Area %u at 0x%x on %s for %u bytes\n",
	       id, (unsigned int)pfa->fa_off, pfa->fa_dev_name,
	       (unsigned int)pfa->fa_size);

	if (rc != 0 && IS_ENABLED(CONFIG_APP_WIPE_STORAGE)) {
		printk("Erasing flash area ... ");
		rc = flash_area_erase(pfa, 0, pfa->fa_size);
		printk("%d\n", rc);
	}

	if (rc != 0) {
		flash_area_close(pfa);
	}
#endif
	return rc;
}

static int mount_fatfs(struct fs_mount_t *mnt)
{
	int rc = 0;
#if CONFIG_FAT_FILESYSTEM_ELM
	static FATFS fat_fs;

	mnt->type = FS_FATFS;

	if (IS_ENABLED(CONFIG_DISK_ACCESS_RAM)) {
		mnt->mnt_point = "/RAM:";
	} else {
		mnt->mnt_point = "/NAND:";
	}

	mnt->storage_dev = DISK_STORAGE_DEV;
	mnt->fs_data = &fat_fs;

	rc = fs_mount(mnt);
#endif
	return rc;
}

static int mount_littlefs(struct fs_mount_t *mnt)
{
	int rc = 0;
#if CONFIG_FILE_SYSTEM_LITTLEFS
	if (!IS_ENABLED(CONFIG_DISK_ACCESS_FLASH)) {
		return -ENOTSUP;
	}

	mnt->type = FS_LITTLEFS;
	mnt->mnt_point = "/lfs";
	mnt->storage_dev = DISK_STORAGE_DEV;
	mnt->fs_data = &storage;

	rc = fs_mount(mnt);
#endif
	return rc;
}

static void setup_disk(void)
{
	struct fs_mount_t *mp = &fs_mnt;
	int rc;

	if (!IS_ENABLED(CONFIG_DISK_ACCESS_RAM) &&
	     IS_ENABLED(CONFIG_DISK_ACCESS_FLASH)) {
		rc = setup_flash(mp->storage_dev);
		if (rc != 0) {
			LOG_ERR("Failed to setup flash area");
			return;
		}
	}

	if (IS_ENABLED(CONFIG_FAT_FILESYSTEM_ELM)) {
		rc = mount_fatfs(mp);
	} else if (IS_ENABLED(CONFIG_FILE_SYSTEM_LITTLEFS) &&
		   IS_ENABLED(CONFIG_DISK_ACCESS_FLASH)) {
		rc = mount_littlefs(mp);
	} else {
		return;
	}

	if (rc != 0) {
		LOG_ERR("Failed to mount filesystem");
		return;
	}

	/* Allow log messages to flush to avoid interleaved output */
	k_sleep(K_MSEC(50));

	printk("Mount %s: %d\n", fs_mnt.mnt_point, rc);

	struct fs_statvfs sbuf;

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

	struct fs_dir_t dir = { 0 };

	rc = fs_opendir(&dir, mp->mnt_point);
	printk("%s opendir: %d\n", mp->mnt_point, rc);

	while (rc >= 0) {
		struct fs_dirent ent = { 0 };

		rc = fs_readdir(&dir, &ent);
		if (rc < 0) {
			break;
		}
		if (ent.name[0] == 0) {
			printk("End of files\n");
			break;
		}
		printk("  %c %u %s\n",
		       (ent.type == FS_DIR_ENTRY_FILE) ? 'F' : 'D',
		       ent.size,
		       ent.name);
	}

	(void)fs_closedir(&dir);

	return;
}

void main(void)
{
	int ret;

	setup_disk();

	ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}

	LOG_INF("The device is put in USB mass storage mode.\n");
}
