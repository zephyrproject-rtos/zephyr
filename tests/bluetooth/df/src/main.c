/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include "common.h"
#include "test_set_cl_cte_tx_params.h"
#include "test_set_cl_cte_tx_enable.h"


/*test case main entry*/
void test_main(void)
{
	common_setup();
	common_create_adv_set();
	run_set_cl_cte_tx_params_tests();
	common_delete_adv_set();
	run_set_cl_cte_tx_enable_tests();
}
