/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Test cases for filesystem api
 *
 * @defgroup filesystem_api File system API
 * @ingroup filesystem
 * @{
 * @}
 */

#include "test_fs.h"
#include <stdio.h>
#include <string.h>

struct fs_file_system_t null_fs = {NULL};
static struct test_fs_data test_data;

static struct fs_mount_t test_fs_mnt_1 = {
		.type = TEST_FS_1,
		.mnt_point = TEST_FS_MNTP,
		.fs_data = &test_data,
};

static struct fs_mount_t test_fs_mnt_unsupported_fs = {
		.type = FS_TYPE_END,
		.mnt_point = "/MMCBLOCK:",
		.fs_data = &test_data,
};

static struct fs_mount_t test_fs_mnt_invalid_root = {
		.type = TEST_FS_2,
		.mnt_point = "SDA:",
		.fs_data = &test_data,
};

static struct fs_mount_t test_fs_mnt_already_mounted = {
		.type = TEST_FS_2,
		.mnt_point = TEST_FS_MNTP,
		.fs_data = &test_data,
};

static struct fs_mount_t test_fs_mnt_invalid_parm = {
		.type = TEST_FS_2,
		.mnt_point = "/SDA",
		.fs_data = &test_data,
};

static struct fs_mount_t test_fs_mnt_no_op = {
		.type = TEST_FS_2,
		.mnt_point = "/SDA:",
		.fs_data = &test_data,
};

static struct fs_file_t filep;
static const char test_str[] = "hello world!";

/**
 * @brief Test mount interface of filesystem
 *
 * @ingroup filesystem_api
 */
void test_mount(void)
{
	int ret;

	TC_PRINT("\nmount tests:\n");
	TC_PRINT("Mount to a NULL directory\n");
	ret = fs_mount(NULL);
	zassert_not_equal(ret, 0, "Mount a NULL fs");

	TC_PRINT("Mount to a unsupported directory\n");
	ret = fs_mount(&test_fs_mnt_unsupported_fs);
	zassert_not_equal(ret, 0, "Mount a unsupported fs");

	fs_register(TEST_FS_2, &temp_fs);
	TC_PRINT("Mount to an invalid directory\n");
	ret = fs_mount(&test_fs_mnt_invalid_root);
	zassert_not_equal(ret, 0, "Mount to an invalid dir");

	TC_PRINT("Invalid parameter pass to file system operation interface\n");
	ret = fs_mount(&test_fs_mnt_invalid_parm);
	zassert_not_equal(ret, 0, "Mount with invalid parm");

	ret = fs_mount(&test_fs_mnt_1);
	zassert_equal(ret, 0, "Error mounting fs");

	TC_PRINT("Mount to a directory that has file system mounted already\n");
	ret = fs_mount(&test_fs_mnt_already_mounted);
	zassert_not_equal(ret, 0, "Mount to a mounted dir");

	fs_unregister(TEST_FS_2, &temp_fs);

	fs_register(TEST_FS_2, &null_fs);
	TC_PRINT("Mount a file system has no interface implemented\n");
	ret = fs_mount(&test_fs_mnt_no_op);
	zassert_not_equal(ret, 0, "Mount to a fs without op interface");
	fs_unregister(TEST_FS_2, &null_fs);
}

/**
 * @brief Test unmount interface of filesystem
 *
 * @ingroup filesystem_api
 */
void test_unmount(void)
{
	int ret;

	TC_PRINT("\nunmount tests:\n");

	TC_PRINT("\nunmount nothing:\n");
	ret = fs_unmount(NULL);
	zassert_not_equal(ret, 0, "Unmount a NULL fs");

	TC_PRINT("\nunmount file system that has never been mounted:\n");
	ret = fs_unmount(&test_fs_mnt_unsupported_fs);
	zassert_not_equal(ret, 0, "Unmount a never mounted fs");

	TC_PRINT("\nunmount file system multiple times:\n");
	ret = fs_unmount(&test_fs_mnt_1);
	zassert_true(ret >= 0, "Fail to unmount fs");

	test_fs_mnt_1.fs = &temp_fs;
	ret = fs_unmount(&test_fs_mnt_1);
	zassert_not_equal(ret, 0, "Unmount a unmounted fs");
}

/**
 * @brief Test statvfs interface of filesystem
 *
 * @ingroup filesystem_api
 */
void test_file_statvfs(void)
{
	struct fs_statvfs stat;
	int ret;

	ret = fs_statvfs(NULL, &stat);
	zassert_not_equal(ret, 0, "Get valume without path");

	ret = fs_statvfs("/SDCARD:", &stat);
	zassert_not_equal(ret, 0, "Get valume by no-exist path");

	ret = fs_statvfs(TEST_FS_MNTP, NULL);
	zassert_not_equal(ret, 0, "Get valume without stat structure");

	ret = fs_statvfs(TEST_FS_MNTP, &stat);
	zassert_equal(ret, 0, "Error getting voluem stats");

	TC_PRINT("\n");
	TC_PRINT("Optimal transfer block size   = %lu\n", stat.f_bsize);
	TC_PRINT("Allocation unit size          = %lu\n", stat.f_frsize);
	TC_PRINT("Volume size in f_frsize units = %lu\n", stat.f_blocks);
	TC_PRINT("Free space in f_frsize units  = %lu\n", stat.f_bfree);
}

/**
 * @brief Test make directory interface of filesystem
 *
 * @ingroup filesystem_api
 */
void test_mkdir(void)
{
	int ret;

	TC_PRINT("\nmkdir tests:\n");

	ret = fs_mkdir(NULL);
	zassert_not_equal(ret, 0, "Create a NULL directory");

	ret = fs_mkdir("/SDCARD:/testdir");
	zassert_not_equal(ret, 0, "Create dir in no fs mounted dir");

	ret = fs_mkdir(TEST_FS_MNTP);
	zassert_not_equal(ret, 0, "Shoult not create root dir");

	ret = fs_mkdir(TEST_DIR);
	zassert_equal(ret, 0, "Error creating dir");

	TC_PRINT("Created dir %s!\n", TEST_DIR);
}

/**
 * @brief Test open directory interface of filesystem
 *
 * @ingroup filesystem_api
 */
void test_opendir(void)
{
	int ret;
	struct fs_dir_t dirp;

	TC_PRINT("\nopendir tests:\n");

	memset(&dirp, 0, sizeof(dirp));
	TC_PRINT("Test null path\n");
	ret = fs_opendir(NULL, NULL);
	zassert_not_equal(ret, 0, "Open NULL dir");

	TC_PRINT("Test root directory\n");
	ret = fs_opendir(&dirp, "/");
	zassert_equal(ret, 0, "Fail to open root dir");

	TC_PRINT("Test non-exist mount point\n");
	ret = fs_opendir(&dirp, "/SDCARD:/test_dir");
	zassert_not_equal(ret, 0, "Open dir in a unmounted fs");

	ret = fs_opendir(&dirp, TEST_DIR);
	zassert_equal(ret, 0, "Fail to open dir");

	TC_PRINT("Open same directory multi times\n");
	ret = fs_opendir(&dirp, TEST_DIR);
	zassert_not_equal(ret, 0, "Can't reopen an opened dir");

	TC_PRINT("Opening dir successfully\n");
}

/**
 * @brief Test close directory interface of filesystem
 *
 * @ingroup filesystem_api
 */
void test_closedir(void)
{
	int ret;
	struct fs_dir_t dirp;

	TC_PRINT("\nclosedir tests: %s\n", TEST_DIR);
	memset(&dirp, 0, sizeof(dirp));
	ret = fs_opendir(&dirp, TEST_DIR);
	zassert_equal(ret, 0, "Fail to open dir");

	ret = fs_closedir(&dirp);
	zassert_equal(ret, 0, "Fail to close dir");

	dirp.mp = &test_fs_mnt_1;
	ret = fs_closedir(&dirp);
	zassert_not_equal(ret, 0, "Should no close a closed dir");
}

static int _test_lsdir(const char *path)
{
	int ret;
	struct fs_dir_t dirp;
	struct fs_dirent entry;

	TC_PRINT("\nlsdir tests:\n");

	memset(&dirp, 0, sizeof(dirp));
	memset(&entry, 0, sizeof(entry));

	TC_PRINT("read an unopened dir\n");
	dirp.dirp = "somepath";
	ret = fs_readdir(&dirp, &entry);
	if (!ret) {
		return TC_FAIL;
	}

	dirp.mp = &test_fs_mnt_1;
	ret = fs_readdir(&dirp, NULL);
	if (!ret) {
		return TC_FAIL;
	}

	TC_PRINT("read an opened dir\n");
	ret = fs_opendir(&dirp, path);
	if (ret) {
		if (path) {
			TC_PRINT("Error opening dir %s [%d]\n", path, ret);
		}
		return TC_FAIL;
	}

	TC_PRINT("\nListing dir %s:\n", path);
	for (;;) {
		ret = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (ret || entry.name[0] == 0) {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			TC_PRINT("[DIR ] %s\n", entry.name);
		} else {
			TC_PRINT("[FILE] %s (size = %zu)\n",
				entry.name, entry.size);
		}
	}

	ret = fs_closedir(&dirp);
	if (ret) {
		TC_PRINT("Error close a directory\n");
		return TC_FAIL;
	}

	return ret;
}

/**
 * @brief Test lsdir interface include opendir, readdir, closedir
 *
 * @ingroup filesystem_api
 */
void test_lsdir(void)
{
	zassert_true(_test_lsdir(NULL) == TC_FAIL, NULL);
	zassert_true(_test_lsdir("/") == TC_PASS, NULL);
	zassert_true(_test_lsdir("/test") == TC_FAIL, NULL);
	zassert_true(_test_lsdir(TEST_DIR) == TC_PASS, NULL);
}

/**
 * @brief Open a existing file or create a new file
 *
 * @ingroup filesystem_api
 */

void test_file_open(void)
{
	int ret;

	TC_PRINT("\nOpen tests:\n");

	TC_PRINT("\nOpen a file without a path\n");
	ret = fs_open(&filep, NULL, FS_O_READ);
	zassert_not_equal(ret, 0, "Open a NULL file");

	TC_PRINT("\nOpen a file with wrong abs path\n");
	ret = fs_open(&filep, "/test_file.txt", FS_O_READ);
	zassert_not_equal(ret, 0, "Open a file with wrong path");

	ret = fs_open(&filep, TEST_FILE, FS_O_READ);
	zassert_equal(ret, 0, "Fail to open file");

	TC_PRINT("\nReopen the same file");
	ret = fs_open(&filep, TEST_FILE, FS_O_READ);
	zassert_not_equal(ret, 0, "Reopen an opend file");

	TC_PRINT("Opened file %s\n", TEST_FILE);
}

static int _test_file_write(void)
{
	ssize_t brw;
	int ret;

	TC_PRINT("\nWrite tests:\n");

	ret = fs_seek(&filep, 0, FS_SEEK_SET);
	if (ret) {
		TC_PRINT("fs_seek failed [%d]\n", ret);
		fs_close(&filep);
		return ret;
	}

	TC_PRINT("Write to file from a invalid source\n");
	brw = fs_write(&filep, NULL, strlen(test_str));
	if (brw >= 0) {
		return TC_FAIL;
	}

	TC_PRINT("Data written:\"%s\"\n\n", test_str);

	brw = fs_write(&filep, (char *)test_str, strlen(test_str));
	if (brw < 0) {
		TC_PRINT("Failed writing to file [%zd]\n", brw);
		fs_close(&filep);
		return brw;
	}

	if (brw < strlen(test_str)) {
		TC_PRINT("Unable to complete write. Volume full.\n");
		TC_PRINT("Number of bytes written: [%zd]\n", brw);
		fs_close(&filep);
		return TC_FAIL;
	}

	TC_PRINT("Data successfully written!\n");

	return ret;
}

/**
 * @brief Write items of data of size bytes long
 *
 * @ingroup filesystem_api
 *
 */
void test_file_write(void)
{
	zassert_true(_test_file_write() == TC_PASS, NULL);
}

static int _test_file_sync(void)
{
	int ret;
	ssize_t brw;

	TC_PRINT("\nSync tests:\n");

	ret = fs_open(&filep, TEST_FILE, FS_O_RDWR);

	for (;;) {
		brw = fs_write(&filep, (char *)test_str, strlen(test_str));
		if (brw < strlen(test_str)) {
			break;
		}
		ret = fs_sync(&filep);
		if (ret) {
			TC_PRINT("Error syncing file [%d]\n", ret);
			fs_close(&filep);
			return ret;
		}

		ret = fs_tell(&filep);
		if (ret < 0) {
			TC_PRINT("Error tell file [%d]\n", ret);
			fs_close(&filep);
			return ret;
		}

	}

	TC_PRINT("Sync a overflowed file\n");
	ret = fs_sync(&filep);
	if (!ret) {
		fs_close(&filep);
		return TC_FAIL;
	}

	TC_PRINT("Tell a overflowed file\n");
	ret = fs_tell(&filep);
	if (!ret) {
		fs_close(&filep);
		return TC_FAIL;
	}

	fs_close(&filep);
	return TC_PASS;
}

/**
 * @brief Flush the cache of an open file
 *
 * @ingroup filesystem_api
 */
void test_file_sync(void)
{
	zassert_true(_test_file_sync() == TC_PASS, NULL);
}

/**
 * @brief Read items of data of size bytes long
 *
 * @ingroup filesystem_api
 */
void test_file_read(void)
{
	ssize_t brw;
	char read_buff[80];
	size_t sz = strlen(test_str);

	TC_PRINT("\nRead tests:\n");

	TC_PRINT("Read to a invalid buffer\n");
	brw = fs_read(&filep, NULL, sz);
	zassert_false(brw >= 0, "Read data to a invalid buffer");

	brw = fs_read(&filep, read_buff, sz);
	zassert_true(brw >= 0, "Fail to read file");

	read_buff[brw] = 0;
	TC_PRINT("Data read:\"%s\"\n\n", read_buff);

	zassert_true(strcmp(test_str, read_buff) == 0,
		    "Error - Data read does not match data written");

	TC_PRINT("Data read matches data written\n");
}

static int _test_file_truncate(void)
{
	int ret;
	off_t orig_pos;
	char read_buff[80];
	ssize_t brw;

	TC_PRINT("\nTruncate tests: max file size is 128byte\n");

	TC_PRINT("Truncating to size larger than 128byte\n");
	ret = fs_truncate(&filep, 256);
	if (!ret) {
		fs_close(&filep);
		return TC_FAIL;
	}

	/* Test truncating to 0 size */
	TC_PRINT("\nTesting shrink to 0 size\n");
	ret = fs_truncate(&filep, 0);
	if (ret) {
		TC_PRINT("fs_truncate failed [%d]\n", ret);
		fs_close(&filep);
		return ret;
	}

	TC_PRINT("File seek from invalid whence\n");
	ret = fs_seek(&filep, 0, 100);
	if (!ret) {
		fs_close(&filep);
		return TC_FAIL;
	}

	fs_seek(&filep, 0, FS_SEEK_END);
	if (fs_tell(&filep) > 0) {
		TC_PRINT("Failed truncating to size 0\n");
		fs_close(&filep);
		return TC_FAIL;
	}

	TC_PRINT("Testing write after truncating\n");
	ret = _test_file_write();
	if (ret) {
		TC_PRINT("Write failed after truncating\n");
		return ret;
	}

	fs_seek(&filep, 0, FS_SEEK_END);

	orig_pos = fs_tell(&filep);
	TC_PRINT("Original size of file = %d\n", (int)orig_pos);

	/* Test shrinking file */
	TC_PRINT("\nTesting shrinking\n");
	ret = fs_truncate(&filep, orig_pos - 5);
	if (ret) {
		TC_PRINT("fs_truncate failed [%d]\n", ret);
		fs_close(&filep);
		return ret;
	}

	fs_seek(&filep, 0, FS_SEEK_END);
	TC_PRINT("File size after shrinking by 5 bytes = %d\n",
						(int)fs_tell(&filep));
	if (fs_tell(&filep) != (orig_pos - 5)) {
		TC_PRINT("File size after fs_truncate not as expected\n");
		fs_close(&filep);
		return TC_FAIL;
	}

	/* Test expanding file */
	TC_PRINT("\nTesting expanding\n");
	fs_seek(&filep, 0, FS_SEEK_END);
	orig_pos = fs_tell(&filep);
	ret = fs_truncate(&filep, orig_pos + 10);
	if (ret) {
		TC_PRINT("fs_truncate failed [%d]\n", ret);
		fs_close(&filep);
		return ret;
	}

	fs_seek(&filep, 0, FS_SEEK_END);
	TC_PRINT("File size after expanding by 10 bytes = %d\n",
						(int)fs_tell(&filep));
	if (fs_tell(&filep) != (orig_pos + 10)) {
		TC_PRINT("File size after fs_truncate not as expected\n");
		fs_close(&filep);
		return TC_FAIL;
	}

	/* Check if expanded regions are zeroed */
	TC_PRINT("Testing for zeroes in expanded region\n");
	fs_seek(&filep, -5, FS_SEEK_END);

	brw = fs_read(&filep, read_buff, 5);

	if (brw < 5) {
		TC_PRINT("Read failed after truncating\n");
		fs_close(&filep);
		return -1;
	}

	for (int i = 0; i < 5; i++) {
		if (read_buff[i]) {
			TC_PRINT("Expanded regions are not zeroed\n");
			fs_close(&filep);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

/**
 * @brief Truncate the file to the new length
 *
 * @details This test include three cases:
 *            - fs_seek, locate the position to truncate
 *            - fs_truncate
 *            - fs_tell, retrieve the current position
 *
 * @ingroup filesystem_api
 */
void test_file_truncate(void)
{
	zassert_true(_test_file_truncate() == TC_PASS, NULL);
}

/**
 * @brief Flush associated stream and close the file
 *
 * @ingroup filesystem_api
 *
 */

void test_file_close(void)
{
	int ret;

	TC_PRINT("\nClose tests:\n");

	ret = fs_close(&filep);
	zassert_equal(ret, 0, "Fail to close file");

	TC_PRINT("\nClose a closed file:\n");
	filep.mp = &test_fs_mnt_1;
	ret = fs_close(&filep);
	zassert_not_equal(ret, 0, "Should not reclose a closed file");

	TC_PRINT("Closed file %s\n", TEST_FILE);
}

/**
 * @brief Rename a file or directory
 *
 * @ingroup filesystem_api
 */
void test_file_rename(void)
{
	int ret = TC_FAIL;

	TC_PRINT("\nRename file tests:\n");

	ret = fs_rename(NULL, NULL);
	zassert_not_equal(ret, 0, "Rename a NULL file");

	ret = fs_rename("/SDCARD:/testfile.txt", TEST_FILE_RN);
	zassert_not_equal(ret, 0, "Rename a non-exist file");

	ret = fs_rename(TEST_FILE, "/SDCARD:/testfile_renamed.txt");
	zassert_not_equal(ret, 0, "Rename file to different mount point");

	ret = fs_rename(TEST_FILE, TEST_FILE_EX);
	zassert_not_equal(ret, 0, "Rename file to an exist file");

	ret = fs_rename(TEST_FILE, TEST_FILE_RN);
	zassert_equal(ret, 0, "Fail to rename a file");
}

/**
 * @brief Check the status of a file or directory specified by the path
 *
 * @ingroup filesystem_api
 */
void test_file_stat(void)
{
	int ret;
	struct fs_dirent entry;

	TC_PRINT("\nStat file tests:\n");

	ret = fs_stat(NULL, &entry);
	zassert_not_equal(ret, 0, "Stat a NULL dir");

	ret = fs_stat("/SDCARD", &entry);
	zassert_not_equal(ret, 0, "Stat a non-exist dir");

	ret = fs_stat(TEST_DIR, NULL);
	zassert_not_equal(ret, 0, "Stat a dir without entry");

	ret = fs_stat(TEST_DIR, &entry);
	zassert_equal(ret, 0, "Fail to stat a dir");

	ret = fs_stat(TEST_DIR_FILE, &entry);
	zassert_equal(ret, 0, "Fail to stat a file");
}

/**
 * @brief Delete the specified file or directory
 *
 * @ingroup filesystem_api
 *
 */

void test_file_unlink(void)
{
	int ret;

	TC_PRINT("\nDelete tests:\n");

	ret = fs_unlink(NULL);
	zassert_not_equal(ret, 0, "Delete a NULL file");

	ret = fs_unlink("/SDCARD:/test_file.txt");
	zassert_not_equal(ret, 0, "Delete a non-exist file");

	ret = fs_unlink(TEST_FS_MNTP);
	zassert_not_equal(ret, 0, "Delete a root dir");

	ret = fs_unlink(TEST_FILE_RN);
	zassert_equal(ret, 0, "Fail to delete file");

	TC_PRINT("File (%s) deleted successfully!\n", TEST_FILE_RN);
}
