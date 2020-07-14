/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fcb_test.h"

void test_fcb_len(void)
{
	uint8_t buf[3];
	uint16_t len;
	uint16_t len2;
	int rc;
	int rc2;

	for (len = 0U; len < FCB_MAX_LEN; len++) {
		rc = fcb_put_len(buf, len);
		zassert_true(rc == 1 || rc == 2, "fcb_pull_len call failure");

		rc2 = fcb_get_len(buf, &len2);
		zassert_true(rc2 == rc, "fcb_get_len call failure");

		zassert_true(len == len2, "fcb_get_len call failure");
	}
}
