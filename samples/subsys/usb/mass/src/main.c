/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Sample to put the device in USB mass storage mode backed on a 16k RAMDisk. */

#include <zephyr.h>
#include <logging/log.h>
#include <usb/usb_device.h>
#include <stdio.h>

LOG_MODULE_REGISTER(main);

#if CONFIG_DISK_ACCESS_FLASH
#include <storage/flash_map.h>
#if CONFIG_FAT_FILESYSTEM_ELM
#include <fs/fs.h>
#include <ff.h>

static FATFS fat_fs;
#define FATFS_MNTP      "/NAND:"

static struct fs_mount_t fs_mnt = {
	.type = FS_FATFS,
	.mnt_point = FATFS_MNTP,
	.storage_dev = (void *)FLASH_AREA_ID(storage),
	.fs_data = &fat_fs,
};
#elif CONFIG_FILE_SYSTEM_LITTLEFS
#include <fs/fs.h>
#include <fs/littlefs.h>

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
static struct fs_mount_t fs_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &storage,
	.storage_dev = (void *)FLASH_AREA_ID(storage),
	.mnt_point = "/lfs",
};
#else
#error No recognized file system
#endif  /* file system */
#endif  /* CONFIG_DISK_ACCESS_FLASH */

static void setup_disk(void)
{
#if CONFIG_DISK_ACCESS_FLASH
	struct fs_mount_t *mp = &fs_mnt;
	unsigned int id = (uintptr_t)mp->storage_dev;
	int rc;
	const struct flash_area *pfa;

	rc = flash_area_open(id, &pfa);
	printk("Area %u at 0x%x on %s for %u bytes\n",
	       id, (unsigned int)pfa->fa_off, pfa->fa_dev_name,
	       (unsigned int)pfa->fa_size);

	if (IS_ENABLED(CONFIG_APP_WIPE_STORAGE)) {
		printk("Erasing flash area ... ");
		rc = flash_area_erase(pfa, 0, pfa->fa_size);
		printk("%d\n", rc);
	}

	rc = fs_mount(&fs_mnt);

	/* Allow log messages to flush to avoid interleaved output */
	k_sleep(K_MSEC(50));

	printk("Mount %s: %d\n", fs_mnt.mnt_point, rc);

	if (rc >= 0) {
		struct fs_statvfs sbuf;

		rc = fs_statvfs(mp->mnt_point, &sbuf);
		if (rc < 0) {
			printk("FAIL: statvfs: %d\n", rc);
			goto out;
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
	}

out:
	flash_area_close(pfa);
#endif
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
