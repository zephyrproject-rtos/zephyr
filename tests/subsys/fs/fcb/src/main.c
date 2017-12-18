/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include "fcb_test.h"
#include "flash_map.h"

struct fcb test_fcb;

/* Sectors for FCB are defined far from application code
 * area. This test suite is the non bootable application so 1. image slot is
 * suitable for it.
 */
struct flash_area test_fcb_area[] = {
	[0] = {
		.fa_device_id = 0,
		.fa_off = FLASH_AREA_IMAGE_1_OFFSET,
		.fa_size = 0x4000, /* 16K */
	},
	[1] = {
		.fa_device_id = 0,
		.fa_off = FLASH_AREA_IMAGE_1_OFFSET + 0x4000,
		.fa_size = 0x4000
	},
	[2] = {
		.fa_device_id = 0,
		.fa_off = FLASH_AREA_IMAGE_1_OFFSET + 0x8000,
		.fa_size = 0x4000
	},
	[3] = {
		.fa_device_id = 0,
		.fa_off = FLASH_AREA_IMAGE_1_OFFSET + 0xc000,
		.fa_size = 0x4000
	}
};

void fcb_test_wipe(void)
{
	int i;
	int rc;
	struct flash_area *fap;

	for (i = 0; i < ARRAY_SIZE(test_fcb_area); i++) {
		fap = &test_fcb_area[i];
		rc = flash_area_erase(fap, 0, fap->fa_size);
		zassert_true(rc == 0, "erase call failure");
	}
}

int fcb_test_empty_walk_cb(struct fcb_entry *loc, void *arg)
{
	zassert_unreachable("fcb_test_empty_walk_cb");
	return 0;
}

u8_t fcb_test_append_data(int msg_len, int off)
{
	return (msg_len ^ off);
}

int fcb_test_data_walk_cb(struct fcb_entry *loc, void *arg)
{
	u16_t len;
	u8_t test_data[128];
	int rc;
	int i;
	int *var_cnt = (int *)arg;

	len = loc->fe_data_len;

	zassert_true(len == *var_cnt, "");

	rc = flash_area_read(loc->fe_area, loc->fe_data_off, test_data, len);
	zassert_true(rc == 0, "read call failure");

	for (i = 0; i < len; i++) {
		zassert_true(test_data[i] == fcb_test_append_data(len, i),
		"fcb_test_append_data redout misrepresentation");
	}
	(*var_cnt)++;
	return 0;
}

int fcb_test_cnt_elems_cb(struct fcb_entry *loc, void *arg)
{
	struct append_arg *aa = (struct append_arg *)arg;
	int idx;

	idx = loc->fe_area - &test_fcb_area[0];
	aa->elem_cnts[idx]++;
	return 0;
}

void fcb_tc_pretest(int sectors)
{
	struct fcb *fcb;
	int rc = 0;

	fcb_test_wipe();
	fcb = &test_fcb;
	memset(fcb, 0, sizeof(*fcb));
	fcb->f_sector_cnt = sectors;
	fcb->f_sectors = test_fcb_area; /* XXX */

	rc = 0;
	rc = fcb_init(fcb);
	if (rc != 0) {
		printf("%s rc == %x, %d\n", __func__, rc, rc);
		zassert_true(rc == 0, "fbc initialization failure");
	}
}

void fcb_pretest_2_sectors(void)
{
	fcb_tc_pretest(2);
}

void fcb_pretest_4_sectors(void)
{
	fcb_tc_pretest(4);
}

void teardown_nothing(void)
{
}

void fcb_test_len(void);
void fcb_test_init(void);
void fcb_test_empty_walk(void);
void fcb_test_append(void);
void fcb_test_append_too_big(void);
void fcb_test_append_fill(void);
void fcb_test_reset(void);
void fcb_test_rotate(void);
void fcb_test_multi_scratch(void);
void fcb_test_last_of_n(void);

void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_fcb,
			 ztest_unit_test_setup_teardown(fcb_test_len,
							fcb_pretest_2_sectors,
							teardown_nothing),
			 ztest_unit_test(fcb_test_init),
			 ztest_unit_test_setup_teardown(fcb_test_empty_walk,
							fcb_pretest_2_sectors,
							teardown_nothing),
			 ztest_unit_test_setup_teardown(fcb_test_append,
							fcb_pretest_2_sectors,
							teardown_nothing),
			 ztest_unit_test_setup_teardown(fcb_test_append_too_big,
							fcb_pretest_2_sectors,
							teardown_nothing),
			 ztest_unit_test_setup_teardown(fcb_test_append_fill,
							fcb_pretest_2_sectors,
							teardown_nothing),
			 ztest_unit_test_setup_teardown(fcb_test_reset,
							fcb_pretest_2_sectors,
							teardown_nothing),
			 ztest_unit_test_setup_teardown(fcb_test_rotate,
							fcb_pretest_2_sectors,
							teardown_nothing),
			 ztest_unit_test_setup_teardown(fcb_test_multi_scratch,
							fcb_pretest_4_sectors,
							teardown_nothing),
			 ztest_unit_test_setup_teardown(fcb_test_last_of_n,
							fcb_pretest_4_sectors,
							teardown_nothing)
			 );

	ztest_run_test_suite(test_fcb);
}
