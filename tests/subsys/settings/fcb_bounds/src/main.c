/*
 * Copyright (c) 2026 Canonical Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Regression test for bounds checking in settings_line_raw_read_until().
 *
 * Exercises the arithmetic at line 201 of settings_line.c with a past-EOF
 * seek at a nonzero intra-block offset. Without the fix, len = read_size - off
 * underflows and memcpy reads past the 32-byte stack buffer. With the fix,
 * the function breaks early and returns len_read < len_req.
 */

#include <zephyr/ztest.h>
#include <zephyr/settings/settings.h>
#include <zephyr/fs/fcb.h>
#include <string.h>

#include "settings_priv.h"
#include "settings/settings_fcb.h"

/* Minimal handler — accepts any write, never reads */
static int dummy_set(const char *name, size_t len, settings_read_cb read_cb,
		     void *cb_arg)
{
	ARG_UNUSED(name);
	ARG_UNUSED(len);
	ARG_UNUSED(read_cb);
	ARG_UNUSED(cb_arg);
	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(test_bounds, "tb", NULL, dummy_set, NULL, NULL);

/*
 * Test: settings_line_raw_read with past-EOF seek at nonzero intra-block offset.
 *
 * Setup: write a real setting "tb/x" (short name) so that val_off is within
 * the first block. Then corrupt fe_data_len to be shorter than val_off,
 * causing the read_handler to return *len=0 while intra-block offset > 0.
 *
 * Expected (with fix): function breaks early, len_read < len_req, output
 * buffer contains 0xBB sentinel (untouched).
 *
 * Would fail (without fix): len_read == len_req, output contains stack garbage.
 */
ZTEST(settings_bounds, test_raw_read_past_eof_nonzero_offset)
{
	struct fcb *fcb_ptr;
	struct fcb_entry_ctx entry_ctx;
	uint8_t out[64];
	size_t len_read;
	int rc;
	int32_t val = 99;

	/* Initialize FCB backend */
	rc = settings_subsys_init();
	zassert_equal(rc, 0, "settings_subsys_init failed: %d", rc);

	rc = settings_storage_get((void **)&fcb_ptr);
	zassert_equal(rc, 0, "settings_storage_get failed: %d", rc);
	zassert_not_null(fcb_ptr, "fcb_ptr is NULL");
	zassert_true(fcb_ptr->f_align >= 2,
		     "f_align=%u, need >=2 for this test", fcb_ptr->f_align);

	/* Write a real setting to flash */
	rc = settings_save_one("tb/x", &val, sizeof(val));
	zassert_equal(rc, 0, "settings_save_one failed: %d", rc);

	/* Retrieve the FCB entry (CRC-validated) */
	entry_ctx.loc.fe_sector = NULL;
	entry_ctx.loc.fe_elem_off = 0;
	entry_ctx.fap = fcb_ptr->fap;

	rc = fcb_getnext(fcb_ptr, &entry_ctx.loc);
	zassert_equal(rc, 0, "fcb_getnext failed: %d", rc);

	/*
	 * Entry layout: "tb/x=<4 bytes>", so:
	 *   name = "tb/x" (4 chars), separator '=' (1 char)
	 *   val_off = 5, value = 4 bytes, total fe_data_len = 9
	 */
	size_t val_off = 5; /* strlen("tb/x") + 1 for '=' */

	zassert_true(entry_ctx.loc.fe_data_len >= val_off + sizeof(val),
		     "unexpected fe_data_len=%u", entry_ctx.loc.fe_data_len);

	/*
	 * Corrupt fe_data_len to be shorter than val_off.
	 * This simulates the condition that would cause read_handler to
	 * return *len=0 while seek lands at a nonzero intra-block offset.
	 */
	entry_ctx.loc.fe_data_len = 2; /* < val_off=5 */

	/* Fill output with sentinel */
	memset(out, 0xBB, sizeof(out));

	/*
	 * Call with seek=val_off (5). With f_align=8:
	 *   off_aligned = (5/8)*8 = 0
	 *   read_handler: tries to read 32 bytes at offset 0, but
	 *     fe_data_len=2, so returns read_size=2
	 *   intra-block off = 5 - 0 = 5
	 *   Without fix: len = 2 - 5 = underflow → over-read
	 *   With fix: read_size(2) < off(5) → break
	 */
	rc = settings_line_raw_read((off_t)val_off, (char *)out, 30,
				    &len_read, &entry_ctx);

	/*
	 * With the fix: function should break early.
	 * len_read should be 0 (no data copied), rc should be 0.
	 * Output buffer should still contain sentinel bytes.
	 */
	zassert_equal(rc, 0, "expected rc=0, got %d", rc);
	zassert_true(len_read < 30,
		     "len_read=%zu — expected < 30 (over-read occurred!)",
		     len_read);
	zassert_equal(len_read, 0,
		     "len_read=%zu — expected 0 (no valid data past EOF)",
		     len_read);

	/* Verify sentinel is intact (no over-read into output) */
	for (size_t i = 0; i < sizeof(out); i++) {
		zassert_equal(out[i], 0xBB,
			      "out[%zu]=0x%02x, expected 0xBB (buffer corrupted)",
			      i, out[i]);
	}
}

/*
 * Test: settings_line_val_get_len with val_off > fe_data_len.
 *
 * Expected (with fix): returns 0.
 * Would fail (without fix): returns ~SIZE_MAX.
 */
ZTEST(settings_bounds, test_val_get_len_underflow)
{
	struct fcb *fcb_ptr;
	struct fcb_entry_ctx entry_ctx;
	int rc;
	int32_t val = 77;

	rc = settings_subsys_init();
	zassert_equal(rc, 0, "settings_subsys_init failed: %d", rc);

	rc = settings_storage_get((void **)&fcb_ptr);
	zassert_equal(rc, 0, "settings_storage_get failed: %d", rc);

	/* Write a setting so we have a valid entry */
	rc = settings_save_one("tb/y", &val, sizeof(val));
	zassert_equal(rc, 0, "settings_save_one failed: %d", rc);

	/* Get entry */
	entry_ctx.loc.fe_sector = NULL;
	entry_ctx.loc.fe_elem_off = 0;
	entry_ctx.fap = fcb_ptr->fap;
	/* Walk to the latest entry */
	while (fcb_getnext(fcb_ptr, &entry_ctx.loc) == 0) {
		/* keep walking */
	}
	/* Back up: use the last valid entry from the first getnext */
	entry_ctx.loc.fe_sector = NULL;
	entry_ctx.loc.fe_elem_off = 0;
	rc = fcb_getnext(fcb_ptr, &entry_ctx.loc);
	zassert_equal(rc, 0, "fcb_getnext failed: %d", rc);

	/* Corrupt fe_data_len to be tiny */
	entry_ctx.loc.fe_data_len = 2;

	/*
	 * Call val_get_len with val_off=5 > fe_data_len=2.
	 * Without fix: returns (2 - 5) as size_t = ~SIZE_MAX.
	 * With fix: returns 0.
	 */
	size_t result = settings_line_val_get_len(5, &entry_ctx);

	zassert_equal(result, 0,
		      "expected 0 for val_off > len, got %zu", result);
}

ZTEST_SUITE(settings_bounds, NULL, NULL, NULL, NULL, NULL);
