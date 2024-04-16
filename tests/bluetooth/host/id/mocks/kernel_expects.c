/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/kernel.h"
#include "mocks/kernel_expects.h"

#include <zephyr/kernel.h>

void expect_single_call_k_work_init_delayable(struct k_work_delayable *dwork)
{
	const char *func_name = "k_work_init_delayable";

	zassert_equal(k_work_init_delayable_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);
	zassert_equal(k_work_init_delayable_fake.arg0_val, dwork,
		      "'%s()' was called with incorrect '%s' value", func_name, "dwork");
	zassert_not_null(k_work_init_delayable_fake.arg1_val,
			 "'%s()' was called with incorrect '%s' value", func_name, "handler");
}
