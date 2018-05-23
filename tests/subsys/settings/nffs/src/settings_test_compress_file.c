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

int file_str_cmp(const char *fname, char *string);

void test_config_compress_file(void)
{
	int rc;
	struct settings_file cf;

	config_wipe_srcs();

	rc = fs_mkdir(TEST_CONFIG_DIR);
	zassert_true(rc == 0 || rc == -EEXIST, "can't create directory");

	cf.cf_name = TEST_CONFIG_DIR "/korwin";
	cf.cf_maxlines = 24;
	rc = settings_file_src(&cf);
	zassert_true(rc == 0, "can't register FS as configuration source");

	rc = settings_file_dst(&cf);
	zassert_true(rc == 0,
		     "can't register FS as configuration destination");

	val64 = 1125;

	for (int i = 0; i < 22; i++) {
		val8 = i;
		rc = settings_save();
		zassert_true(rc == 0, "fs write error");

		val8 = 0xff;
		settings_load();
		zassert_true(val8 == i, "Bad value loaded");
	}

	val64 = 37;
	rc = settings_save();
	zassert_true(rc == 0, "fs write error");

	/* check 1st compression */
	rc = file_str_cmp(cf.cf_name, "myfoo/mybar64=1125\n" \
				      "myfoo/mybar=21\n" \
				      "myfoo/mybar64=37\n");
	zassert_true(rc == 0, "bad value read");

	for (int i = 0; i < 21; i++) {
		val64 = i;
		rc = settings_save();
		zassert_true(rc == 0, "fs write error");

		val64 = 0xff;
		settings_load();
		zassert_true(val64 == i, "Bad value loaded");
	}

	/* check subsequent compression */
	rc = file_str_cmp(cf.cf_name, "myfoo/mybar=21\n" \
				      "myfoo/mybar64=19\n" \
				      "myfoo/mybar64=20\n");
	zassert_true(rc == 0, "bad value read");
}

int file_str_cmp(const char *fname, char *string)
{
	int rc;
	u32_t len;
	u32_t rlen;
	char *buf;
	struct fs_dirent entry;

	rc = fs_stat(fname, &entry);
	if (rc) {
		return rc;
	}

	if (entry.size != strlen(string)) {
		return -1;
	}

	len = entry.size;
	buf = (char *)k_malloc(len + 1);
	zassert_not_null(buf, "out of memory");

	rc = fsutil_read_file(fname, 0, len, buf, &rlen);
	zassert_true(rc == 0, "can't access the file\n'");
	zassert_true(rc == 0, "not enough data read\n'");
	buf[rlen] = '\0';

	if (strcmp(buf, string)) {
		return -1;
	}

	return -0;
}
