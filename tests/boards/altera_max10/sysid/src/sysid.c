/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "altera_avalon_sysid.h"

ZTEST(nios2_sysid, test_sysid)
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

ZTEST_SUITE(nios2_sysid, NULL, NULL, NULL, NULL, NULL);
