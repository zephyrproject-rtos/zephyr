/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Directory littlefs operations:
 * * mkdir
 * * opendir
 * * readdir
 * * closedir
 * * rename
 */

#include <string.h>
#include <zephyr/ztest.h>
#include "testfs_tests.h"
#include "testfs_lfs.h"
#include <lfs.h>

#include <zephyr/fs/littlefs.h>

void test_fs_dirops(void);

/* Mount structure needed by test_fs_basic tests. */
struct fs_mount_t *fs_dirops_test_mp = &testfs_small_mnt;

ZTEST(littlefs, test_lfs_dirops)
{
	struct fs_mount_t *mp = &testfs_small_mnt;

	zassert_equal(testfs_lfs_wipe_partition(mp),
		      TC_PASS,
		      "failed to wipe partition");

	/* Common dirops tests.
	 * (File system is mounted and unmounted during that test.)
	 */
	test_fs_dirops();
}

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(root_lfs);
static struct fs_mount_t root_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &root_lfs,
	.mnt_point = "/",
};

ZTEST(littlefs, test_lfs_opendir_root_mount)
{
	struct fs_file_t file;
	struct fs_dir_t dir;
	struct fs_dirent entry;
	bool found = false;
	int rc;

	root_mnt.storage_dev = testfs_small_mnt.storage_dev;
	zassert_equal(testfs_lfs_wipe_partition(&root_mnt), TC_PASS, "failed to wipe partition");
	zassert_equal(fs_mount(&root_mnt), 0, "failed to mount at /");

	fs_file_t_init(&file);
	zassert_equal(fs_open(&file, "/root_file", FS_O_CREATE | FS_O_WRITE), 0,
		      "failed to create /root_file");
	zassert_equal(fs_close(&file), 0, "failed to close /root_file");

	fs_dir_t_init(&dir);
	zassert_equal(fs_opendir(&dir, "/"), 0, "failed to open /");
	do {
		rc = fs_readdir(&dir, &entry);
		if ((rc == 0) && (strcmp(entry.name, "root_file") == 0)) {
			found = true;
		}
	} while ((rc == 0) && (entry.name[0] != '\0'));
	zassert_equal(fs_closedir(&dir), 0, "failed to close /");
	zassert_equal(fs_unmount(&root_mnt), 0, "failed to unmount /");

	zassert_equal(rc, 0, "readdir of / failed: %d", rc);
	zassert_true(found, "root_file not listed by readdir of /");
}
