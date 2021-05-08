/*
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <bt_common.h>
#include "common.h"
#include "test_set_cl_cte_tx_params.h"
#include "test_set_cl_cte_tx_enable.h"

/*test case main entry*/
void test_main(void)
{
	ut_bt_setup();

	common_create_adv_set();
	run_set_cl_cte_tx_params_tests();
	common_delete_adv_set();
	run_set_cl_cte_tx_enable_tests();
}
