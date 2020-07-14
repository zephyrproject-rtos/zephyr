/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>

#include "altera_avalon_sysid.h"

void test_sysid(void)
{
	int32_t sysid, status = TC_FAIL;

	sysid = alt_avalon_sysid_test();
	if (!sysid) {
		status = TC_PASS;
		TC_PRINT("[SysID] hardware and software appear to be in sync\n");
	} else if (sysid > 0) {
		TC_PRINT("[SysID] software appears to be older than hardware\n");
	} else {
		TC_PRINT("[SysID] hardware appears to be older than software\n");
	}

	zassert_equal(status, TC_PASS, "SysID test failed");
}

void test_main(void)
{
	ztest_test_suite(nios2_sysid_test_suite,
			ztest_unit_test(test_sysid));
	ztest_run_test_suite(nios2_sysid_test_suite);
}
