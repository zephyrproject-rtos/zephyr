/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>

#include <bt_common.h>

#include <bt_conn_common.h>
#include "test_cte_set_rx_params.h"
#include "test_cte_req_enable.h"

/*test case main entry*/
void test_main(void)
{
	ut_bt_setup();

	run_set_cte_rx_params_tests();
	run_cte_request_enable_tests();
}
