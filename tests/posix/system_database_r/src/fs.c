/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <ff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

static FATFS _fs;

static struct fs_mount_t _mnt = {
	.type = FS_FATFS,
	.mnt_point = "/",
	.fs_data = &_fs,
};

struct _entry {
	const char *const name;
	const char *const data;
};

static const struct _entry _data[] = {
	{.name = "/etc/passwd",
	 .data = "user:x:1000:1000:user:/home/user:/bin/sh\nroot:x:0:0:root:/root:/bin/sh\n"},
	{.name = "/etc/group", .data = "user:x:1000:staff,admin\nroot:x:0:\n"},
};

void *setup(void)
{
	int ret;

	memset(&_fs, 0, sizeof(_fs));

	ret = fs_mount(&_mnt);
	zassert_ok(ret, "mount failed: %d", ret);

	ret = fs_mkdir("/etc");
	zassert_ok(ret, "mkdir failed: %d", ret);

	ARRAY_FOR_EACH_PTR(_data, entry) {
		int len;
		struct fs_file_t zfp;

		fs_file_t_init(&zfp);

		ret = fs_open(&zfp, entry->name, FS_O_CREATE | FS_O_RDWR | FS_O_TRUNC);
		zassert_true(ret >= 0, "open failed: %d", ret);

		len = strlen(entry->data);
		ret = fs_write(&zfp, entry->data, len);
		zassert_equal(ret, len, "%s returned %d instead of %d", "write", ret, len);

		ret = fs_close(&zfp);
		zassert_ok(ret, "%s returned %d instead of %d", "close", ret, len);
	};

	return &_mnt;
}

void teardown(void *arg)
{
	struct fs_mount_t *const mnt = arg;

	(void)fs_unmount(mnt);
}
