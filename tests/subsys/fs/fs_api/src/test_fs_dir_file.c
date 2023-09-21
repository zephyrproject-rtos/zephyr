/*
 * Copyright (c) 2020 Intel Corporation.
 * Copyright (c) 2020 Nordic Semiconductor ASA
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

static struct fs_mount_t test_fs_mnt_already_mounted_same_data = {
		.type = TEST_FS_2,
		.mnt_point = "/OTHER",
		.fs_data = &test_data,
};

static struct test_fs_data test_data1;
static struct fs_mount_t test_fs_mnt_unsupported_fs = {
		.type = FS_TYPE_EXTERNAL_BASE,
		.mnt_point = "/MMCBLOCK:",
		.fs_data = &test_data1,
};

/* invalid name of mount point, not start with '/' */
static struct test_fs_data test_data2;
static struct fs_mount_t test_fs_mnt_invalid_root_1 = {
		.type = TEST_FS_2,
		.mnt_point = "SDA:",
		.fs_data = &test_data2,
};

/* length of the name of mount point is too short */
static struct test_fs_data test_data3;
static struct fs_mount_t test_fs_mnt_invalid_root_2 = {
		.type = TEST_FS_2,
		.mnt_point = "/",
		.fs_data = &test_data3,
};

/* NULL mount point */
static struct test_fs_data test_data4;
static struct fs_mount_t test_fs_mnt_invalid_root_3 = {
		.type = TEST_FS_2,
		.mnt_point = NULL,
		.fs_data = &test_data4,
};

static struct test_fs_data test_data5;
static struct fs_mount_t test_fs_mnt_already_mounted = {
		.type = TEST_FS_2,
		.mnt_point = TEST_FS_MNTP,
		.fs_data = &test_data5,
};

/* for test_fs, name of mount point must end with ':' */
static struct test_fs_data test_data6;
static struct fs_mount_t test_fs_mnt_invalid_mntp = {
		.type = TEST_FS_2,
		.mnt_point = "/SDA",
		.fs_data = &test_data6,
};

#define NOOP_MNTP "/SDCD:"
static struct test_fs_data test_data7;
static struct fs_mount_t test_fs_mnt_no_op = {
		.type = TEST_FS_2,
		.mnt_point = NOOP_MNTP,
		.fs_data = &test_data7,
};

static struct fs_file_t filep;
static struct fs_file_t err_filep;
static const char test_str[] = "hello world!";

/**
 * @brief Test fs_file_t_init initializer
 */
ZTEST(fs_api_dir_file, test_fs_file_t_init)
{
	struct fs_file_t fst;

	memset(&fst, 0xff, sizeof(fst));

	fs_file_t_init(&fst);
	zassert_equal(fst.mp, NULL, "Expected to be initialized to NULL");
	zassert_equal(fst.filep, NULL, "Expected to be initialized to NULL");
	zassert_equal(fst.flags, 0, "Expected to be initialized to 0");
}

/**
 * @brief Test fs_dir_t_init initializer
 */
ZTEST(fs_api_dir_file, test_fs_dir_t_init)
{
	struct fs_dir_t dirp;

	memset(&dirp, 0xff, sizeof(dirp));

	fs_dir_t_init(&dirp);
	zassert_equal(dirp.mp, NULL, "Expected to be initialized to NULL");
	zassert_equal(dirp.dirp, NULL, "Expected to be initialized to NULL");
}

/**
 * @brief Test mount interface of filesystem
 *
 * @details
 * Test fs_mount() interface in file system core
 * Following test cases depend on file systems mounted in this test case
 *
 * @ingroup filesystem_api
 */
void test_mount(void)
{
	int ret;

	TC_PRINT("\nmount tests:\n");
	TC_PRINT("Pass NULL pointer to fs_mount()\n");
	ret = fs_mount(NULL);
	zassert_not_equal(ret, 0, "Mount a NULL fs");

	TC_PRINT("Mount an unsupported file system\n");
	ret = fs_mount(&test_fs_mnt_unsupported_fs);
	zassert_not_equal(ret, 0, "Mount an unsupported fs");

	fs_register(TEST_FS_2, &temp_fs);

	TC_PRINT("Mount to an invalid directory\n");
	ret = fs_mount(&test_fs_mnt_invalid_root_1);
	zassert_not_equal(ret, 0, "Mount to an invalid dir");
	ret = fs_mount(&test_fs_mnt_invalid_root_2);
	zassert_not_equal(ret, 0, "Mount dir name too short");
	ret = fs_mount(&test_fs_mnt_invalid_root_3);
	zassert_not_equal(ret, 0, "Mount point is NULL");
	ret = fs_mount(&test_fs_mnt_invalid_mntp);
	zassert_not_equal(ret, 0, "Mount with invalid mount point");

	ret = fs_mount(&test_fs_mnt_1);
	zassert_equal(ret, 0, "Error mounting fs");

	TC_PRINT("Mount to a directory that has file system mounted already\n");
	ret = fs_mount(&test_fs_mnt_already_mounted);
	zassert_not_equal(ret, 0, "Mount to a mounted dir");

	TC_PRINT("Mount using same private data as already mounted system\n");
	ret = fs_mount(&test_fs_mnt_already_mounted_same_data);
	zassert_equal(ret, -EBUSY, "Re-mount using same data should have failed");

	fs_unregister(TEST_FS_2, &temp_fs);
	memset(&null_fs, 0, sizeof(null_fs));
	fs_register(TEST_FS_2, &null_fs);

	TC_PRINT("Mount a file system has no interface implemented\n");
	ret = fs_mount(&test_fs_mnt_no_op);
	zassert_not_equal(ret, 0, "Mount to a fs without op interface");

	/* mount an file system has no unmount functionality */
	null_fs.mount = temp_fs.mount;
	ret = fs_mount(&test_fs_mnt_no_op);
	zassert_equal(ret, 0, "fs has no unmount functionality can be mounted");
}

/**
 * @brief Test fs_unmount() interface in file system core
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

	ret = fs_unmount(&test_fs_mnt_1);
	zassert_true(ret >= 0, "Fail to unmount fs");

	TC_PRINT("\nunmount file system multiple times:\n");
	test_fs_mnt_1.fs = &temp_fs;
	ret = fs_unmount(&test_fs_mnt_1);
	zassert_not_equal(ret, 0, "Unmount an unmounted fs");

	TC_PRINT("unmount a file system has no unmount functionality\n");
	ret = fs_unmount(&test_fs_mnt_no_op);
	zassert_not_equal(ret, 0, "Unmount a fs has no unmount functionality");
	/* assign a unmount interface to null_fs to unmount it */
	null_fs.unmount = temp_fs.unmount;
	ret = fs_unmount(&test_fs_mnt_no_op);
	zassert_equal(ret, 0, "file system should be unmounted");
	/* TEST_FS_2 is registered in test_mount(), unregister it here */
	fs_unregister(TEST_FS_2, &null_fs);
}

/**
 * @brief Test fs_statvfs() interface in file system core
 *
 * @ingroup filesystem_api
 */
ZTEST(fs_api_dir_file, test_file_statvfs)
{
	struct fs_statvfs stat;
	int ret;

	ret = fs_statvfs(NULL, &stat);
	zassert_not_equal(ret, 0, "Pass NULL for path pointer");
	ret = fs_statvfs(TEST_FS_MNTP, NULL);
	zassert_not_equal(ret, 0, "Pass NULL for stat structure pointer");

	ret = fs_statvfs("/", &stat);
	zassert_not_equal(ret, 0, "Path name too short");

	ret = fs_statvfs("SDCARD:", &stat);
	zassert_not_equal(ret, 0, "Path name should start with /");

	ret = fs_statvfs("/SDCARD:", &stat);
	zassert_not_equal(ret, 0, "Get volume info by no-exist path");

	/* System with no statvfs interface */
	ret = fs_statvfs(NOOP_MNTP, &stat);
	zassert_equal(ret, -ENOTSUP, "fs has no statvfs functionality");

	ret = fs_statvfs(TEST_FS_MNTP, &stat);
	zassert_equal(ret, 0, "Error getting volume stats");
	TC_PRINT("\n");
	TC_PRINT("Optimal transfer block size   = %lu\n", stat.f_bsize);
	TC_PRINT("Allocation unit size          = %lu\n", stat.f_frsize);
	TC_PRINT("Volume size in f_frsize units = %lu\n", stat.f_blocks);
	TC_PRINT("Free space in f_frsize units  = %lu\n", stat.f_bfree);
}

/**
 * @brief Test fs_mkdir() interface in file system core
 *
 * @ingroup filesystem_api
 */
void test_mkdir(void)
{
	int ret;

	TC_PRINT("\nmkdir tests:\n");

	ret = fs_mkdir(NULL);
	zassert_not_equal(ret, 0, "Create a NULL directory");

	ret = fs_mkdir("d");
	zassert_not_equal(ret, 0, "Create dir with too short name");

	ret = fs_mkdir("SDCARD:/testdir");
	zassert_not_equal(ret, 0, "Create dir with wrong path");

	ret = fs_mkdir("/SDCARD:/testdir");
	zassert_not_equal(ret, 0, "Create dir in no fs mounted dir");

	ret = fs_mkdir(TEST_FS_MNTP);
	zassert_not_equal(ret, 0, "Should not create root dir");

	ret = fs_mkdir(NOOP_MNTP"/testdir");
	zassert_not_equal(ret, 0, "Filesystem has no mkdir interface");

	ret = fs_mkdir(TEST_DIR);
	zassert_equal(ret, 0, "Error creating dir");
}

/**
 * @brief Test fs_opendir() interface in file system core
 *
 * @ingroup filesystem_api
 */
void test_opendir(void)
{
	int ret;
	struct fs_dir_t dirp, dirp2, dirp3;

	TC_PRINT("\nopendir tests:\n");

	fs_dir_t_init(&dirp);
	fs_dir_t_init(&dirp2);
	fs_dir_t_init(&dirp3);

	TC_PRINT("Test null path\n");
	ret = fs_opendir(NULL, NULL);
	zassert_not_equal(ret, 0, "Open dir with NULL pointer parameter");

	TC_PRINT("Test directory without root path\n");
	ret = fs_opendir(&dirp, "ab");
	zassert_not_equal(ret, 0, "Can't open dir without root path");

	TC_PRINT("Test directory without name\n");
	ret = fs_opendir(&dirp, "");
	zassert_not_equal(ret, 0, "Can't open dir without path name");

	TC_PRINT("Test not existing mount point\n");
	ret = fs_opendir(&dirp, "/SDCARD:/test_dir");
	zassert_not_equal(ret, 0, "Open dir in an unmounted fs");

	TC_PRINT("Test filesystem has no opendir functionality\n");
	ret = fs_opendir(&dirp, NOOP_MNTP"/test_dir");
	zassert_not_equal(ret, 0, "Filesystem has no opendir functionality");

	TC_PRINT("Test root directory\n");
	ret = fs_opendir(&dirp, "/");
	zassert_equal(ret, 0, "Fail to open root dir");

	TC_PRINT("Double-open using occupied fs_dir_t object\n");
	ret = fs_opendir(&dirp, "/not_a_dir");
	zassert_equal(ret, -EBUSY, "Expected -EBUSY, got %d", ret);

	ret = fs_opendir(&dirp2, TEST_DIR);
	zassert_equal(ret, 0, "Fail to open dir");

	TC_PRINT("Double-open using occupied fs_dir_t object\n");
	ret = fs_opendir(&dirp2, "/xD");
	zassert_equal(ret, -EBUSY, "Expected -EBUSY, got %d", ret);

	mock_opendir_result(-EIO);
	TC_PRINT("Transfer underlying FS error\n");
	ret = fs_opendir(&dirp3, TEST_DIR);
	mock_opendir_result(0);
	zassert_equal(ret, -EIO, "FS error not transferred\n");
}

/**
 * @brief Test fs_close() interface in file system core
 *
 * @ingroup filesystem_api
 */
void test_closedir(void)
{
	int ret;
	struct fs_dir_t dirp;

	TC_PRINT("\nclosedir tests: %s\n", TEST_DIR);
	fs_dir_t_init(&dirp);
	ret = fs_opendir(&dirp, TEST_DIR);
	zassert_equal(ret, 0, "Fail to open dir");

	ret = fs_closedir(&dirp);
	zassert_equal(ret, 0, "Fail to close dir");

	dirp.mp = &test_fs_mnt_1;
	ret = fs_closedir(&dirp);
	zassert_not_equal(ret, 0, "Should no close a closed dir");

	dirp.mp = &test_fs_mnt_no_op;
	ret = fs_closedir(&dirp);
	zassert_not_equal(ret, 0, "Filesystem has no closedir interface");
}

/**
 * @brief Test Reuse fs_dir_t object from closed directory"
 *
 * @ingroup filesystem_api
 */
void test_opendir_closedir(void)
{
	int ret;
	struct fs_dir_t dirp;

	TC_PRINT("\nreuse fs_dir_t tests:\n");

	fs_dir_t_init(&dirp);

	TC_PRINT("Test: open root dir, close, open volume dir\n");
	ret = fs_opendir(&dirp, "/");
	zassert_equal(ret, 0, "Fail to open root dir");

	ret = fs_closedir(&dirp);
	zassert_equal(ret, 0, "Fail to close dir");

	ret = fs_opendir(&dirp, TEST_DIR);
	zassert_equal(ret, 0, "Fail to open dir");

	TC_PRINT("Test: open volume dir, close, open root dir\n");
	ret = fs_closedir(&dirp);
	zassert_equal(ret, 0, "Fail to close dir");

	ret = fs_opendir(&dirp, "/");
	zassert_equal(ret, 0, "Fail to open root dir");
}

static int _test_lsdir(const char *path)
{
	int ret;
	struct fs_dir_t dirp;
	struct fs_dirent entry;

	TC_PRINT("\nlsdir tests:\n");

	fs_dir_t_init(&dirp);
	memset(&entry, 0, sizeof(entry));

	TC_PRINT("read an unopened dir\n");
	dirp.dirp = "somepath";
	ret = fs_readdir(&dirp, &entry);
	if (!ret) {
		return TC_FAIL;
	}

	dirp.mp = &test_fs_mnt_no_op;
	ret = fs_readdir(&dirp, NULL);
	if (!ret) {
		return TC_FAIL;
	}

	dirp.mp = &test_fs_mnt_1;
	ret = fs_readdir(&dirp, NULL);
	if (!ret) {
		return TC_FAIL;
	}

	TC_PRINT("read an opened dir\n");
	fs_dir_t_init(&dirp);
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
 * @brief Test fs_readdir() interface in file system core
 *
 * @ingroup filesystem_api
 */
void test_lsdir(void)
{
	zassert_true(_test_lsdir(NULL) == TC_FAIL);
	zassert_true(_test_lsdir("/") == TC_PASS);
	zassert_true(_test_lsdir("/test") == TC_FAIL);
	zassert_true(_test_lsdir(TEST_DIR) == TC_PASS);
}

/**
 * @brief Test fs_open() interface in file system core
 *
 * @ingroup filesystem_api
 */
void test_file_open(void)
{
	int ret;

	TC_PRINT("\nOpen tests:\n");
	fs_file_t_init(&filep);

	TC_PRINT("\nOpen a file without a path\n");
	ret = fs_open(&filep, NULL, FS_O_READ);
	zassert_not_equal(ret, 0, "Open a NULL file");

	TC_PRINT("\nOpen a file with wrong abs path\n");
	ret = fs_open(&filep, "/", FS_O_READ);
	zassert_not_equal(ret, 0, "Open a file with wrong path");

	TC_PRINT("\nOpen a file with wrong path\n");
	ret = fs_open(&filep, "test_file.txt", FS_O_READ);
	zassert_not_equal(ret, 0, "Open a file with wrong path");

	TC_PRINT("\nOpen a file with wrong abs path\n");
	ret = fs_open(&filep, "/test_file.txt", FS_O_READ);
	zassert_not_equal(ret, 0, "Open a file with wrong abs path");

	TC_PRINT("\nFilesystem has no open functionality\n");
	ret = fs_open(&filep, NOOP_MNTP"/test_file.txt", FS_O_READ);
	zassert_not_equal(ret, 0, "Filesystem has no open functionality");

	ret = fs_open(&filep, TEST_FILE, FS_O_READ);
	zassert_equal(ret, 0, "Fail to open file");

	TC_PRINT("\nDouble-open\n");
	ret = fs_open(&filep, TEST_FILE, FS_O_READ);
	zassert_equal(ret, -EBUSY, "Expected -EBUSY, got %d", ret);

	TC_PRINT("\nReopen the same file");
	ret = fs_open(&filep, TEST_FILE, FS_O_READ);
	zassert_not_equal(ret, 0, "Reopen an opened file");

	TC_PRINT("Opened file %s\n", TEST_FILE);
}

static int _test_file_write(void)
{
	ssize_t brw;
	int ret;

	TC_PRINT("\nWrite tests:\n");

	TC_PRINT("Write to an unopened file\n");
	fs_file_t_init(&err_filep);
	brw = fs_write(&err_filep, (char *)test_str, strlen(test_str));
	if (brw >= 0) {
		return TC_FAIL;
	}

	TC_PRINT("Write to filesystem has no write interface\n");
	err_filep.mp = &test_fs_mnt_no_op;
	brw = fs_write(&err_filep, (char *)test_str, strlen(test_str));
	if (brw >= 0) {
		return TC_FAIL;
	}

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
 * @brief Test fs_write() interface in file system core
 *
 * @ingroup filesystem_api
 */
void test_file_write(void)
{
	zassert_true(_test_file_write() == TC_PASS);
}

static int _test_file_sync(void)
{
	int ret;
	ssize_t brw;

	TC_PRINT("\nSync tests:\n");

	TC_PRINT("sync an unopened file\n");
	fs_file_t_init(&err_filep);
	ret = fs_sync(&err_filep);
	if (!ret) {
		return TC_FAIL;
	}

	TC_PRINT("sync to filesystem has no sync functionality\n");
	err_filep.mp = &test_fs_mnt_no_op;
	ret = fs_sync(&err_filep);
	if (!ret) {
		return TC_FAIL;
	}

	fs_file_t_init(&filep);
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
 * @brief Test fs_sync() interface in file system core
 *
 * @ingroup filesystem_api
 */
ZTEST(fs_api_dir_file, test_file_sync)
{
	zassert_true(_test_file_sync() == TC_PASS);
}

/**
 * @brief Test fs_read() interface in file system core
 *
 * @ingroup filesystem_api
 */
void test_file_read(void)
{
	ssize_t brw;
	char read_buff[80];
	size_t sz = strlen(test_str);

	TC_PRINT("\nRead tests:\n");

	TC_PRINT("Read an unopened file\n");
	fs_file_t_init(&err_filep);
	brw = fs_read(&err_filep, read_buff, sz);
	zassert_false(brw >= 0, "Can't read an unopened file");

	TC_PRINT("Filesystem has no read interface\n");
	err_filep.mp = &test_fs_mnt_no_op;
	brw = fs_read(&err_filep, read_buff, sz);
	zassert_false(brw >= 0, "Filesystem has no read interface");

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

/**
 * @brief fs_seek tests for expected ENOTSUP
 *
 * @ingroup filesystem_api
 */
void test_file_seek(void)
{
	struct fs_file_system_t backup = temp_fs;

	/* Simulate tell and lseek not implemented */
	temp_fs.lseek = NULL;
	temp_fs.tell = NULL;

	zassert_equal(fs_seek(&filep, 0, FS_SEEK_CUR),
		      -ENOTSUP,
		      "fs_seek not expected to be implemented");
	zassert_equal(fs_tell(&filep),
		      -ENOTSUP,
		      "fs_tell not expected to be implemented");

	/* Restore fs API interface */
	temp_fs = backup;
}

static int _test_file_truncate(void)
{
	int ret;
	off_t orig_pos;
	char read_buff[80];
	ssize_t brw;

	TC_PRINT("\nTruncate tests: max file size is 128byte\n");

	TC_PRINT("\nTruncate, seek, tell an unopened file\n");
	fs_file_t_init(&err_filep);
	ret = fs_truncate(&err_filep, 256);
	if (!ret) {
		return TC_FAIL;
	}
	ret = fs_seek(&err_filep, 0, FS_SEEK_END);
	if (!ret) {
		return TC_FAIL;
	}

	ret = fs_tell(&err_filep);
	if (!ret) {
		return TC_FAIL;
	}

	TC_PRINT("\nTruncate, seek, tell fs has no these functionality\n");
	err_filep.mp = &test_fs_mnt_no_op;
	ret = fs_truncate(&err_filep, 256);
	if (!ret) {
		return TC_FAIL;
	}
	ret = fs_seek(&err_filep, 0, FS_SEEK_END);
	if (!ret) {
		return TC_FAIL;
	}

	ret = fs_tell(&err_filep);
	if (!ret) {
		return TC_FAIL;
	}

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
 * @brief Truncate the file to the new length and check
 *
 * @details
 * - fs_seek(), locate the position to truncate
 * - fs_truncate()
 * - fs_tell(), retrieve the current position
 *
 * @ingroup filesystem_api
 */
void test_file_truncate(void)
{
	zassert_true(_test_file_truncate() == TC_PASS);
}

/**
 * @brief Test close file interface in file system core
 *
 * @ingroup filesystem_api
 */
void test_file_close(void)
{
	int ret;

	TC_PRINT("\nClose tests:\n");

	TC_PRINT("Close an unopened file\n");
	fs_file_t_init(&err_filep);
	ret = fs_close(&err_filep);
	zassert_equal(ret, 0, "Should close an unopened file");

	TC_PRINT("Filesystem has no close interface\n");
	err_filep.mp = &test_fs_mnt_no_op;
	ret = fs_close(&err_filep);
	zassert_not_equal(ret, 0, "Filesystem has no close interface");

	ret = fs_close(&filep);
	zassert_equal(ret, 0, "Fail to close file");

	TC_PRINT("Reuse fs_file_t from closed file");
	ret = fs_open(&filep, TEST_FILE, FS_O_READ);
	zassert_equal(ret, 0, "Expected open to succeed, got %d", ret);
	ret = fs_close(&filep);
	zassert_equal(ret, 0, "Expected close to succeed, got %d", ret);

	TC_PRINT("\nClose a closed file:\n");
	filep.mp = &test_fs_mnt_1;
	ret = fs_close(&filep);
	zassert_not_equal(ret, 0, "Should not reclose a closed file");

	TC_PRINT("Closed file %s\n", TEST_FILE);
}

/**
 * @brief Test fs_rename() interface in file system core
 *
 * @ingroup filesystem_api
 */
ZTEST(fs_api_dir_file, test_file_rename)
{
	int ret = TC_FAIL;

	TC_PRINT("\nRename file tests:\n");

	ret = fs_rename(NULL, NULL);
	zassert_not_equal(ret, 0, "Rename a NULL file");

	ret = fs_rename("/", TEST_FILE_RN);
	zassert_not_equal(ret, 0, "source file name is too short");

	ret = fs_rename("testfile.txt", TEST_FILE_RN);
	zassert_not_equal(ret, 0, "source file name doesn't start with /");

	ret = fs_rename("/SDCARD:/testfile.txt", NULL);
	zassert_not_equal(ret, 0, "Rename to a NULL file");

	ret = fs_rename("/SDCARD:/testfile.txt", "/");
	zassert_not_equal(ret, 0, "dest file name too short");

	ret = fs_rename("/SDCARD:/testfile.txt", "rename.txt");
	zassert_not_equal(ret, 0, "dest file name doesn't start with /");

	ret = fs_rename("/SDCARD:/testfile.txt", TEST_FILE_RN);
	zassert_not_equal(ret, 0, "Rename a not existing file");

	ret = fs_rename(TEST_FILE, "/SDCARD:/testfile_renamed.txt");
	zassert_not_equal(ret, 0, "Rename file to different mount point");

	ret = fs_rename(TEST_FILE, TEST_FILE_EX);
	zassert_not_equal(ret, 0, "Rename file to an exist file");

	ret = fs_rename(NOOP_MNTP"/test.txt", NOOP_MNTP"/test_new.txt");
	zassert_not_equal(ret, 0, "Filesystem has no rename functionality");

	ret = fs_rename(TEST_FILE, TEST_FILE_RN);
	zassert_equal(ret, 0, "Fail to rename a file");
}

/**
 * @brief Test fs_stat() interface in filesystem core
 *
 * @ingroup filesystem_api
 */
ZTEST(fs_api_dir_file, test_file_stat)
{
	int ret;
	struct fs_dirent entry;

	TC_PRINT("\nStat file tests:\n");

	ret = fs_stat(NULL, &entry);
	zassert_not_equal(ret, 0, "Pointer to path is NULL");

	ret = fs_stat(TEST_DIR, NULL);
	zassert_not_equal(ret, 0, "Stat a dir without entry");

	ret = fs_stat("/", &entry);
	zassert_not_equal(ret, 0, "dir path name is too short");

	ret = fs_stat("SDCARD", &entry);
	zassert_not_equal(ret, 0, "Stat a dir path without /");

	ret = fs_stat("/SDCARD", &entry);
	zassert_not_equal(ret, 0, "Stat a not existing dir");

	ret = fs_stat(NOOP_MNTP, &entry);
	zassert_not_equal(ret, 0, "filesystem has no stat functionality");

	ret = fs_stat(TEST_DIR, &entry);
	zassert_equal(ret, 0, "Fail to stat a dir");

	ret = fs_stat(TEST_DIR_FILE, &entry);
	zassert_equal(ret, 0, "Fail to stat a file");
}

/**
 * @brief Test fs_unlink() interface in filesystem core
 *
 * @ingroup filesystem_api
 */
ZTEST(fs_api_dir_file, test_file_unlink)
{
	int ret;

	TC_PRINT("\nDelete tests:\n");

	ret = fs_unlink(NULL);
	zassert_not_equal(ret, 0, "Delete a NULL file");

	ret = fs_unlink("/");
	zassert_not_equal(ret, 0, "Delete a file with too short name");

	ret = fs_unlink("SDCARD:/test_file.txt");
	zassert_not_equal(ret, 0, "Delete a file with missing root / in path");

	ret = fs_unlink("/SDCARD:/test_file.txt");
	zassert_not_equal(ret, 0, "Delete a not existing file");

	ret = fs_unlink(TEST_FS_MNTP);
	zassert_not_equal(ret, 0, "Delete a root dir");

	ret = fs_unlink(NOOP_MNTP);
	zassert_not_equal(ret, 0, "Filesystem has no unlink functionality");

	/* In filesystem core, static function fs_get_mnt_point() check
	 * the length of mount point's name. It is not an API, test this
	 * function at this point because test_file_unlink() is the
	 * last test case before test_unmount(), change mountp_len of
	 * test_fs_mnt_no_op to 0 could not effect other test cases
	 */
	test_fs_mnt_no_op.mountp_len = 0;
	ret = fs_unlink(NOOP_MNTP);
	zassert_not_equal(ret, 0, "mount point with 0 mountp_len can't be get");

	ret = fs_unlink(TEST_FILE_RN);
	zassert_equal(ret, 0, "Fail to delete file");

	TC_PRINT("File (%s) deleted successfully!\n", TEST_FILE_RN);
}

static void *fs_api_setup(void)
{
	fs_register(TEST_FS_1, &temp_fs);
	fs_mount(&test_fs_mnt_1);
	memset(&null_fs, 0, sizeof(null_fs));
	null_fs.mount = temp_fs.mount;
	null_fs.unmount = temp_fs.unmount;
	fs_register(TEST_FS_2, &null_fs);
	fs_mount(&test_fs_mnt_no_op);
	return NULL;
}

static void fs_api_teardown(void *fixtrue)
{
	fs_unmount(&test_fs_mnt_no_op);
	fs_unregister(TEST_FS_2, &null_fs);
	fs_unmount(&test_fs_mnt_1);
	fs_unregister(TEST_FS_1, &temp_fs);
}

ZTEST(fs_api_dir_file, test_fs_dir)
{
	test_mkdir();
	test_opendir();
	test_closedir();
	test_opendir_closedir();
	test_lsdir();
}

ZTEST(fs_api_dir_file, test_file_ops)
{
	test_file_open();
	test_file_write();
	test_file_read();
	test_file_seek();
	test_file_truncate();
	test_file_close();
}

ZTEST(fs_api_register_mount, test_mount_unmount)
{
	fs_register(TEST_FS_1, &temp_fs);
	test_mount();
	test_unmount();
	fs_unregister(TEST_FS_1, &temp_fs);
}

ZTEST_SUITE(fs_api_register_mount, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(fs_api_dir_file, NULL, fs_api_setup, NULL, NULL, fs_api_teardown);
