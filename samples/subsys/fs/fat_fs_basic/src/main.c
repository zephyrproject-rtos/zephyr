/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is very basic sample that demonstrates how the to use SoC internal flash for FAT FS
 * file system.
 * The sample will make FAT FS at first run and create file to which it will write single byte
 * increamenting it at each reboot.
 *
 * Note that, in case when using SoC flash for FAT, the system somehow needs to be created so
 * if an appication of only means to do so then the CONFIG_FS_FATFS_MOUNT_MKFS=y Kconfig
 * option is required, this will enable application to create FAT in designated area on first run.
 * The CONFIG_FS_FATFS_READ_ONLY=n is required as read-only systems prevent usage of
 * CONFIG_FS_FATFS_MOUNT_MKFS.
 */
#include <zephyr.h>
#include <device.h>
#include <disk/disk_access.h>
#include <logging/log.h>
#include <fs/fs.h>
#include <ff.h>

LOG_MODULE_REGISTER(main);

/*
 * This object, or buffer of its size, is needed by the FAT driver to store internal objects;
 * it will be given the driver via mount point definition, structure of type fs_mount_t.
 * User should avoiding accessing it and care must be taken for the structure to be defined
 * for the entire time when FAT FS is mounted.
 * Each mounted FAT file system requires separate FATFS object.
 */
static FATFS fat_fs;

/*
 * The correct format for mount point, for FAT FS, is "/<dir>:" where dir is 0-9 digit or one of
 * strings listed by preprocesor macro _VOLUME_STRS in FAT ELM configuration,
 * modules/fs/fatfs/include/ffconf.h.
 * In this sample the <dir> is taken from Kconfig option CONFIG_DISK_FLASH_VOLUME_NAME.
 */
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.mnt_point = "/"CONFIG_DISK_FLASH_VOLUME_NAME":",
};


void main(void)
{
	struct fs_file_t f;
	uint8_t b = 0;
	/*
	 * Notice the additional '/' in path of file name, after the ':'; the FAT fs is special that
	 * it requires mount points to end with ':', and as the mount point serves as virtual
	 * directory in structure the '/' before next path element is required, for example:
	 *  /RAM:/Hello.txt
	 * is correct, but attempting to open
	 *  /RAM:Hello.txt
	 * will result in error -2 and information that mount point does not exist.
	 */
	const char *fname = "/"CONFIG_DISK_FLASH_VOLUME_NAME":/Hello.txt";

	/*
	 * Use the fs_mount_t object mp to mount file system
	 */
	int res = fs_mount(&mp);
	/* Initialize fs_file_t type object before first use */
	fs_file_t_init(&f);

	if (res == 0) {
		printk("Successfully mounted FAT at %s\n", mp.mnt_point);
	} else {
		printk("Error mounting disk.\n");
		printk("Failed to mount disk at %s with error %d\n", mp.mnt_point, res);
		goto spin_me;
	}

	/* Open or create the file */
	res = fs_open(&f, fname, FS_O_CREATE | FS_O_RDWR);
	if (res != 0) {
		printk("Failed to open file with error %d\n", res);
		goto spin_me;
	}

	res = fs_read(&f, &b, 1);
	if (res == 0) {
		printk("Nothing to read yet\n");
	} else if (res == 1) {
		printk("One byte read %x\n", (int)b);
		++b;
	} else if (res < 0) {
		printk("Failed to read file with error %d\n", res);
		goto spin_me;
	}

	/* Rewind file */
	res = fs_seek(&f, 0, FS_SEEK_SET);
	if (res < 0) {
		printk("Failed to rewind file with error %d\n", res);
		goto spin_me;
	}

	printk("Write to file %x\n", (int)b);

	res = fs_write(&f, &b, 1);
	if (res < 0) {
		printk("Failed to write to file with error %d\b", res);
		goto spin_me;
	}
	fs_sync(&f);

spin_me:
	/* Can close unopened file with no error */
	fs_close(&f);

	if (res != 0) {
		printk("Halting");
	} else {
		printk("Idle looping, awaiting reboot");

	}
	while (1) {
		k_sleep(K_MSEC(1000));
	}
}
