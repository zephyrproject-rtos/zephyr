/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>

#include <bt_common.h>

#include "test_set_conn_cte_tx_params.h"

/*test case main entry*/
void test_main(void)
{
	ut_bt_setup();

	run_set_conn_cte_tx_params_tests();
}
