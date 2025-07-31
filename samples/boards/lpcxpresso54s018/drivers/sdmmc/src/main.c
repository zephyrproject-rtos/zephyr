/*
 * Copyright (c) 2024 VCI Development
 * SPDX-License-Identifier: Apache-2.0
 *
 * SD/MMC card sample application for LPC54S018
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <ff.h>
#include <stdio.h>
#include <string.h>

LOG_MODULE_REGISTER(sdmmc_sample, LOG_LEVEL_INF);

/* Mount point for SD card */
#define DISK_MOUNT_PT "/SD:"
#define TEST_FILE     DISK_MOUNT_PT"/test.txt"
#define TEST_DIR      DISK_MOUNT_PT"/testdir"

static FATFS fat_fs;
static struct fs_mount_t mount_point = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.mnt_point = DISK_MOUNT_PT,
};

static int test_file_operations(void)
{
	struct fs_file_t file;
	int ret;
	char write_buf[] = "Hello from LPC54S018!\r\nThis is a test of SD card functionality.\r\n";
	char read_buf[100];
	
	fs_file_t_init(&file);
	
	/* Create and write to a file */
	LOG_INF("Creating file: %s", TEST_FILE);
	ret = fs_open(&file, TEST_FILE, FS_O_CREATE | FS_O_WRITE);
	if (ret < 0) {
		LOG_ERR("Failed to create file: %d", ret);
		return ret;
	}
	
	LOG_INF("Writing data to file...");
	ret = fs_write(&file, write_buf, strlen(write_buf));
	if (ret < 0) {
		LOG_ERR("Failed to write to file: %d", ret);
		fs_close(&file);
		return ret;
	}
	LOG_INF("Wrote %d bytes", ret);
	
	fs_close(&file);
	
	/* Read back the file */
	LOG_INF("Reading file: %s", TEST_FILE);
	ret = fs_open(&file, TEST_FILE, FS_O_READ);
	if (ret < 0) {
		LOG_ERR("Failed to open file for reading: %d", ret);
		return ret;
	}
	
	memset(read_buf, 0, sizeof(read_buf));
	ret = fs_read(&file, read_buf, sizeof(read_buf) - 1);
	if (ret < 0) {
		LOG_ERR("Failed to read from file: %d", ret);
		fs_close(&file);
		return ret;
	}
	
	LOG_INF("Read %d bytes: %s", ret, read_buf);
	fs_close(&file);
	
	return 0;
}

static int test_directory_operations(void)
{
	struct fs_dir_t dir;
	struct fs_dirent entry;
	int ret;
	
	fs_dir_t_init(&dir);
	
	/* Create a directory */
	LOG_INF("Creating directory: %s", TEST_DIR);
	ret = fs_mkdir(TEST_DIR);
	if (ret < 0 && ret != -EEXIST) {
		LOG_ERR("Failed to create directory: %d", ret);
		return ret;
	}
	
	/* List root directory */
	LOG_INF("Listing root directory:");
	ret = fs_opendir(&dir, DISK_MOUNT_PT);
	if (ret < 0) {
		LOG_ERR("Failed to open directory: %d", ret);
		return ret;
	}
	
	while (1) {
		ret = fs_readdir(&dir, &entry);
		if (ret < 0) {
			LOG_ERR("Failed to read directory: %d", ret);
			break;
		}
		
		if (entry.name[0] == 0) {
			break;
		}
		
		LOG_INF("  %s%s (size: %zu)",
			entry.name,
			(entry.type == FS_DIR_ENTRY_DIR) ? "/" : "",
			entry.size);
	}
	
	fs_closedir(&dir);
	
	return 0;
}

static int get_disk_info(void)
{
	uint64_t total_size = 0;
	uint64_t free_size = 0;
	int ret;
	
	/* Get volume statistics */
	struct fs_statvfs stats;
	ret = fs_statvfs(DISK_MOUNT_PT, &stats);
	if (ret < 0) {
		LOG_ERR("Failed to get volume stats: %d", ret);
		return ret;
	}
	
	total_size = (uint64_t)stats.f_blocks * stats.f_frsize;
	free_size = (uint64_t)stats.f_bfree * stats.f_frsize;
	
	LOG_INF("Disk info:");
	LOG_INF("  Block size: %lu bytes", stats.f_bsize);
	LOG_INF("  Total blocks: %lu", stats.f_blocks);
	LOG_INF("  Free blocks: %lu", stats.f_bfree);
	LOG_INF("  Total size: %llu MB", total_size / (1024 * 1024));
	LOG_INF("  Free size: %llu MB", free_size / (1024 * 1024));
	LOG_INF("  Used: %llu%%", 
		((total_size - free_size) * 100) / total_size);
	
	return 0;
}

int main(void)
{
	int ret;
	
	LOG_INF("SD/MMC Sample Application");
	LOG_INF("Insert an SD card to begin...");
	
	/* Wait for SD card to be detected */
	do {
		ret = disk_access_init("SD");
		if (ret != 0) {
			LOG_INF("Waiting for SD card...");
			k_msleep(1000);
		}
	} while (ret != 0);
	
	LOG_INF("SD card detected!");
	
	/* Get disk status */
	ret = disk_access_status("SD");
	if (ret != 0) {
		LOG_ERR("Failed to get disk status: %d", ret);
		return -1;
	}
	
	/* Mount the file system */
	LOG_INF("Mounting file system...");
	ret = fs_mount(&mount_point);
	if (ret < 0) {
		LOG_ERR("Failed to mount file system: %d", ret);
		return -1;
	}
	LOG_INF("File system mounted successfully!");
	
	/* Get disk information */
	get_disk_info();
	
	/* Test file operations */
	LOG_INF("\n=== Testing file operations ===");
	ret = test_file_operations();
	if (ret < 0) {
		LOG_ERR("File operations test failed");
	}
	
	/* Test directory operations */
	LOG_INF("\n=== Testing directory operations ===");
	ret = test_directory_operations();
	if (ret < 0) {
		LOG_ERR("Directory operations test failed");
	}
	
	/* Keep the file system mounted */
	LOG_INF("\n=== SD card operations complete ===");
	LOG_INF("You can now safely remove the SD card");
	
	while (1) {
		/* Check if card is still present */
		ret = disk_access_status("SD");
		if (ret != 0) {
			LOG_WRN("SD card removed!");
			
			/* Unmount file system */
			fs_unmount(&mount_point);
			
			/* Wait for card to be inserted again */
			LOG_INF("Insert SD card to continue...");
			do {
				k_msleep(1000);
				ret = disk_access_init("SD");
			} while (ret != 0);
			
			/* Remount */
			LOG_INF("SD card detected! Remounting...");
			ret = fs_mount(&mount_point);
			if (ret < 0) {
				LOG_ERR("Failed to remount: %d", ret);
			} else {
				LOG_INF("Remounted successfully!");
				get_disk_info();
			}
		}
		
		k_msleep(1000);
	}
	
	return 0;
}