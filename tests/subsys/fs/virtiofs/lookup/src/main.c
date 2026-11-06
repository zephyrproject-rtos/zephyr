/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/fff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/virtiofs.h>
#include <zephyr/ztest.h>

#include "virtiofs_zfs.c"

#define MOUNT_POINT   "/virtiofs"
#define SOURCE_NODE   10U
#define SOURCE_PARENT 11U
#define TARGET_NODE   20U
#define TARGET_PARENT 21U
#define INVALID_NODE  0xdeadbeefU

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, virtiofs_init, const struct device *, struct fuse_init_out *);
FAKE_VALUE_FUNC(int, virtiofs_destroy, const struct device *);
FAKE_VALUE_FUNC(int, virtiofs_lookup, const struct device *, uint64_t, const char *,
		struct fuse_entry_out *, uint64_t *);
FAKE_VALUE_FUNC(int, virtiofs_open, const struct device *, uint64_t, uint32_t,
		struct fuse_open_out *, enum fuse_object_type);
FAKE_VALUE_FUNC(int, virtiofs_create, const struct device *, uint64_t, const char *, uint32_t,
		uint32_t, struct fuse_create_out *);
FAKE_VALUE_FUNC(int, virtiofs_release, const struct device *, uint64_t, uint64_t,
		enum fuse_object_type);
FAKE_VOID_FUNC(virtiofs_forget, const struct device *, uint64_t, uint64_t);
FAKE_VALUE_FUNC(int, virtiofs_rename, const struct device *, uint64_t, const char *, uint64_t,
		const char *);
FAKE_VALUE_FUNC(int, virtiofs_read, const struct device *, uint64_t, uint64_t, uint64_t, uint32_t,
		uint8_t *);
FAKE_VALUE_FUNC(int, virtiofs_write, const struct device *, uint64_t, uint64_t, uint64_t, uint32_t,
		const uint8_t *);
FAKE_VALUE_FUNC(int, virtiofs_lseek, const struct device *, uint64_t, uint64_t, uint64_t, uint32_t,
		struct fuse_lseek_out *);
FAKE_VALUE_FUNC(int, virtiofs_setattr, const struct device *, uint64_t, struct fuse_setattr_in *,
		struct fuse_attr_out *);
FAKE_VALUE_FUNC(int, virtiofs_fsync, const struct device *, uint64_t, uint64_t);
FAKE_VALUE_FUNC(int, virtiofs_mkdir, const struct device *, uint64_t, const char *, uint32_t);
FAKE_VALUE_FUNC(int, virtiofs_unlink, const struct device *, const char *, enum fuse_object_type);
FAKE_VALUE_FUNC(int, virtiofs_statfs, const struct device *, struct fuse_kstatfs *);
FAKE_VALUE_FUNC(int, virtiofs_readdir, const struct device *, uint64_t, uint64_t, uint64_t,
		uint8_t *, uint32_t, uint8_t *, uint32_t);

struct lookup_result {
	int ret;
	uint64_t node;
	uint64_t parent;
};

static struct lookup_result lookup_results[2];

static int lookup(const struct device *dev, uint64_t inode, const char *path,
		  struct fuse_entry_out *response, uint64_t *parent)
{
	const struct lookup_result *result = &lookup_results[virtiofs_lookup_fake.call_count - 1U];

	zassert_true(virtiofs_lookup_fake.call_count <= ARRAY_SIZE(lookup_results));
	*response = (struct fuse_entry_out){.nodeid = result->node};
	if (parent != NULL) {
		*parent = result->parent;
	}
	return result->ret;
}

static int open_object(const struct device *dev, uint64_t inode, uint32_t flags,
		       struct fuse_open_out *response, enum fuse_object_type type)
{
	*response = (struct fuse_open_out){.fh = 1U};
	return virtiofs_open_fake.return_val;
}

static void expect_forgets(const uint64_t *nodes, size_t count)
{
	zassert_equal(virtiofs_forget_fake.call_count, count, "Unexpected FORGET count");
	for (size_t i = 0; i < count; i++) {
		size_t matches = 0;

		for (size_t j = 0; j < count; j++) {
			if (virtiofs_forget_fake.arg1_history[j] == nodes[i]) {
				matches++;
			}
		}
		zassert_equal(matches, 1U, "Expected one FORGET for inode %llu",
			      (unsigned long long)nodes[i]);
		zassert_equal(virtiofs_forget_fake.arg2_history[i], 1U);
	}
}

static void before(void *fixture)
{
	RESET_FAKE(virtiofs_lookup);
	RESET_FAKE(virtiofs_open);
	RESET_FAKE(virtiofs_create);
	RESET_FAKE(virtiofs_release);
	RESET_FAKE(virtiofs_forget);
	RESET_FAKE(virtiofs_rename);
	FFF_RESET_HISTORY();
	lookup_results[0] = (struct lookup_result){0, SOURCE_NODE, SOURCE_PARENT};
	lookup_results[1] = (struct lookup_result){0, TARGET_NODE, TARGET_PARENT};
	virtiofs_lookup_fake.custom_fake = lookup;
	virtiofs_open_fake.custom_fake = open_object;
}

static int init(const struct device *dev, struct fuse_init_out *response)
{
	*response = (struct fuse_init_out){.max_write = 4096U};
	return 0;
}

static void *setup(void)
{
	static struct virtiofs_fs_data fs_data;
	static struct fs_mount_t mount = {
		.type = FS_VIRTIOFS,
		.mnt_point = MOUNT_POINT,
		.fs_data = &fs_data,
	};

	virtiofs_init_fake.custom_fake = init;
	zassert_ok(fs_mount(&mount));
	return &mount;
}

static void teardown(void *fixture)
{
	zassert_ok(fs_unmount(fixture));
}

ZTEST(virtiofs_lookup, test_open_create_failure)
{
	struct fs_file_t file;
	const uint64_t forgotten[] = {SOURCE_PARENT};

	lookup_results[0] = (struct lookup_result){-ENOENT, INVALID_NODE, SOURCE_PARENT};
	virtiofs_create_fake.return_val = -EACCES;
	fs_file_t_init(&file);
	zassert_equal(fs_open(&file, MOUNT_POINT "/parent/file", FS_O_CREATE | FS_O_WRITE),
		      -EACCES);
	expect_forgets(forgotten, ARRAY_SIZE(forgotten));
}

ZTEST(virtiofs_lookup, test_open_lookup_failure)
{
	struct fs_file_t file;
	const uint64_t forgotten[] = {SOURCE_PARENT};

	lookup_results[0] = (struct lookup_result){-ENOENT, INVALID_NODE, SOURCE_PARENT};
	fs_file_t_init(&file);
	zassert_equal(fs_open(&file, MOUNT_POINT "/parent/file", FS_O_READ), -ENOENT);
	zassert_equal(virtiofs_open_fake.call_count, 0U);
	expect_forgets(forgotten, ARRAY_SIZE(forgotten));
}

ZTEST(virtiofs_lookup, test_open_missing_parent)
{
	struct fs_file_t file;

	lookup_results[0] = (struct lookup_result){-ENOENT, INVALID_NODE, 0U};
	fs_file_t_init(&file);
	zassert_equal(fs_open(&file, MOUNT_POINT "/missing/file", FS_O_CREATE | FS_O_WRITE),
		      -ENOENT);
	zassert_equal(virtiofs_create_fake.call_count, 0U);
	expect_forgets(NULL, 0U);
}

ZTEST(virtiofs_lookup, test_open_failure_after_lookup)
{
	struct fs_file_t file;
	const uint64_t forgotten[] = {SOURCE_NODE, SOURCE_PARENT};

	virtiofs_open_fake.return_val = -EACCES;
	fs_file_t_init(&file);
	zassert_equal(fs_open(&file, MOUNT_POINT "/parent/file", FS_O_READ), -EACCES);
	expect_forgets(forgotten, ARRAY_SIZE(forgotten));
}

ZTEST(virtiofs_lookup, test_open_success)
{
	struct fs_file_t file;
	const uint64_t forgotten[] = {SOURCE_PARENT, SOURCE_NODE};

	fs_file_t_init(&file);
	zassert_ok(fs_open(&file, MOUNT_POINT "/parent/file", FS_O_READ));
	expect_forgets(forgotten, 1U);
	zassert_ok(fs_close(&file));
	expect_forgets(forgotten, ARRAY_SIZE(forgotten));
}

ZTEST(virtiofs_lookup, test_opendir_lookup_failure)
{
	struct fs_dir_t dir;

	lookup_results[0] = (struct lookup_result){-ENOENT, INVALID_NODE, 0U};
	fs_dir_t_init(&dir);
	zassert_equal(fs_opendir(&dir, MOUNT_POINT "/missing/dir"), -ENOENT);
	zassert_equal(virtiofs_open_fake.call_count, 0U);
	expect_forgets(NULL, 0U);
}

ZTEST(virtiofs_lookup, test_opendir_lookup_failure_with_parent)
{
	struct fs_dir_t dir;
	const uint64_t forgotten[] = {SOURCE_PARENT};

	lookup_results[0] = (struct lookup_result){-EIO, SOURCE_PARENT, SOURCE_PARENT};
	fs_dir_t_init(&dir);
	zassert_equal(fs_opendir(&dir, MOUNT_POINT "/parent/dir"), -EIO);
	zassert_equal(virtiofs_open_fake.call_count, 0U);
	expect_forgets(forgotten, ARRAY_SIZE(forgotten));
}

ZTEST(virtiofs_lookup, test_opendir_failure_after_lookup)
{
	struct fs_dir_t dir;
	const uint64_t forgotten[] = {SOURCE_PARENT, SOURCE_NODE};

	virtiofs_open_fake.return_val = -EACCES;
	fs_dir_t_init(&dir);
	zassert_equal(fs_opendir(&dir, MOUNT_POINT "/parent/dir"), -EACCES);
	expect_forgets(forgotten, ARRAY_SIZE(forgotten));
}

ZTEST(virtiofs_lookup, test_opendir_success)
{
	struct fs_dir_t dir;
	const uint64_t forgotten[] = {SOURCE_PARENT, SOURCE_NODE};

	fs_dir_t_init(&dir);
	zassert_ok(fs_opendir(&dir, MOUNT_POINT "/parent/dir"));
	expect_forgets(forgotten, 1U);
	zassert_ok(fs_closedir(&dir));
	expect_forgets(forgotten, ARRAY_SIZE(forgotten));
}

ZTEST(virtiofs_lookup, test_rename_missing_target)
{
	const uint64_t forgotten[] = {SOURCE_NODE, SOURCE_PARENT, TARGET_PARENT};

	lookup_results[1] = (struct lookup_result){-ENOENT, INVALID_NODE, TARGET_PARENT};
	zassert_ok(fs_rename(MOUNT_POINT "/old/file", MOUNT_POINT "/new/file"));
	zassert_equal(virtiofs_rename_fake.call_count, 1U);
	expect_forgets(forgotten, ARRAY_SIZE(forgotten));
}

ZTEST(virtiofs_lookup, test_rename_missing_target_parent)
{
	const uint64_t forgotten[] = {SOURCE_NODE, SOURCE_PARENT};

	lookup_results[1] = (struct lookup_result){-ENOENT, INVALID_NODE, 0U};
	zassert_equal(fs_rename(MOUNT_POINT "/old/file", MOUNT_POINT "/missing/file"), -ENOENT);
	zassert_equal(virtiofs_rename_fake.call_count, 0U);
	expect_forgets(forgotten, ARRAY_SIZE(forgotten));
}

ZTEST(virtiofs_lookup, test_rename_existing_target)
{
	const uint64_t forgotten[] = {TARGET_NODE, SOURCE_NODE, SOURCE_PARENT, TARGET_PARENT};

	zassert_ok(fs_rename(MOUNT_POINT "/old/file", MOUNT_POINT "/new/file"));
	expect_forgets(forgotten, ARRAY_SIZE(forgotten));
}

ZTEST_SUITE(virtiofs_lookup, NULL, setup, before, NULL, teardown);
