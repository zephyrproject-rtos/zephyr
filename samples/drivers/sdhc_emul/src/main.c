/*
 * Copyright 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <ff.h>
#include <stdio.h>
#include <string.h>

#define DISK_DRIVE_NAME "SD"
#define DISK_MOUNT_PT "/"DISK_DRIVE_NAME":"
#define TEST_FILE DISK_MOUNT_PT"/hello.txt"

static FATFS fat_fs;
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.mnt_point = DISK_MOUNT_PT,
};

int main(void)
{
	struct fs_file_t file;
	int res;
	const char *test_str = "Zephyr SDHC emulator OK\n";
	char read_buf[64] = {0};

	printf("SDHC Emulator Sample\n");

	fs_file_t_init(&file);

	/* Formatting the disk */
	printf("Formatting disk...\n");
	res = fs_mkfs(FS_FATFS, (uintptr_t)DISK_DRIVE_NAME":", NULL, 0);
	if (res != 0) {
		printf("Error formatting disk: %d\n", res);
		printf("FAIL\n");
		return 0;
	}

	/* Mount */
	res = fs_mount(&mp);
	if (res != 0) {
		printf("Error mounting disk: %d\n", res);
		printf("FAIL\n");
		return 0;
	}
	printf("Disk mounted.\n");

	/* Write File */
	res = fs_open(&file, TEST_FILE, FS_O_CREATE | FS_O_WRITE);
	if (res != 0) {
		printf("Error opening file for write: %d\n", res);
		goto unmount;
	}

	res = fs_write(&file, test_str, strlen(test_str));
	if (res < 0) {
		printf("Error writing to file: %d\n", res);
		fs_close(&file);
		goto unmount;
	}
	fs_close(&file);
	printf("Wrote to file %s\n", TEST_FILE);

	/* Read File */
	res = fs_open(&file, TEST_FILE, FS_O_READ);
	if (res != 0) {
		printf("Error opening file for read: %d\n", res);
		goto unmount;
	}

	res = fs_read(&file, read_buf, sizeof(read_buf) - 1);
	if (res < 0) {
		printf("Error reading from file: %d\n", res);
		fs_close(&file);
		goto unmount;
	}
	fs_close(&file);

	/* Verify */
	if (strcmp(test_str, read_buf) == 0) {
		printf("Content matches: %s", read_buf);
		printf("PASS\n");
	} else {
		printf("Content mismatch!\n");
		printf("Expected: %s\n", test_str);
		printf("Got: %s\n", read_buf);
		printf("FAIL\n");
	}

unmount:
	fs_unmount(&mp);
	return 0;
}
