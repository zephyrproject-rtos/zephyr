/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2023 Husqvarna AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>
#include <ff.h>

#ifdef CONFIG_DISK_DRIVER_RAM
#define DISK_NAME CONFIG_DISK_RAM_VOLUME_NAME
#elif defined(CONFIG_DISK_DRIVER_FLASH)
#define DISK_NAME DT_PROP(DT_NODELABEL(test_disk), disk_name)
#elif defined(CONFIG_DISK_DRIVER_SDMMC)
#define DISK_NAME CONFIG_SDMMC_VOLUME_NAME
#elif defined(CONFIG_DISK_DRIVER_MMC)
#define DISK_NAME CONFIG_MMC_VOLUME_NAME
#else
#error "Failed to select DISK access type"
#endif

#define FATFS_MNTP	"/"DISK_NAME":"
#if defined(CONFIG_FS_FATFS_LFN)
#define TEST_FILE	FATFS_MNTP \
	"/testlongfilenamethatsmuchlongerthan8.3chars.text"
#else
#define TEST_FILE	FATFS_MNTP"/testfile.txt"
#endif /* CONFIG_FS_FATFS_LFN */
#define TEST_DIR	FATFS_MNTP"/testdir"
#define TEST_DIR_FILE	FATFS_MNTP"/testdir/testfile.txt"

extern struct fs_file_t filep;
extern const char test_str[];
/* FatFs work area */
extern FATFS fat_fs;

int check_file_dir_exists(const char *path);

void test_fat_mount(void);
void test_fat_unmount(void);
void test_fat_file(void);
void test_fat_dir(void);
void test_fat_fs(void);
void test_fat_rename(void);
#ifdef CONFIG_FS_FATFS_REENTRANT
void test_fat_file_reentrant(void);
#endif /* CONFIG_FS_FATFS_REENTRANT */
