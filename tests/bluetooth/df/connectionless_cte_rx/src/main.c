/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <bt_common.h>
#include <ztest.h>

#include "common.h"

/*test case main entry*/
void test_main(void)
{
	ut_bt_setup();
	common_create_per_sync_set();

	ztest_run_test_suites(NULL);
	ztest_verify_all_test_suites_ran();
}
