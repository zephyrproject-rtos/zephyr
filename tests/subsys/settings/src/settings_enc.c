/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"
#include "settings_priv.h"

static char enc_buf[128];
static int enc_buf_cnt;
static u8_t test_rwbs = 1U;
#define ENC_CTX_VAL 0x2018

static int write_handler(void *ctx, off_t off, char const *buf, size_t len)
{
	zassert_equal(ctx, (void *)ENC_CTX_VAL, "bad write callback context\n");

	if (off % test_rwbs || len % test_rwbs) {
		return -EIO;
	}

	memcpy(&enc_buf[off], buf, len);
	enc_buf_cnt += len;
	return 0;
}

static void test_encoding_iteration(char const *name, char const *value,
				    char const *pattern, int exp_len, u8_t wbs)
{
	int rc;

	test_rwbs = wbs;

	settings_line_io_init(NULL, write_handler, NULL, wbs);

	enc_buf_cnt = 0;

	rc = settings_line_write(name, value, strlen(value), 0,
				 (void *)ENC_CTX_VAL);

	zassert_equal(rc, 0, "Can't encode the line %d.\n", rc);

	zassert_equal(enc_buf_cnt, exp_len, "Wrote more than expected\n");

	zassert_true(memcmp(pattern, enc_buf, exp_len) == 0,
		     "encoding defect, was     : %s\nexpected: %s\n", enc_buf,
		     pattern);
}

void test_settings_encode(void)
{
	char const name[] = "nordic";
	char const value[] = "Doubt. Only an evil man, master Geralt,"
			     " is without it. And no one escapes his destiny";
	char const pattern[] = "nordic=RG91YnQuIE9ubHkgYW4gZXZpbCBtYW4sIG1hc3Rl"
			       "ciBHZXJhbHQsIGlzIHdpdGhvdXQgaXQuIEFuZCBubyBvbmU"
			       "gZXNjYXBlcyBoaXMgZGVzdGlueQ==\0";
	char const pattern2[] = "nordic=RG91YnQuIE9ubHkgYW4gZXZpbCBtYW4sIG1hc3R"
				"lciBHZXJhbHQsIGlzIHdpdGhvdXQgaXQuIEFuZCBubyBvb"
				"mUgZXNjYXBlcyBoaXMgZGVzdGlueQ==\0\0\0\0\0";
	char const name2[] = "nord";
	char const value2[] = "123";
	char const pattern3[] = "nord=MTIz\0\0\0";

	test_encoding_iteration(name, value, pattern, 124, 4);
	test_encoding_iteration(name, value, pattern, 123, 1);
	test_encoding_iteration(name, value, pattern2, 128, 8);
	test_encoding_iteration(name2, value2, pattern3, 12, 4);
	test_encoding_iteration(name2, value2, pattern3, 9, 1);
}

static int read_handle(void *ctx, off_t off, char *buf, size_t *len)
{
	int r_len;

	zassert_equal(ctx, (void *)ENC_CTX_VAL, "bad write callback context\n");

	if (off % test_rwbs || *len % test_rwbs) {
		return -EIO;
	}

	r_len = MIN(sizeof(enc_buf) - off, *len);
	if (r_len <= 0) {
		*len = 0;
		return 0;
	}

	memcpy(buf, &enc_buf[off], r_len);
	*len = r_len;
	return 0;
}

char read_buf[128+10];

static void test_raw_read_iteration(u8_t rbs, size_t off, size_t len)
{
	test_rwbs = rbs;
	size_t len_read;
	int rc;
	size_t expected;

	memset(read_buf, 0x00, sizeof(read_buf));

	settings_line_io_init(read_handle, write_handler, NULL, rbs);

	rc = settings_line_raw_read(off, &read_buf[4], len, &len_read,
				    (void *)ENC_CTX_VAL);

	zassert_equal(rc, 0, "Can't read the line %d.\n", rc);

	expected = MIN((sizeof(enc_buf) - off), len);
	zassert_equal(expected, len_read, "Unexpected read size\n");

	zassert_true(memcmp(&enc_buf[off], &read_buf[4], len_read) == 0,
			    "read defect\n");

	for (rc = 0; rc < 4; rc++) {
		zassert_equal(read_buf[rc], 0, "buffer lickage\n");
	}

	for (rc = len_read + 4; rc < sizeof(read_buf); rc++) {
		zassert_equal(read_buf[rc], 0, "buffer lickage\n");
	}
}

void test_setting_raw_read(void)
{
	int i;

	for (i = 0; i < sizeof(enc_buf); i++) {
		enc_buf[i] = i + 1;
	}

	test_raw_read_iteration(1, 0, 56);
	test_raw_read_iteration(1, 5, 128);
	test_raw_read_iteration(4, 1, 56);
	test_raw_read_iteration(4, 0, 128);
	test_raw_read_iteration(4, 3, 128);
	test_raw_read_iteration(8, 3, 128);
	test_raw_read_iteration(8, 0, 128);
	test_raw_read_iteration(8, 77, 3);
}




static void test_val_read_iteration(char const *src, size_t src_len,
				    char const *pattern, size_t pattern_len,
				    size_t len, u8_t rbs, size_t off,
				    size_t val_off)
{
	size_t len_read;
	int rc;

	memcpy(enc_buf, src, src_len);

	test_rwbs = rbs;

	settings_line_io_init(read_handle, write_handler, NULL, rbs);

	rc =  settings_line_val_read(val_off, off, read_buf, len, &len_read,
				     (void *)ENC_CTX_VAL);

	zassert_equal(rc, 0, "Can't read the value.\n");

	zassert_equal(len_read, pattern_len, "Bad length (was %d).\n",
		      len_read);
	read_buf[len_read] = 0;
	zassert_true(memcmp(pattern, read_buf, pattern_len) == 0,
		     "encoding defect, was :\n%s\nexpected :\n%s\n", read_buf,
		     pattern);
}

void test_setting_val_read(void)
{
	char const val_src[] = "V2FzIHdyaXR0ZW4gaW4gS3Jha293AA==";
	char const val_src2[] = "jozef/pilsodski="
				"V2FzIHdyaXR0ZW4gaW4gS3Jha293AA==";
	char const val_pattern[] = "Was written in Krakow";

	test_val_read_iteration(val_src, sizeof(val_src), val_pattern,
				sizeof(val_pattern), 128, 1, 0, 0);
	test_val_read_iteration(val_src, sizeof(val_src), &val_pattern[1],
				sizeof(val_pattern) - 1, 128, 1, 1, 0);
	test_val_read_iteration(val_src, sizeof(val_src), val_pattern,
				sizeof(val_pattern), sizeof(val_pattern), 1, 0,
				0);

	for (int j = 1; j < sizeof(val_pattern); j++) {
		for (int i = 0; i < sizeof(val_pattern) - j; i++) {
			test_val_read_iteration(val_src, sizeof(val_src),
						&val_pattern[i], j, j, 1, i, 0);
		}
	}

	for (int j = 1; j < sizeof(val_pattern); j++) {
		for (int i = 0; i < sizeof(val_pattern) - j; i++) {
			test_val_read_iteration(val_src2, sizeof(val_src2),
						&val_pattern[i], j, j, 1, i,
						16);
		}
	}

	test_val_read_iteration(val_src, sizeof(val_src), val_pattern,
				sizeof(val_pattern), 128, 4, 0, 0);
	test_val_read_iteration(val_src, sizeof(val_src), &val_pattern[1],
				sizeof(val_pattern) - 1, 128, 4, 1, 0);
	test_val_read_iteration(val_src, sizeof(val_src), val_pattern,
				sizeof(val_pattern), sizeof(val_pattern), 4, 0,
				0);

	for (int j = 1; j < sizeof(val_pattern); j++) {
		for (int i = 0; i < sizeof(val_pattern) - j; i++) {
			test_val_read_iteration(val_src, sizeof(val_src),
						&val_pattern[i], j, j, 4, i, 0);
		}
	}

	test_val_read_iteration(val_src, sizeof(val_src), val_pattern,
				sizeof(val_pattern), 128, 8, 0, 0);
	test_val_read_iteration(val_src, sizeof(val_src), &val_pattern[1],
				sizeof(val_pattern) - 1, 128, 8, 1, 0);
	test_val_read_iteration(val_src, sizeof(val_src), val_pattern,
				sizeof(val_pattern), sizeof(val_pattern), 8, 0,
				0);

	for (int j = 1; j < sizeof(val_pattern); j++) {
		for (int i = 0; i < sizeof(val_pattern) - j; i++) {
			test_val_read_iteration(val_src, sizeof(val_src),
						&val_pattern[i], j, j, 8, i, 0);
		}
	}


}
