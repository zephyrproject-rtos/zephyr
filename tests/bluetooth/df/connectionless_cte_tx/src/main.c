/*
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <bt_common.h>
#include <ztest.h>
#include "common.h"

struct bt_test_state test_state;

/*test case main entry*/
void test_main(void)
{
	ut_bt_setup();
	test_state.is_setup = true;

	common_create_adv_set();
	ztest_run_registered_test_suites(&test_state);

	common_delete_adv_set();
	ztest_run_registered_test_suites(&test_state);
}
