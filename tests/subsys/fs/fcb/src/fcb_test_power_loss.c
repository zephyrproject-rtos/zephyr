/*
 * Copyright (c) 2026 Harsimranjeet Singh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/stats/stats.h>

#include "fcb_test.h"

/* Only the flash simulator can cut power, real flash boards skip these tests */
#ifdef CONFIG_FLASH_SIMULATOR_STATS

/*
 * These tests cut power in the middle of a flash write using the flash
 * simulator: the write numbered max_write_calls keeps only its first
 * max_len bytes, and every write after it is dropped.
 */

static uint32_t *write_calls;
static uint32_t *max_write_calls;
static uint32_t *max_len;

static int find_sim_stat(struct stats_hdr *hdr, void *arg, const char *name, uint16_t off)
{
	uint32_t *stat = (uint32_t *)((uint8_t *)hdr + off);

	if (strcmp(name, "flash_write_calls") == 0) {
		write_calls = stat;
	} else if (strcmp(name, "max_write_calls") == 0) {
		max_write_calls = stat;
	} else if (strcmp(name, "max_len") == 0) {
		max_len = stat;
	}

	return 0;
}

/* Cut power during the next flash write, keeping only its first bytes. */
static void cut_power_on_next_write(uint32_t bytes)
{
	*max_len = bytes;
	*max_write_calls = *write_calls + 1;
}

static void restore_power(void)
{
	*max_write_calls = 0;
	*max_len = 0;
}

/* Throw away the FCB state in RAM, like a reboot, and init it again. */
static int reboot_fcb(struct fcb *fcb)
{
	memset(fcb, 0, sizeof(*fcb));
	fcb->f_sector_cnt = 2;
	fcb->f_sectors = test_fcb_sector;

	return fcb_init(TEST_FCB_FLASH_AREA_ID, fcb);
}

/* Write one complete entry using the same data pattern as the other tests. */
static void append_entry(struct fcb *fcb, uint16_t len)
{
	struct fcb_entry loc;
	uint8_t data[64];
	int rc;

	zassert_true(len <= sizeof(data));

	rc = fcb_append(fcb, len, &loc);
	zassert_equal(rc, 0, "fcb_append failed: %d", rc);

	for (int i = 0; i < len; i++) {
		data[i] = fcb_test_append_data(len, i);
	}

	rc = flash_area_write(fcb->fap, FCB_ENTRY_FA_DATA_OFF(loc), data, len);
	zassert_equal(rc, 0, "flash_area_write failed: %d", rc);

	rc = fcb_append_finish(fcb, &loc);
	zassert_equal(rc, 0, "fcb_append_finish failed: %d", rc);
}

/* #121748: power cut while writing the 2 byte length of an entry */
ZTEST(fcb_test_power_loss, test_fcb_torn_len)
{
	struct fcb *fcb = &test_fcb;
	struct fcb_entry loc;
	int cnt;
	int rc;

	append_entry(fcb, 32);
	append_entry(fcb, 33);

	/* 200 bytes needs a 2 byte length, only the first byte gets written */
	cut_power_on_next_write(1);
	rc = fcb_append(fcb, 200, &loc);
	zassert_equal(rc, 0, "fcb_append failed: %d", rc);
	restore_power();

	rc = reboot_fcb(fcb);
	zassert_equal(rc, 0, "fcb_init after torn length failed: %d", rc);

	/* the two entries written before the power cut are still there */
	cnt = 32;
	rc = fcb_walk(fcb, NULL, fcb_test_data_walk_cb, &cnt);
	zassert_equal(rc, 0, "fcb_walk failed: %d", rc);
	zassert_equal(cnt, 34, "entries written before the power cut are missing");

	/* and we can still add new ones */
	append_entry(fcb, 34);

	cnt = 32;
	rc = fcb_walk(fcb, NULL, fcb_test_data_walk_cb, &cnt);
	zassert_equal(rc, 0, "fcb_walk failed: %d", rc);
	zassert_equal(cnt, 35, "new entry missing after recovery");
}

/* A cut can also leave the second length byte half written. If the length
 * that comes out still looks valid but runs past the end of the sector,
 * it must be treated as the end of the log too.
 */
ZTEST(fcb_test_power_loss, test_fcb_len_past_sector_end)
{
	struct fcb *fcb = &test_fcb;
	/* a 246 byte entry has length f6 01. If only bit 7 of the second byte
	 * gets programmed, it reads f6 7f: 118 + 127 * 128 = 16374 bytes, under
	 * FCB_MAX_LEN but past the end of the 16K test sector
	 */
	const uint8_t bad_len[2] = {0xf6, 0x7f};
	int cnt;
	int rc;

	append_entry(fcb, 32);
	append_entry(fcb, 33);

	rc = flash_area_write(fcb->fap,
			      fcb->f_active.fe_sector->fs_off + fcb->f_active.fe_elem_off,
			      bad_len, sizeof(bad_len));
	zassert_equal(rc, 0, "flash_area_write failed: %d", rc);

	rc = reboot_fcb(fcb);
	zassert_equal(rc, 0, "fcb_init after bad length failed: %d", rc);

	cnt = 32;
	rc = fcb_walk(fcb, NULL, fcb_test_data_walk_cb, &cnt);
	zassert_equal(rc, 0, "fcb_walk failed: %d", rc);
	zassert_equal(cnt, 34, "entries written before the power cut are missing");

	append_entry(fcb, 34);

	cnt = 32;
	rc = fcb_walk(fcb, NULL, fcb_test_data_walk_cb, &cnt);
	zassert_equal(rc, 0, "fcb_walk failed: %d", rc);
	zassert_equal(cnt, 35, "new entry missing after recovery");
}

static void power_loss_before(void *data)
{
	stats_walk(stats_group_find("flash_sim_stats"), find_sim_stat, NULL);
	stats_walk(stats_group_find("flash_sim_thresholds"), find_sim_stat, NULL);
	zassert_not_null(write_calls);
	zassert_not_null(max_write_calls);
	zassert_not_null(max_len);

	restore_power();
	fcb_tc_pretest(2, &test_fcb);
}

/* restore power even if a test failed half way, so other tests aren't affected */
static void power_loss_after(void *data)
{
	if (max_write_calls != NULL) {
		restore_power();
	}
}

ZTEST_SUITE(fcb_test_power_loss, NULL, NULL, power_loss_before, power_loss_after, NULL);

#endif /* CONFIG_FLASH_SIMULATOR_STATS */
