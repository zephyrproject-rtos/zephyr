/*
 * Copyright (c) 2017 - 2020 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fcb_test.h"

ZTEST(fcb_test_without_set, test_fcb_init)
{
	int rc;
	struct fcb *fcb;

	fcb = &test_fcb;
	(void)memset(fcb, 0, sizeof(*fcb));
	fcb->f_erase_value = fcb_test_erase_value;

	rc = fcb_init(TEST_FCB_FLASH_AREA_ID, fcb);
	zassert_true(rc == -EINVAL, "fcb_init call should fail");

	fcb->f_sectors = test_fcb_sector;

	rc = fcb_init(TEST_FCB_FLASH_AREA_ID, fcb);
	zassert_true(rc == -EINVAL, "fcb_init call should fail");

	fcb->f_sector_cnt = 2U;
	fcb->f_magic = 0x12345678;
	rc = fcb_init(TEST_FCB_FLASH_AREA_ID, fcb);
	zassert_true(rc == -ENOMSG, "fcb_init call should fail");

	fcb->f_magic = 0U;
	rc = fcb_init(TEST_FCB_FLASH_AREA_ID, fcb);
	zassert_true(rc == 0,  "fcb_init call failure");
}
