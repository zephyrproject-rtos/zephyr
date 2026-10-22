/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/ztest.h>

#include "settings_priv.h"

static int write_calls;
static int name_writes;
static int name_write_rc;

static int dummy_read(void *ctx, off_t off, char *buf, size_t *len)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(off);
	ARG_UNUSED(buf);

	*len = 0;
	return 0;
}

static size_t dummy_get_len(void *ctx)
{
	ARG_UNUSED(ctx);

	return 0;
}

static int counting_write(void *ctx, off_t off, char const *buf, size_t len)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(off);
	ARG_UNUSED(buf);

	write_calls++;
	if (len == strlen("myfoo/mybar")) {
		name_writes++;
		return name_write_rc;
	}

	return 0;
}

ZTEST(settings_line, test_name_write_error_is_reported)
{
	const char val = 0x14;
	int rc;

	settings_line_io_init(dummy_read, counting_write, dummy_get_len, 1);

	write_calls = 0;
	name_writes = 0;
	name_write_rc = -EIO;

	rc = settings_line_write("myfoo/mybar", &val, sizeof(val), 0, NULL);
	zassert_equal(name_writes, 1, "name record was not written");
	zassert_equal(rc, -EIO,
		      "name write failure should fail the save, got %d after %d writes",
		      rc, write_calls);
}

ZTEST(settings_line, test_name_write_success)
{
	const char val = 0x14;
	int rc;

	settings_line_io_init(dummy_read, counting_write, dummy_get_len, 1);

	write_calls = 0;
	name_writes = 0;
	name_write_rc = 0;

	rc = settings_line_write("myfoo/mybar", &val, sizeof(val), 0, NULL);
	zassert_equal(name_writes, 1, "name record was not written");
	zassert_ok(rc, "successful name write should return 0, got %d", rc);
}

ZTEST_SUITE(settings_line, NULL, NULL, NULL, NULL, NULL);
