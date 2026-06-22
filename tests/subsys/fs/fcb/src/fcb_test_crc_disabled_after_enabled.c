/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fcb_test.h"

static void fcb_pretest_crc_disabled_after_enabled(void)
{
	int rc;
	struct fcb_entry loc;
	uint8_t test_data[128];
	int i;
	int j;
	int var_cnt;

	for (i = 0; i < sizeof(test_data); i++) {
		for (j = 0; j < i; j++) {
			test_data[j] = fcb_test_append_data(i, j);
		}
		rc = fcb_append(&test_fcb, i, &loc);
		zassert_true(rc == 0, "fcb_append call failure");
		rc = flash_area_write(test_fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc),
				      test_data, i);
		zassert_true(rc == 0, "flash_area_write call failure");
		rc = fcb_append_finish(&test_fcb, &loc);
		zassert_true(rc == 0, "fcb_append_finish call failure");
	}

	test_fcb_crc_disabled.f_erase_value = test_fcb.f_erase_value;
	test_fcb_crc_disabled.f_sector_cnt = test_fcb.f_sector_cnt;
	test_fcb_crc_disabled.f_sectors = test_fcb.f_sectors;

	rc = fcb_init(TEST_FCB_FLASH_AREA_ID, &test_fcb_crc_disabled);
	if (rc != 0) {
		printf("%s rc == %xm, %d\n", __func__, rc, rc);
		zassert_true(rc == 0, "fbc initialization failure");
	}

	var_cnt = 0;
	rc = fcb_walk(&test_fcb_crc_disabled, 0, fcb_test_data_walk_cb, &var_cnt);
	zassert_true(rc == 0, "fcb_walk call failure");
	printk("var_cnt: %d", var_cnt);
	zassert_true(var_cnt == sizeof(test_data),
		     "fetched data size not match to wrote data size");
}


ZTEST(fcb_test_with_2sectors_set, test_fcb_crc_disabled_after_enabled)
{
	fcb_pretest_crc_disabled_after_enabled();
}
