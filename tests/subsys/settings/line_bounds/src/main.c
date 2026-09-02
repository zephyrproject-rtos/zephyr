/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <settings_priv.h>

#define FAKE_RWBS 16U
#define FAKE_SHORT_READ_LEN 4U

static uint8_t fake_store[64];

static int fake_read_cb_short(void *ctx, off_t off, char *buf, size_t *len)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(off);

	*len = MIN(*len, FAKE_SHORT_READ_LEN);
	memset(buf, 0xAA, *len);

	return 0;
}

static int fake_read_cb_full(void *ctx, off_t off, char *buf, size_t *len)
{
	ARG_UNUSED(ctx);

	size_t avail = sizeof(fake_store) - MIN(sizeof(fake_store), (size_t)off);

	*len = MIN(*len, avail);
	memcpy(buf, &fake_store[off], *len);

	return 0;
}

static size_t fake_get_len_cb(void *ctx)
{
	ARG_UNUSED(ctx);

	return 4;
}

static void *line_bounds_setup(void)
{
	for (size_t i = 0; i < sizeof(fake_store); i++) {
		fake_store[i] = (uint8_t)i;
	}

	return NULL;
}

ZTEST(settings_line_bounds, test_val_get_len_returns_zero_past_end)
{
	size_t len;

	settings_line_io_init(fake_read_cb_short, NULL, fake_get_len_cb, FAKE_RWBS);

	len = settings_line_val_get_len(10, NULL);

	zassert_equal(len, 0, "expected 0 for val_off past the record end, got %zu", len);
}

ZTEST(settings_line_bounds, test_raw_read_short_backend_no_overflow)
{
	char out[40];
	size_t len_read = 123;	int rc;

	settings_line_io_init(fake_read_cb_short, NULL, fake_get_len_cb, FAKE_RWBS);

	rc = settings_line_raw_read(10, out, sizeof(out), &len_read, NULL);

	zassert_equal(rc, 0, "unexpected error %d", rc);
	zassert_equal(len_read, 0, "expected no bytes read past the end of backend data, got %zu",
		      len_read);
}

ZTEST(settings_line_bounds, test_raw_read_normal_still_works)
{
	char out[10];
	size_t len_read = 0;
	int rc;

	settings_line_io_init(fake_read_cb_full, NULL, fake_get_len_cb, FAKE_RWBS);

	rc = settings_line_raw_read(5, out, sizeof(out), &len_read, NULL);

	zassert_equal(rc, 0, "unexpected error %d", rc);
	zassert_equal(len_read, sizeof(out), "short read, got %zu", len_read);
	zassert_mem_equal(out, &fake_store[5], sizeof(out));
}

ZTEST_SUITE(settings_line_bounds, NULL, line_bounds_setup, NULL, NULL, NULL);
