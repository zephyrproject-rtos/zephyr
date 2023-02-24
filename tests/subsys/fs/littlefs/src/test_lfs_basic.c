/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Basic littlefs operations:
 * * create
 * * write
 * * stat
 * * read
 * * seek
 * * tell
 * * truncate
 * * unlink
 * * sync
 */

#include <string.h>
#include <zephyr/ztest.h>
#include "testfs_tests.h"
#include "testfs_lfs.h"
#include <lfs.h>

#include <zephyr/fs/littlefs.h>

static int mount(struct fs_mount_t *mp)
{
	TC_PRINT("mounting %s\n", mp->mnt_point);

	zassert_equal(fs_mount(mp), 0,
		      "mount failed");

	return TC_PASS;
}

static int clear_partition(struct fs_mount_t *mp)
{
	TC_PRINT("clearing partition %s\n", mp->mnt_point);

	zassert_equal(testfs_lfs_wipe_partition(mp),
		      TC_PASS,
		      "failed to wipe partition");

	return TC_PASS;
}

static int clean_statvfs(const struct fs_mount_t *mp)
{
	struct fs_statvfs stat;

	TC_PRINT("checking clean statvfs of %s\n", mp->mnt_point);

	zassert_equal(fs_statvfs(mp->mnt_point, &stat), 0,
		      "statvfs failed");

	TC_PRINT("%s: bsize %lu ; frsize %lu ; blocks %lu ; bfree %lu\n",
		 mp->mnt_point,
		 stat.f_bsize, stat.f_frsize, stat.f_blocks, stat.f_bfree);
	zassert_equal(stat.f_bsize, 16,
		      "bsize fail");
	zassert_equal(stat.f_frsize, 4096,
		      "frsize fail");
	zassert_equal(stat.f_blocks, 16,
		      "blocks fail");
	zassert_equal(stat.f_bfree, stat.f_blocks - 2U,
		      "bfree fail");

	return TC_PASS;
}

static int check_medium(void)
{
	struct fs_mount_t *mp = &testfs_medium_mnt;
	struct fs_statvfs stat;

	zassert_equal(clear_partition(mp), TC_PASS,
		      "clear partition failed");

	zassert_equal(fs_mount(mp), 0,
		      "medium mount failed");

	zassert_equal(fs_statvfs(mp->mnt_point, &stat), 0,
		      "statvfs failed");

	TC_PRINT("%s: bsize %lu ; frsize %lu ; blocks %lu ; bfree %lu\n",
		 mp->mnt_point,
		 stat.f_bsize, stat.f_frsize, stat.f_blocks, stat.f_bfree);
	zassert_equal(stat.f_bsize, MEDIUM_IO_SIZE,
		      "bsize fail");
	zassert_equal(stat.f_frsize, 4096,
		      "frsize fail");
	zassert_equal(stat.f_blocks, 240,
		      "blocks fail");
	zassert_equal(stat.f_bfree, stat.f_blocks - 2U,
		      "bfree fail");

	zassert_equal(fs_unmount(mp), 0,
		      "medium unmount failed");

	return TC_PASS;
}

static int check_large(void)
{
	struct fs_mount_t *mp = &testfs_large_mnt;
	struct fs_statvfs stat;

	zassert_equal(clear_partition(mp), TC_PASS,
		      "clear partition failed");

	zassert_equal(fs_mount(mp), 0,
		      "large mount failed");

	zassert_equal(fs_statvfs(mp->mnt_point, &stat), 0,
		      "statvfs failed");

	TC_PRINT("%s: bsize %lu ; frsize %lu ; blocks %lu ; bfree %lu\n",
		 mp->mnt_point,
		 stat.f_bsize, stat.f_frsize, stat.f_blocks, stat.f_bfree);
	zassert_equal(stat.f_bsize, LARGE_IO_SIZE,
		      "bsize fail");
	zassert_equal(stat.f_frsize, 32768,
		      "frsize fail");
	zassert_equal(stat.f_blocks, 96,
		      "blocks fail");
	zassert_equal(stat.f_bfree, stat.f_blocks - 2U,
		      "bfree fail");

	zassert_equal(fs_unmount(mp), 0,
		      "large unmount failed");

	return TC_PASS;
}

static int num_files(struct fs_mount_t *mp)
{
	struct testfs_path path;
	char name[2] = { 0 };
	const char *pstr;
	struct fs_file_t files[CONFIG_FS_LITTLEFS_NUM_FILES];
	size_t fi = 0;
	int rc;

	memset(files, 0, sizeof(files));

	TC_PRINT("CONFIG_FS_LITTLEFS_NUM_FILES=%u\n", CONFIG_FS_LITTLEFS_NUM_FILES);
	while (fi < ARRAY_SIZE(files)) {
		struct fs_file_t *const file = &files[fi];

		name[0] = 'A' + fi;
		pstr = testfs_path_init(&path, mp,
					name,
					TESTFS_PATH_END);

		TC_PRINT("opening %s\n", pstr);
		rc = fs_open(file, pstr, FS_O_CREATE | FS_O_RDWR);
		zassert_equal(rc, 0, "open %s failed: %d", pstr, rc);

		rc = testfs_write_incrementing(file, 0, TESTFS_BUFFER_SIZE);
		zassert_equal(rc, TESTFS_BUFFER_SIZE, "write %s failed: %d", pstr, rc);

		++fi;
	}

	while (fi-- != 0)  {
		struct fs_file_t *const file = &files[fi];

		name[0] = 'A' + fi;
		pstr = testfs_path_init(&path, mp,
					name,
					TESTFS_PATH_END);

		TC_PRINT("Close and unlink %s\n", pstr);

		rc = fs_close(file);
		zassert_equal(rc, 0, "close %s failed: %d", pstr, rc);

		rc = fs_unlink(pstr);
		zassert_equal(rc, 0, "unlink %s failed: %d", pstr, rc);
	}

	return TC_PASS;
}

static int num_dirs(struct fs_mount_t *mp)
{
	struct testfs_path path;
	char name[3] = "Dx";
	const char *pstr;
	struct fs_dir_t dirs[CONFIG_FS_LITTLEFS_NUM_DIRS];
	size_t di = 0;
	int rc;

	memset(dirs, 0, sizeof(dirs));

	TC_PRINT("CONFIG_FS_LITTLEFS_NUM_DIRS=%u\n", CONFIG_FS_LITTLEFS_NUM_DIRS);
	while (di < ARRAY_SIZE(dirs)) {
		struct fs_dir_t *const dir = &dirs[di];

		name[1] = 'A' + di;
		pstr = testfs_path_init(&path, mp,
					name,
					TESTFS_PATH_END);

		TC_PRINT("making and opening directory %s\n", pstr);
		rc = fs_mkdir(pstr);
		zassert_equal(rc, 0, "mkdir %s failed: %d", pstr, rc);

		rc = fs_opendir(dir, pstr);
		zassert_equal(rc, 0, "opendir %s failed: %d", name, rc);

		++di;
	}

	while (di-- != 0)  {
		struct fs_dir_t *const dir = &dirs[di];

		name[1] = 'A' + di;
		pstr = testfs_path_init(&path, mp,
					name,
					TESTFS_PATH_END);

		TC_PRINT("Close and rmdir %s\n", pstr);

		rc = fs_closedir(dir);
		zassert_equal(rc, 0, "closedir %s failed: %d", name, rc);

		rc = fs_unlink(pstr);
		zassert_equal(rc, 0, "unlink %s failed: %d", name, rc);
	}

	return TC_PASS;
}

void test_fs_basic(void);

/* Mount structure needed by test_fs_basic tests. */
struct fs_mount_t *fs_basic_test_mp = &testfs_small_mnt;

ZTEST(littlefs, test_lfs_basic)
{
	struct fs_mount_t *mp = &testfs_small_mnt;

	zassert_equal(clear_partition(mp), TC_PASS,
		      "clear partition failed");

	/* Common basic tests.
	 * (File system is mounted and unmounted during that test.)
	 */
	test_fs_basic();

	/* LittleFS specific tests */

	zassert_equal(mount(mp), TC_PASS,
		      "clean mount failed");

	zassert_equal(clean_statvfs(mp), TC_PASS,
		      "clean statvfs failed");

	zassert_equal(num_files(mp), TC_PASS,
		      "num_files failed");

	zassert_equal(num_dirs(mp), TC_PASS,
		      "num_dirs failed");

	TC_PRINT("unmounting %s\n", mp->mnt_point);
	zassert_equal(fs_unmount(mp), 0,
		      "unmount small failed");

	if (IS_ENABLED(CONFIG_APP_TEST_CUSTOM)) {
		zassert_equal(check_medium(), TC_PASS,
			      "check medium failed");

		zassert_equal(check_large(), TC_PASS,
			      "check large failed");
	}
}
