/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <bt_common.h>

#include "common.h"
#include "test_set_iq_sampling_enable.h"

/*test case main entry*/
void test_main(void)
{
	ut_bt_setup();

	common_create_per_sync_set();
	run_set_scan_cte_rx_enable_tests();
}
