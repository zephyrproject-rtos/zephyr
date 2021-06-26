/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include "test_add_cte_to_chain.h"
#include "test_remove_cte_from_chain.h"

/* Test case main entry */
void test_main(void)
{
	run_add_cte_to_per_adv_chain_tests();
	run_remove_cte_to_per_adv_chain_tests();
}
