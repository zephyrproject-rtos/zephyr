/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_FAT_FILESYSTEM_ELM)
#define TESTING_FAT
#elif defined(CONFIG_FILE_SYSTEM_LITTLEFS)
#define TESTING_LFS
#else
#error Either LittleFs or FAT FS test has to be selected
#endif

#include <zephyr.h>
#include <device.h>
#include <disk/disk_access.h>
#include <fs/fs.h>
#include <string.h>
#ifdef TESTING_LFS
#include <fs/littlefs.h>
#endif

#ifdef TESTING_FAT
#include <ff.h>
#endif

#define FLASH_MNT_POINT "NAND"
/* The value will be use to present how ticks relate to us */
#define TICKS_TO_US 100000


#ifdef TESTING_FAT
static FATFS fat_fs;
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.mnt_point = "/"FLASH_MNT_POINT":",
};
#endif

#ifdef TESTING_LFS
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(little_fs);
static struct fs_mount_t mp= {
	.type = FS_LITTLEFS,
	.fs_data = &little_fs,
	.storage_dev = (void *)FLASH_AREA_ID(storage),
	.mnt_point = "/"FLASH_MNT_POINT":",
};
#endif

static uint8_t buf[4096] = { 0 };
/*
 * File size is set by report_test_setup to half of the volume size. Reason for that is that
 * the volume size is partially consumed by file system structures.
 */
size_t file_size = 0;

void die_in_loop()
{
	printk("Idle looping, awaiting reboot");
	while (1) {
		k_sleep(K_MSEC(1000));
	}
}

#define PRINTK_CONFIG(var, format) printk("%s="format"\n", #var, var)
#define PRINTK_CONFIG_YN(var) printk("%s=%c\n", #var, IS_ENABLED(var) ? 'y' : 'n')

void report_test_setup()
{
	if (IS_ENABLED(CONFIG_FLASH_SIMULATOR)) {
		printk("Flash simulator in RAM has been enabled.\n");
		printk("All flash operations will be simulated in RAM.\n");
	}
#ifdef TESTING_LFS
	const struct flash_area *fa = NULL;
	if (flash_area_open(FLASH_AREA_ID(storage), &fa) < 0 || fa == NULL) {
		printk("Access to 'storage' failed\n");
		die_in_loop();
	}
	//flash_area_open(FLASH_AREA_ID(storage), &fa);
	printk("Configured for LittleFS\n");
	BUILD_ASSERT(IS_ENABLED(CONFIG_FLASH_MAP), "Flash map required");
	printk("Data taken from flash map for 'storage' partition:\n");
	printk(" fa_dev_name == %s\n", fa->fa_dev_name);
	printk(" fa_size == %u\n", fa->fa_size);
	printk(" fa_off == 0x%x\n", fa->fa_off);
	printk("Page sizes by offset:\n");

	file_size = fa->fa_size / 2;

	flash_area_close(fa);
#endif
#ifdef TESTING_FAT
	if (strcmp(FLASH_MNT_POINT, CONFIG_DISK_FLASH_VOLUME_NAME)) {
		printk("Tests require CONFIG_DISK_FLASH_VOLUME_NAME == %s, but it is %s\n",
		       FLASH_MNT_POINT, CONFIG_DISK_FLASH_VOLUME_NAME);
		die_in_loop();
	}
	printk("Configured for FATFS\n");
	BUILD_ASSERT(IS_ENABLED(CONFIG_DISK_ACCESS), "Disk access required");
	BUILD_ASSERT(IS_ENABLED(CONFIG_DISK_DRIVER_FLASH), "Disk to flash required");
	PRINTK_CONFIG(CONFIG_DISK_FLASH_DEV_NAME, "%s");
	PRINTK_CONFIG(CONFIG_DISK_FLASH_START, "0x%x");
	PRINTK_CONFIG(CONFIG_DISK_VOLUME_SIZE, "%d");
	PRINTK_CONFIG(CONFIG_DISK_ERASE_BLOCK_SIZE, "%d");
	PRINTK_CONFIG(CONFIG_DISK_FLASH_ERASE_ALIGNMENT, "%d");
	PRINTK_CONFIG(CONFIG_DISK_FLASH_MAX_RW_SIZE, "%d");
	PRINTK_CONFIG(CONFIG_DISK_FLASH_SECTOR_SIZE, "%d");
	PRINTK_CONFIG(CONFIG_FS_FATFS_MAX_SS, "%d");

	file_size = CONFIG_DISK_VOLUME_SIZE / 2;
#endif
	PRINTK_CONFIG_YN(CONFIG_SPEED_OPTIMIZATIONS);
	PRINTK_CONFIG_YN(CONFIG_SIZE_OPTIMIZATIONS);
	PRINTK_CONFIG_YN(CONFIG_TICKLESS_KERNEL);

	printk("Test file size is %d\n", file_size);
}



void main(void)
{
	struct fs_file_t f;
	int64_t start, end;

	const char *fname = "/"FLASH_MNT_POINT":/Hello.txt";
	/* Stabilize */
	k_sleep(K_MSEC(1000));

	start = k_uptime_ticks();
	k_sleep(K_MSEC(1000));
	end = k_uptime_ticks();
	printk("1s sleep took %llu ticks\n", (int64_t)end - start);
	printk("%d ticks take %lluus\n\n", TICKS_TO_US, (int64_t)k_cyc_to_us_floor64(TICKS_TO_US));

	report_test_setup();

	int ret = fs_mount(&mp);

	fs_file_t_init(&f);

	if (ret != 0) {
		printk("Error mounting disk.\n");
		printk("Failed to mount disk at %s with error %d\n", mp.mnt_point, ret);
		goto spin_me;
	}


	printk("== WRITE TESTS ==\n");
	printk("Bytes per buffer; Buffers; Ticks\n");
	for (int bib = 1; bib <= ARRAY_SIZE(buf); bib *= 2) {
		int bufs = file_size / bib;
		int i = 0;

		start = k_uptime_ticks();
		ret = fs_unlink(fname);
		end = k_uptime_ticks();
		if (unlikely(ret < 0)) {
			printk("File not found yet\n");
		}
		printk("fs_unlink %lld\n", end - start);

		start = k_uptime_ticks();
		ret = fs_open(&f, fname, FS_O_CREATE | FS_O_RDWR);
		end = k_uptime_ticks();
		printk("fs_open %lld\n", end - start);

		if (unlikely(ret != 0)) {
			printk("Write: Failed to open file with error %d\n", ret);
			goto spin_me;
		}

		start = k_uptime_ticks();
		while(1) {
			ret = fs_write(&f, &buf, bib);
			if (unlikely(ret < bib)) {
				printk("Write: Failed to write %d to file with error %d\b", i, ret);
				goto spin_me;
			}

			++i;
			if (unlikely(i >= bufs)) {
				break;
			}
		}

		ret = fs_close(&f);
		if (unlikely(ret < 0)) {
			printk("Write: Closing error %d\n", ret);
			goto spin_me;
		}
		end = k_uptime_ticks();
		printk("%d;%d;%lld\n", bib, bufs, (end - start));
	}
	printk("== READ TESTS ==\n");
	printk("Bytes per buffer; Buffers; Ticks\n");
	for (int bib = 1; bib <= ARRAY_SIZE(buf); bib *= 2) {
		int bufs = file_size / bib;
		int i = 0;

		start = k_uptime_ticks();
		ret = fs_open(&f, fname, FS_O_READ);
		end = k_uptime_ticks();
		printk("fs_open %lld\n", end - start);

		if (ret != 0) {
			printk("Read: Failed to open file with error %d\n", ret);
			goto spin_me;
		}

		start = k_uptime_ticks();
		while (1) {
			ret = fs_read(&f, &buf, bib);
			if (unlikely(ret < bib)) {
				printk("Read: Failed to read %d from file with error %d\b", i, ret);
				goto spin_me;
			}

			++i;
			if (unlikely(i >= bufs)) {
				break;
			}
		}

		ret = fs_close(&f);
		if (unlikely(ret < 0)) {
			printk("Read: Closing error %d\n", ret);
			goto spin_me;
		}

		end = k_uptime_ticks();
		printk("%d;%d;%lld\n", bib, bufs, (end - start));
	}

spin_me:
	die_in_loop();
}
