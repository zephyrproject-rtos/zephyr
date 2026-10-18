/*
 * Copyright (c) 2026 Rishav Chakraborty.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fs.h"
#include <string.h>

static struct test_fs_data test_data;
static struct fs_mount_t test_fs_mnt_1 = {
	.type = TEST_FS_1,
	.mnt_point = TEST_FS_MNTP,
	.fs_data = &test_data,
};

struct fs_utils_ent {
	sys_dnode_t node;
	struct fs_dirent ent;
};

/* list of directory entries */
static sys_dlist_t fs_utils_ent_list = SYS_DLIST_STATIC_INIT(&fs_utils_ent_list);

static struct fs_file_system_t fs_utils_fs;

static int fs_utils_stat(struct fs_mount_t *mountp, const char *path, struct fs_dirent *entry)
{
	struct fs_utils_ent *itr;
	sys_dnode_t *node;

	if (mountp == NULL || path == NULL || entry == NULL) {
		return -EINVAL;
	}

	SYS_DLIST_FOR_EACH_NODE(&fs_utils_ent_list, node) {
		itr = CONTAINER_OF(node, struct fs_utils_ent, node);
		if (strcmp(itr->ent.name, path) == 0) {
			*entry = itr->ent;
			return 0;
		}
	}

	return -ENOENT;
}

static int fs_utils_mkdir(struct fs_mount_t *mountp, const char *path)
{
	struct fs_utils_ent *e;
	struct fs_dirent ent;

	if (mountp == NULL || path == NULL) {
		return -EINVAL;
	}

	if (strcmp(mountp->mnt_point, path) == 0) {
		return -EPERM;
	}

	if (fs_utils_stat(mountp, path, &ent) == 0) {
		return -EEXIST;
	}

	if (strlen(path) >= sizeof(ent.name)) {
		return -ENAMETOOLONG;
	}

	e = k_malloc(sizeof(*e));
	if (e == NULL) {
		return -ENOMEM;
	}

	e->ent.type = FS_DIR_ENTRY_DIR;
	e->ent.size = 0;
	strcpy(e->ent.name, path);
	sys_dlist_append(&fs_utils_ent_list, &e->node);

	return 0;
}

/**
 * @brief Test fs_mkdir_p() creating nested directories
 */
ZTEST(fs_utils, test_fs_mkdir_p)
{
	struct fs_dirent ent;
	int rc = fs_mkdir_p(TEST_FS_MNTP "/a/b/c");

	TC_PRINT("\nfs_mkdir_p tests:%d\n", rc);
	zassert_equal(rc, 0);
	zassert_equal(fs_stat(TEST_FS_MNTP "/a", &ent), 0, "a does not exist");
	zassert_equal(fs_stat(TEST_FS_MNTP "/a/b", &ent), 0, "a/b does not exist");
	zassert_equal(fs_stat(TEST_FS_MNTP "/a/b/c", &ent), 0, "a/b/c does not exist");
}

/**
 * @brief Test fs_mkdir_p() on a directory that already exists
 */
ZTEST(fs_utils, test_fs_mkdir_p_idempotent)
{
	struct fs_dirent ent;

	zassert_equal(fs_mkdir_p(TEST_FS_MNTP "/a"), 0);
	zassert_equal(fs_mkdir_p(TEST_FS_MNTP "/a"), 0, "second call failed");
	zassert_equal(fs_stat(TEST_FS_MNTP "/a", &ent), 0, "a does not exist");
}

/**
 * @brief Test fs_mkdir_p() argument validation
 */
ZTEST(fs_utils, test_fs_mkdir_p_invalid_args)
{
	zassert_equal(fs_mkdir_p(NULL), -EINVAL);
	zassert_equal(fs_mkdir_p("a/b"), -EINVAL, "path is not absolute");
}

/**
 * @brief Test fs_mkdir_p() on the VFS root, which always exists
 */
ZTEST(fs_utils, test_fs_mkdir_p_root)
{
	zassert_equal(fs_mkdir_p("/"), 0);
}

/**
 * @brief Test fs_mkdir_p() with a path that does not fit in the internal buffer
 */
ZTEST(fs_utils, test_fs_mkdir_p_name_too_long)
{
	char path[CONFIG_FILE_SYSTEM_UTILS_MAX_PATH_LEN + 1];

	memset(path, 'a', sizeof(path) - 1);
	path[0] = '/';
	path[sizeof(path) - 1] = '\0';

	zassert_equal(fs_mkdir_p(path), -ENAMETOOLONG);
}

static void *fs_utils_setup(void)
{
	fs_utils_fs = temp_fs;
	fs_utils_fs.mkdir = fs_utils_mkdir;
	fs_utils_fs.stat = fs_utils_stat;
	fs_register(TEST_FS_1, &fs_utils_fs);
	fs_mount(&test_fs_mnt_1);
	return NULL;
}

static void fs_utils_reset(void *fixture)
{
	sys_dnode_t *node;

	while ((node = sys_dlist_get(&fs_utils_ent_list)) != NULL) {
		k_free(CONTAINER_OF(node, struct fs_utils_ent, node));
	}
}

static void fs_utils_teardown(void *fixture)
{
	fs_utils_reset(fixture);

	fs_unmount(&test_fs_mnt_1);
	fs_unregister(TEST_FS_1, &fs_utils_fs);
}

ZTEST_SUITE(fs_utils, NULL, fs_utils_setup, fs_utils_reset, NULL, fs_utils_teardown);
