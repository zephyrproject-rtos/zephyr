/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Director littlefs operations:
 * * mkdir
 * * opendir
 * * readdir
 * * closedir
 * * rename
 */

#include <string.h>
#include <ztest.h>
#include "testfs_tests.h"
#include "testfs_lfs.h"
#include <lfs.h>

#include <fs/littlefs.h>

static struct testfs_bcmd test_hierarchy[] = {
	TESTFS_BCMD_FILE("f1", 1, 1),
	TESTFS_BCMD_FILE("f2", 2, 100),
	TESTFS_BCMD_ENTER_DIR("d1"),
	TESTFS_BCMD_FILE("d1f1", 11, 23),
	TESTFS_BCMD_FILE("d1f2", 12, 612),
	TESTFS_BCMD_EXIT_DIR(),
	TESTFS_BCMD_FILE("f3", 3, 10000),
	TESTFS_BCMD_END(),
};

static int clean_mount(struct fs_mount_t *mp)
{
	TC_PRINT("checking clean mount\n");

	zassert_equal(testfs_lfs_wipe_partition(mp),
		      TC_PASS,
		      "failed to wipe partition");
	zassert_equal(fs_mount(mp), 0,
		      "mount small failed");

	return TC_PASS;
}

static int check_mkdir(struct fs_mount_t *mp)
{
	struct testfs_path dpath;

	TC_PRINT("checking dir create unlink\n");
	zassert_equal(testfs_path_init(&dpath, mp,
				       "dir",
				       TESTFS_PATH_END),
		      dpath.path,
		      "root init failed");

	zassert_equal(fs_mkdir(dpath.path),
		      0,
		      "mkdir failed");

	struct fs_file_t file;
	struct testfs_path fpath;

	zassert_equal(fs_open(&file,
			      testfs_path_extend(testfs_path_copy(&fpath,
								  &dpath),
						 "file",
						 TESTFS_PATH_END),
			      FS_O_CREATE | FS_O_RDWR),
		      0,
		      "creat in dir failed");
	zassert_equal(fs_close(&file), 0,
		      "close file failed");

	struct fs_dirent stat;

	zassert_equal(fs_stat(fpath.path, &stat), 0,
		      "stat file failed");

	zassert_equal(fs_unlink(dpath.path),
		      -ENOTEMPTY,
		      "unlink bad failure");

	zassert_equal(fs_unlink(fpath.path),
		      0,
		      "unlink file failed");

	zassert_equal(fs_unlink(dpath.path),
		      0,
		      "unlink dir failed");

	return TC_PASS;
}

static int build_layout(struct fs_mount_t *mp,
			const struct testfs_bcmd *cp)
{
	struct testfs_path path;
	struct fs_statvfs stat;

	TC_PRINT("building layout on %s\n", mp->mnt_point);

	zassert_equal(fs_statvfs(mp->mnt_point, &stat), 0,
		      "statvfs failed");

	TC_PRINT("before: bsize %lu ; frsize %lu ; blocks %lu ; bfree %lu\n",
		 stat.f_bsize, stat.f_frsize, stat.f_blocks, stat.f_bfree);

	zassert_equal(testfs_path_init(&path, mp, TESTFS_PATH_END),
		      path.path,
		      "root init failed");

	zassert_equal(testfs_build(&path, cp),
		      0,
		      "build_layout failed");

	zassert_equal(fs_statvfs(mp->mnt_point, &stat), 0,
		      "statvfs failed");

	TC_PRINT("after: bsize %lu ; frsize %lu ; blocks %lu ; bfree %lu\n",
		 stat.f_bsize, stat.f_frsize, stat.f_blocks, stat.f_bfree);

	return TC_PASS;
}

static int check_layout(struct fs_mount_t *mp,
			struct testfs_bcmd *layout)
{
	struct testfs_path path;
	struct testfs_bcmd *end_layout = testfs_bcmd_end(layout);

	TC_PRINT("checking layout\n");

	zassert_equal(testfs_path_init(&path, mp, TESTFS_PATH_END),
		      path.path,
		      "root init failed");

	int rc = testfs_bcmd_verify_layout(&path, layout, end_layout);

	zassert_true(rc >= 0, "layout check failed");

	zassert_equal(rc, 0,
		      "layout found foreign");

	struct testfs_bcmd *cp = layout;

	while (!TESTFS_BCMD_IS_END(cp)) {
		if (cp->name != NULL) {
			TC_PRINT("verifying %s%s %u\n",
				 cp->name,
				 (cp->type == FS_DIR_ENTRY_DIR) ? "/" : "",
				 cp->size);
			zassert_true(cp->matched,
				     "Unmatched layout entry");
		}
		++cp;
	}

	return TC_PASS;
}

static int check_rename(struct fs_mount_t *mp)
{
	struct testfs_path root;
	struct testfs_path from_path;
	const char *from = from_path.path;
	struct testfs_path to_path;
	const char *to = to_path.path;
	struct fs_dirent stat;
	struct testfs_bcmd from_bcmd[] = {
		TESTFS_BCMD_FILE("f1f", 1, 8),  /* from f1f to f1t */
		TESTFS_BCMD_FILE("f2f", 2, 8),  /* from f2f to f2t */
		TESTFS_BCMD_FILE("f2t", 3, 8),  /* target for f2f clobber, replaced */
		TESTFS_BCMD_FILE("f3f", 4, 8),  /* from f3f to d1f/d1f2t */
		TESTFS_BCMD_FILE("f4f", 5, 8),  /* from f4f to d2t, reject */
		TESTFS_BCMD_ENTER_DIR("d1f"),   /* from d1f to d1t */
		TESTFS_BCMD_FILE("d1f1", 5, 16),
		TESTFS_BCMD_EXIT_DIR(),
		TESTFS_BCMD_ENTER_DIR("d2t"),  /* target for d1f, unchanged */
		TESTFS_BCMD_FILE("d2f1", 6, 16),
		TESTFS_BCMD_EXIT_DIR(),
		TESTFS_BCMD_END(),
	};
	struct testfs_bcmd *from_end_bcmd = testfs_bcmd_end(from_bcmd);
	struct testfs_bcmd to_bcmd[] = {
		TESTFS_BCMD_FILE("f1t", 1, 8),          /* from f1f to f1t */
		TESTFS_BCMD_FILE("f2t", 2, 8),          /* from f2f to f2t */
		TESTFS_BCMD_FILE("f4f", 5, 8),          /* from f4f to d2t, reject */
		TESTFS_BCMD_ENTER_DIR("d1t"),           /* from d1f to d1t */
		TESTFS_BCMD_FILE("d1f1", 5, 16),
		TESTFS_BCMD_FILE("d1f2t", 4, 8),        /* from f3f to d1f/d1f2t */
		TESTFS_BCMD_EXIT_DIR(),
		TESTFS_BCMD_ENTER_DIR("d2t"),           /* target for d1f, unchanged */
		TESTFS_BCMD_FILE("d2f1", 6, 16),
		TESTFS_BCMD_EXIT_DIR(),
		TESTFS_BCMD_END(),
	};
	struct testfs_bcmd *to_end_bcmd = testfs_bcmd_end(to_bcmd);

	zassert_equal(testfs_path_init(&root, mp,
				       "rename",
				       TESTFS_PATH_END),
		      root.path,
		      "root init failed");

	zassert_equal(fs_mkdir(root.path),
		      0,
		      "rename mkdir failed");
	zassert_equal(testfs_build(&root, from_bcmd),
		      0,
		      "rename build failed");

	zassert_equal(testfs_bcmd_verify_layout(&root, from_bcmd, from_end_bcmd),
		      0,
		      "layout check failed");

	testfs_path_extend(testfs_path_copy(&from_path, &root),
			   "nofile",
			   TESTFS_PATH_END);
	testfs_path_extend(testfs_path_copy(&to_path, &root),
			   "f1t",
			   TESTFS_PATH_END);
	TC_PRINT("%s => %s -ENOENT\n", from, to);
	zassert_equal(fs_rename(from, to),
		      -ENOENT,
		      "rename noent failed");

	/* f1f => f1t : ok */
	testfs_path_extend(testfs_path_copy(&from_path, &root),
			   "f1f",
			   TESTFS_PATH_END);
	TC_PRINT("%s => %s ok\n", from, to);
	zassert_equal(fs_rename(from, to),
		      0,
		      "rename noent failed");

	/* f2f => f2t : clobbers */
	testfs_path_extend(testfs_path_copy(&from_path, &root),
			   "f2f",
			   TESTFS_PATH_END);
	testfs_path_extend(testfs_path_copy(&to_path, &root),
			   "f2t",
			   TESTFS_PATH_END);
	TC_PRINT("%s => %s clobber ok\n", from, to);
	zassert_equal(fs_rename(from, to),
		      0,
		      "rename clobber failed");
	zassert_equal(fs_stat(from, &stat),
		      -ENOENT,
		      "rename clobber left from");

	/* f3f => d1f/d1f2t : moves */
	testfs_path_extend(testfs_path_copy(&from_path, &root),
			   "f3f",
			   TESTFS_PATH_END);
	testfs_path_extend(testfs_path_copy(&to_path, &root),
			   "d1f", "d1f2t",
			   TESTFS_PATH_END);
	TC_PRINT("%s => %s move ok\n", from, to);
	zassert_equal(fs_rename(from, to),
		      0,
		      "rename to subdir failed");
	zassert_equal(fs_stat(from, &stat),
		      -ENOENT,
		      "rename to subdir left from");

	/* d1f => d2t : reject */
	testfs_path_extend(testfs_path_copy(&from_path, &root),
			   "d1f",
			   TESTFS_PATH_END);
	testfs_path_extend(testfs_path_copy(&to_path, &root),
			   "d2t",
			   TESTFS_PATH_END);
	TC_PRINT("%s => %s -ENOTEMPTY\n", from, to);
	zassert_equal(fs_rename(from, to),
		      -ENOTEMPTY,
		      "rename to existing dir failed");

	/* d1f => d1t : rename */
	testfs_path_extend(testfs_path_copy(&from_path, &root),
			   "d1f",
			   TESTFS_PATH_END);
	testfs_path_extend(testfs_path_copy(&to_path, &root),
			   "d1t",
			   TESTFS_PATH_END);
	TC_PRINT("%s => %s ok\n", from, to);
	zassert_equal(fs_rename(from, to),
		      0,
		      "rename to new dir failed");
	zassert_equal(fs_stat(from, &stat),
		      -ENOENT,
		      "rename to new dir left from");

	zassert_equal(testfs_bcmd_verify_layout(&root, to_bcmd, to_end_bcmd),
		      0,
		      "layout verification failed");

	struct testfs_bcmd *cp = to_bcmd;

	while (cp != to_end_bcmd) {
		if (cp->name) {
			zassert_true(cp->matched, "foriegn file retained");
		}
		++cp;
	}

	return TC_PASS;
}

void test_lfs_dirops(void)
{
	struct fs_mount_t *mp = &testfs_small_mnt;

	zassert_equal(clean_mount(mp), TC_PASS,
		      "clean mount failed");

	zassert_equal(check_mkdir(mp), TC_PASS,
		      "check mkdir failed");

	k_sleep(K_MSEC(100));   /* flush log messages */
	zassert_equal(build_layout(mp, test_hierarchy), TC_PASS,
		      "build test hierarchy failed");

	k_sleep(K_MSEC(100));   /* flush log messages */
	zassert_equal(check_layout(mp, test_hierarchy), TC_PASS,
		      "check test hierarchy failed");

	k_sleep(K_MSEC(100));   /* flush log messages */
	zassert_equal(check_rename(mp), TC_PASS,
		      "check rename failed");

	k_sleep(K_MSEC(100));   /* flush log messages */
	zassert_equal(fs_unmount(mp), 0,
		      "unmount small failed");
}
