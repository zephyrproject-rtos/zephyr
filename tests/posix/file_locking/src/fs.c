/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
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
	FILE *fp;
};

static struct _entry _data[] = {
	{.name = "/tmp/foo.txt", .data = ""},
	{.name = "/tmp/bar.txt", .data = ""},
};

void setup_callback(void *arg);
void before_callback(void *arg);
void after_callback(void *arg);

void *setup(void)
{
	int ret;

	memset(&_fs, 0, sizeof(_fs));

	ret = fs_mount(&_mnt);
	zassert_ok(ret, "mount failed: %d", ret);

	ret = fs_mkdir("/tmp");
	zassert_ok(ret, "mkdir failed: %d", ret);

	setup_callback(NULL);

	return NULL;
}

void before(void *arg)
{
	ARG_UNUSED(arg);

	ARRAY_FOR_EACH_PTR(_data, entry) {
		int len;
		int ret;

		entry->fp = fopen(entry->name, "w+");
		zassert_not_null(entry->fp, "fopen() failed: %d", errno);

		len = strlen(entry->data);
		if (len > 0) {
			ret = (int)fwrite(entry->data, len, 1, entry->fp);
			zassert_equal(ret, len, "%s returned %d instead of %d: %d", "fwrite", ret,
				      len, errno);
		}

		before_callback(entry->fp);
	};
}

void after(void *arg)
{
	ARG_UNUSED(arg);

	ARRAY_FOR_EACH_PTR(_data, entry) {
		(void)fclose(entry->fp);
	};

	after_callback(NULL);
}

void teardown(void *arg)
{
	ARG_UNUSED(arg);
	(void)fs_unmount(&_mnt);
}
