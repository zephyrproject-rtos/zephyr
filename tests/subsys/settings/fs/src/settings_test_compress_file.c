/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include "settings_test.h"
#include "settings/settings_file.h"

#define EXP_STR_CONTENT_1 "\x10\x00myfoo/mybar16=\x00\x01"\
			  "\x0d\x00myfoo/mybar=\x14"\
			  "\x16\x00myfoo/mybar64=\x25\x00\x00\x00\x00\x00\x00\x00"

#define EXP_STR_CONTENT_2 "\x0d\x00myfoo/mybar=\x14"\
			  "\x10\x00myfoo/mybar16=\x01\x01"\
			  "\x16\x00myfoo/mybar64=\x13\x00\x00\x00\x00\x00\x00\x00"

int file_str_cmp(const char *fname, char const *string, size_t pattern_len);

void test_config_compress_file(void)
{
	int rc;
	struct settings_file cf;

	config_wipe_srcs();

	rc = fs_mkdir(TEST_CONFIG_DIR);
	zassert_true(rc == 0 || rc == -EEXIST, "can't create directory");

	cf.cf_name = TEST_CONFIG_DIR "/korwin";
	cf.cf_maxlines = 24;
	cf.cf_lines = 0; /* required as not start with load settings */

	rc = settings_file_src(&cf);
	zassert_true(rc == 0, "can't register FS as configuration source");

	rc = settings_file_dst(&cf);
	zassert_true(rc == 0,
		     "can't register FS as configuration destination");

	val64 = 1125U;
	val16 = 256U;

	for (int i = 0; i < 21; i++) {
		val8 = i;
		rc = settings_save();
		zassert_true(rc == 0, "fs write error");

		val8 = 0xff;
		settings_load();
		zassert_true(val8 == i, "Bad value loaded");
	}

	val64 = 37U;
	rc = settings_save();
	zassert_true(rc == 0, "fs write error");

	const char exp_content_1[] = EXP_STR_CONTENT_1;

	/* check 1st compression */
	rc = file_str_cmp(cf.cf_name, exp_content_1, sizeof(exp_content_1)-1);
	zassert_true(rc == 0, "bad value read");

	val16 = 257U;
	for (int i = 0; i < 20; i++) {
		val64 = i;
		rc = settings_save();
		zassert_true(rc == 0, "fs write error");

		val64 = 0xff;
		settings_load();
		zassert_true(val64 == i, "Bad value loaded");
	}

	const char exp_content_2[] = EXP_STR_CONTENT_2;

	/* check subsequent compression */
	rc = file_str_cmp(cf.cf_name, exp_content_2, sizeof(exp_content_2)-1);
	zassert_true(rc == 0, "bad value read");
}

int file_str_cmp(const char *fname, char const *string, size_t pattern_len)
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

	if (entry.size != pattern_len) {
		return -1;
	}

	len = entry.size;
	buf = (char *)k_malloc(len + 1);
	zassert_not_null(buf, "out of memory");

	rc = fsutil_read_file(fname, 0, len, buf, &rlen);
	zassert_true(rc == 0, "can't access the file\n'");
	zassert_true(rc == 0, "not enough data read\n'");
	buf[rlen] = '\0';

	if (memcmp(buf, string, len)) {
		return -1;
	}

	return -0;
}
