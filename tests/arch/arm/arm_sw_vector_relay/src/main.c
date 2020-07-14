/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_CPU_CORTEX_M)
  #error test can only run on Cortex-M MCUs
#endif

#include <ztest.h>

extern void test_arm_sw_vector_relay(void);

void test_main(void)
{
	ztest_test_suite(arm_sw_vector_relay,
		ztest_unit_test(test_arm_sw_vector_relay));
	ztest_run_test_suite(arm_sw_vector_relay);
}
