/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2023 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>

#include "test_fs_util.h"

#define HELLO "hello"
#define GOODBYE "goodbye"

/* Mount point for the tests should be provided by test runner.
 * File system should be mounted before tests are started.
 */
extern struct fs_mount_t *fs_basic_test_mp;

static int create_write_hello(const struct fs_mount_t *mp)
{
	struct testfs_path path;
	struct fs_file_t file;

	fs_file_t_init(&file);
	TC_PRINT("creating and writing file\n");

	zassert_equal(fs_open(&file,
			      testfs_path_init(&path, mp,
					       HELLO,
					       TESTFS_PATH_END),
			      FS_O_CREATE | FS_O_RDWR),
		      0,
		      "open hello failed");

	struct fs_dirent stat;

	zassert_equal(fs_stat(path.path, &stat),
		      0,
		      "stat new hello failed");

	zassert_equal(stat.type, FS_DIR_ENTRY_FILE,
		      "stat new hello not file");
	zassert_equal(strcmp(stat.name, HELLO), 0,
		      "stat new hello not hello");
	zassert_equal(stat.size, 0,
		      "stat new hello not empty");

	zassert_equal(testfs_write_incrementing(&file, 0, TESTFS_BUFFER_SIZE),
		      TESTFS_BUFFER_SIZE,
		      "write constant failed");

	zassert_equal(fs_stat(path.path, &stat),
		      0,
		      "stat written hello failed");

	zassert_equal(stat.type, FS_DIR_ENTRY_FILE,
		      "stat written hello not file");
	zassert_equal(strcmp(stat.name, HELLO), 0,
		      "stat written hello not hello");

	/* Anomalous behavior requiring upstream response */
	if (mp->type == FS_LITTLEFS) {
		/* VARIATION POINT: littlefs does not update the file size of
		 * an open file.  See upstream issue #250.
		 */
		zassert_equal(stat.size, 0,
			      "stat written hello bad size");
	}

	zassert_equal(fs_close(&file), 0,
		      "close hello failed");

	zassert_equal(fs_stat(path.path, &stat),
		      0,
		      "stat closed hello failed");

	zassert_equal(stat.type, FS_DIR_ENTRY_FILE,
		      "stat closed hello not file");
	zassert_equal(strcmp(stat.name, HELLO), 0,
		      "stat closed hello not hello");
	zassert_equal(stat.size, TESTFS_BUFFER_SIZE,
		      "stat closed hello badsize");

	return TC_PASS;
}

static int verify_hello(const struct fs_mount_t *mp)
{
	struct testfs_path path;
	struct fs_file_t file;

	fs_file_t_init(&file);
	TC_PRINT("opening and verifying file\n");

	zassert_equal(fs_open(&file,
			      testfs_path_init(&path, mp,
					       HELLO,
					       TESTFS_PATH_END),
			      FS_O_CREATE | FS_O_RDWR),
		      0,
		      "verify hello open failed");

	zassert_equal(fs_tell(&file), 0U,
		      "verify hello open tell failed");

	zassert_equal(testfs_verify_incrementing(&file, 0, TESTFS_BUFFER_SIZE),
		      TESTFS_BUFFER_SIZE,
		      "verify hello at start failed");

	zassert_equal(fs_tell(&file), TESTFS_BUFFER_SIZE,
		      "verify hello read tell failed");

	zassert_equal(fs_close(&file), 0,
		      "verify close hello failed");

	return TC_PASS;
}

static int seek_within_hello(const struct fs_mount_t *mp)
{
	struct testfs_path path;
	struct fs_file_t file;

	fs_file_t_init(&file);
	TC_PRINT("seek and tell in file\n");

	zassert_equal(fs_open(&file,
			      testfs_path_init(&path, mp,
					       HELLO,
					       TESTFS_PATH_END),
			      FS_O_CREATE | FS_O_RDWR),
		      0,
		      "verify hello open failed");

	zassert_equal(fs_tell(&file), 0U,
		      "verify hello open tell failed");

	struct fs_dirent stat;

	zassert_equal(fs_stat(path.path, &stat),
		      0,
		      "stat old hello failed");
	zassert_equal(stat.size, TESTFS_BUFFER_SIZE,
		      "stat old hello bad size");

	off_t pos = stat.size / 4;

	zassert_equal(fs_seek(&file, pos, FS_SEEK_SET),
		      0,
		      "verify hello seek near mid failed");

	zassert_equal(fs_tell(&file), pos,
		      "verify hello tell near mid failed");

	zassert_equal(testfs_verify_incrementing(&file, pos, TESTFS_BUFFER_SIZE),
		      TESTFS_BUFFER_SIZE - pos,
		      "verify hello at middle failed");

	zassert_equal(fs_tell(&file), stat.size,
		      "verify hello read middle tell failed");

	zassert_equal(fs_seek(&file, -stat.size, FS_SEEK_CUR),
		      0,
		      "verify hello seek back from cur failed");

	zassert_equal(fs_tell(&file), 0U,
		      "verify hello tell back from cur failed");

	zassert_equal(fs_seek(&file, -pos, FS_SEEK_END),
		      0,
		      "verify hello seek from end failed");

	zassert_equal(fs_tell(&file), stat.size - pos,
		      "verify hello tell from end failed");

	zassert_equal(testfs_verify_incrementing(&file, stat.size - pos,
						 TESTFS_BUFFER_SIZE),
		      pos,
		      "verify hello at post middle failed");

	zassert_equal(fs_close(&file), 0,
		      "verify close hello failed");

	return TC_PASS;
}

static int truncate_hello(const struct fs_mount_t *mp)
{
	struct testfs_path path;
	struct fs_file_t file;

	fs_file_t_init(&file);
	TC_PRINT("truncate in file\n");

	zassert_equal(fs_open(&file,
			      testfs_path_init(&path, mp,
					       HELLO,
					       TESTFS_PATH_END),
			      FS_O_CREATE | FS_O_RDWR),
		      0,
		      "verify hello open failed");

	struct fs_dirent stat;

	zassert_equal(fs_stat(path.path, &stat),
		      0,
		      "stat old hello failed");
	zassert_equal(stat.size, TESTFS_BUFFER_SIZE,
		      "stat old hello bad size");

	off_t pos = 3 * stat.size / 4;

	zassert_equal(fs_tell(&file), 0U,
		      "truncate initial tell failed");

	zassert_equal(fs_truncate(&file, pos),
		      0,
		      "truncate 3/4 failed");

	zassert_equal(fs_tell(&file), 0U,
		      "truncate post tell failed");

	zassert_equal(fs_stat(path.path, &stat),
		      0,
		      "stat open 3/4 failed");

	/* Anomalous behavior requiring upstream response */
	if (mp->type == FS_LITTLEFS) {
		/* VARIATION POINT: littlefs does not update the file size of
		 * an open file.  See upstream issue #250.
		 */
		zassert_equal(stat.size, TESTFS_BUFFER_SIZE,
			      "stat open 3/4 bad size");
	}

	zassert_equal(testfs_verify_incrementing(&file, 0, 64),
		      48,
		      "post truncate content unexpected");

	zassert_equal(fs_close(&file), 0,
		      "post truncate close failed");

	/* After close size is correct. */
	zassert_equal(fs_stat(path.path, &stat),
		      0,
		      "stat closed truncated failed");
	zassert_equal(stat.size, pos,
		      "stat closed truncated bad size");

	return TC_PASS;
}

static int unlink_hello(const struct fs_mount_t *mp)
{
	struct testfs_path path;

	TC_PRINT("unlink hello\n");

	testfs_path_init(&path, mp,
			 HELLO,
			 TESTFS_PATH_END);

	struct fs_dirent stat;

	zassert_equal(fs_stat(path.path, &stat),
		      0,
		      "stat existing hello failed");
	zassert_equal(fs_unlink(path.path),
		      0,
		      "unlink hello failed");
	zassert_equal(fs_stat(path.path, &stat),
		      -ENOENT,
		      "stat existing hello failed");

	return TC_PASS;
}

static int sync_goodbye(const struct fs_mount_t *mp)
{
	struct testfs_path path;
	struct fs_file_t file;

	fs_file_t_init(&file);
	TC_PRINT("sync goodbye\n");

	zassert_equal(fs_open(&file,
			      testfs_path_init(&path, mp,
					       GOODBYE,
					       TESTFS_PATH_END),
			      FS_O_CREATE | FS_O_RDWR),
		      0,
		      "sync goodbye failed");

	struct fs_dirent stat;

	zassert_equal(fs_stat(path.path, &stat),
		      0,
		      "stat existing hello failed");
	zassert_equal(stat.size, 0,
		      "stat new goodbye not empty");

	zassert_equal(testfs_write_incrementing(&file, 0, TESTFS_BUFFER_SIZE),
		      TESTFS_BUFFER_SIZE,
		      "write goodbye failed");

	zassert_equal(fs_tell(&file), TESTFS_BUFFER_SIZE,
		      "tell goodbye failed");

	if (true && mp->type == FS_LITTLEFS) {
		/* Upstream issue #250 */
		zassert_equal(stat.size, 0,
			      "stat new goodbye not empty");
	}

	zassert_equal(fs_sync(&file), 0,
		      "sync goodbye failed");

	zassert_equal(fs_tell(&file), TESTFS_BUFFER_SIZE,
		      "tell synced moved");

	zassert_equal(fs_stat(path.path, &stat),
		      0,
		      "stat existing hello failed");
	printk("sync size %u\n", (uint32_t)stat.size);

	zassert_equal(stat.size, TESTFS_BUFFER_SIZE,
		      "stat synced goodbye not correct");

	zassert_equal(fs_close(&file), 0,
		      "post sync close failed");

	/* After close size is correct. */
	zassert_equal(fs_stat(path.path, &stat),
		      0,
		      "stat sync failed");
	zassert_equal(stat.size, TESTFS_BUFFER_SIZE,
		      "stat sync bad size");

	return TC_PASS;
}

static int verify_goodbye(const struct fs_mount_t *mp)
{
	struct testfs_path path;
	struct fs_file_t file;

	fs_file_t_init(&file);
	TC_PRINT("verify goodbye\n");

	zassert_equal(fs_open(&file,
			      testfs_path_init(&path, mp,
					       GOODBYE,
					       TESTFS_PATH_END),
			      FS_O_CREATE | FS_O_RDWR),
		      0,
		      "verify goodbye failed");

	zassert_equal(testfs_verify_incrementing(&file, 0, TESTFS_BUFFER_SIZE),
		      TESTFS_BUFFER_SIZE,
		      "write goodbye failed");

	zassert_equal(fs_close(&file), 0,
		      "post sync close failed");

	return TC_PASS;
}

void test_fs_basic(void)
{

	zassert_equal(fs_mount(fs_basic_test_mp), 0,
		      "mount failed");

	zassert_equal(create_write_hello(fs_basic_test_mp), TC_PASS,
		      "write hello failed");

	zassert_equal(verify_hello(fs_basic_test_mp), TC_PASS,
		      "verify hello failed");

	zassert_equal(seek_within_hello(fs_basic_test_mp), TC_PASS,
		      "seek within hello failed");

	zassert_equal(truncate_hello(fs_basic_test_mp), TC_PASS,
		      "truncate hello failed");

	zassert_equal(unlink_hello(fs_basic_test_mp), TC_PASS,
		      "unlink hello failed");

	zassert_equal(sync_goodbye(fs_basic_test_mp), TC_PASS,
		      "sync goodbye failed");

	TC_PRINT("unmounting %s\n", fs_basic_test_mp->mnt_point);
	zassert_equal(fs_unmount(fs_basic_test_mp), 0,
		      "unmount small failed");

	k_sleep(K_MSEC(100));   /* flush log messages */
	TC_PRINT("checking double unmount diagnoses\n");

	zassert_equal(fs_unmount(fs_basic_test_mp), -EINVAL,
		      "unmount unmounted failed");

	zassert_equal(fs_mount(fs_basic_test_mp), 0,
		      "mount failed");

	zassert_equal(verify_goodbye(fs_basic_test_mp), TC_PASS,
		      "verify goodbye failed");

	zassert_equal(fs_unmount(fs_basic_test_mp), 0,
		      "unmount2 small failed");
}
