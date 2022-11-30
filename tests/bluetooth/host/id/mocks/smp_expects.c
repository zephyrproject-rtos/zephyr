/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/smp.h"
#include "mocks/smp_expects.h"

#include <zephyr/kernel.h>

void expect_single_call_bt_smp_le_oob_generate_sc_data(struct bt_le_oob_sc_data *le_sc_oob)
{
	const char *func_name = "bt_smp_le_oob_generate_sc_data";

	zassert_equal(bt_smp_le_oob_generate_sc_data_fake.call_count, 1,
		      "'%s()' was called more than once", func_name);
}

void expect_not_called_bt_smp_le_oob_generate_sc_data(void)
{
	const char *func_name = "bt_smp_le_oob_generate_sc_data";

	zassert_equal(bt_smp_le_oob_generate_sc_data_fake.call_count, 0,
		      "'%s()' was called unexpectedly", func_name);
}
