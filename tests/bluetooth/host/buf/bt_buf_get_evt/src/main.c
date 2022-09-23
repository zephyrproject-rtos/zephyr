/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

void test_main(void)
{
	uint32_t state;

	ztest_run_registered_test_suites(&state);
}
