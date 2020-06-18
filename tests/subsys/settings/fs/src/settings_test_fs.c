/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <string.h>

#include "settings_test.h"
#include "settings_priv.h"


uint8_t val8;
uint16_t val16;
uint64_t val64;

int test_get_called;
int test_set_called;
int test_commit_called;
int test_export_block;

int c2_var_count = 1;

int c1_handle_get(const char *name, char *val, int val_len_max);
int c1_handle_set(const char *name, size_t len, settings_read_cb read_cb,
		  void *cb_arg);
int c1_handle_commit(void);
int c1_handle_export(int (*cb)(const char *name,
			       const void *value, size_t val_len));

struct settings_handler c_test_handlers[] = {
	{
		.name = "myfoo",
		.h_get = c1_handle_get,
		.h_set = c1_handle_set,
		.h_commit = c1_handle_commit,
		.h_export = c1_handle_export
	},
};



int c1_handle_get(const char *name, char *val, int val_len_max)
{
	test_get_called = 1;
	const char *next;

	if (settings_name_steq(name, "mybar", &next) && !next) {
		val_len_max = MIN(val_len_max, sizeof(val8));
		memcpy(val, &val8, MIN(val_len_max, sizeof(val8)));
		return val_len_max;
	}

	if (settings_name_steq(name, "mybar16", &next) && !next) {
		val_len_max = MIN(val_len_max, sizeof(val16));
		memcpy(val, &val16, MIN(val_len_max, sizeof(val16)));
		return val_len_max;
	}

	if (settings_name_steq(name, "mybar64", &next) && !next) {
		val_len_max = MIN(val_len_max, sizeof(val64));
		memcpy(val, &val64, MIN(val_len_max, sizeof(val64)));
		return val_len_max;
	}

	return -ENOENT;
}

int c1_handle_set(const char *name, size_t len, settings_read_cb read_cb,
		  void *cb_arg)
{
	int rc;
	const char *next;
	size_t val_len;

	test_set_called = 1;
	if (settings_name_steq(name, "mybar", &next) && !next) {
		val_len = len;
		zassert_true(val_len == 1, "bad set-value size");

		rc = read_cb(cb_arg, &val8, sizeof(val8));
		zassert_true(rc >= 0, "SETTINGS_VALUE_SET callback");
		return 0;
	}

	if (settings_name_steq(name, "mybar16", &next) && !next) {
		val_len = len;
		zassert_true(val_len == 2, "bad set-value size");

		rc = read_cb(cb_arg, &val16, sizeof(val16));
		zassert_true(rc >= 0, "SETTINGS_VALUE_SET callback");
		return 0;
	}

	if (settings_name_steq(name, "mybar64", &next) && !next) {
		val_len = len;
		zassert_true(val_len == 8, "bad set-value size");

		rc = read_cb(cb_arg, &val64, sizeof(val64));
		zassert_true(rc >= 0, "SETTINGS_VALUE_SET callback");
		return 0;
	}

	return -ENOENT;
}

int c1_handle_commit(void)
{
	test_commit_called = 1;
	return 0;
}

int c1_handle_export(int (*cb)(const char *name,
			       const void *value, size_t val_len))
{
	if (test_export_block) {
		return 0;
	}

	(void)cb("myfoo/mybar", &val8, sizeof(val8));

	(void)cb("myfoo/mybar16", &val16, sizeof(val16));

	(void)cb("myfoo/mybar64", &val64, sizeof(val64));

	return 0;
}

void ctest_clear_call_state(void)
{
	test_get_called = 0;
	test_set_called = 0;
	test_commit_called = 0;
}

int ctest_get_call_state(void)
{
	return test_get_called + test_set_called + test_commit_called;
}

void config_wipe_srcs(void)
{
	sys_slist_init(&settings_load_srcs);
	settings_save_dst = NULL;
}

int fsutil_read_file(const char *path, off_t offset, size_t len, void *dst,
		     size_t *out_len)
{
	struct fs_file_t file;
	int rc;
	ssize_t r_len = 0;

	rc = fs_open(&file, path, FS_O_CREATE | FS_O_RDWR);
	if (rc != 0) {
		return rc;
	}

	r_len = fs_read(&file, dst, len);
	if (r_len < 0) {
		rc = -EIO;
	} else {
		*out_len = r_len;
	}

	fs_close(&file);
	return rc;
}

int fsutil_write_file(const char *path, const void *data, size_t len)
{
	struct fs_file_t file;
	int rc;

	rc = fs_open(&file, path, FS_O_CREATE | FS_O_RDWR);
	if (rc != 0) {
		return rc;
	}

	if (fs_write(&file, data, len) != len) {
		rc = -EIO;
	}

	fs_close(&file);
	return rc;
}

char const *memmem(char const *mem, size_t mem_len, char const *sub,
		   size_t sub_len)
{
	int i;

	if (sub_len <= mem_len && sub_len > 0) {
		for (i = 0; i <= mem_len - sub_len; i++) {
			if (!memcmp(&mem[i], sub, sub_len)) {
				return &mem[i];
			}
		}
	}

	return NULL;
}

int settings_test_file_strstr(const char *fname, char const *string,
			      size_t str_len)
{
	int rc;
	uint32_t len;
	size_t rlen;
	char *buf;
	struct fs_dirent entry;

	rc = fs_stat(fname, &entry);
	if (rc) {
		return rc;
	}

	len = entry.size;
	buf = (char *)k_malloc(len + 1);
	zassert_not_null(buf, "out of memory");

	rc = fsutil_read_file(fname, 0, len, buf, &rlen);
	zassert_true(rc == 0, "can't access the file\n'");
	zassert_true(rc == 0, "not enough data read\n'");
	buf[rlen] = '\0';

	if (memmem(buf, len, string, str_len)) {
		return 0;
	}

	return -1;
}
