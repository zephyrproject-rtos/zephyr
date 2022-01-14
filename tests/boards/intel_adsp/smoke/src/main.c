/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <ztest.h>
#include "tests.h"

void test_main(void)
{
	ztest_test_suite(intel_adsp,
			 ztest_unit_test(test_smp_boot_delay),
			 ztest_unit_test(test_post_boot_ipi),
			 ztest_unit_test(test_host_ipc)
			 );

	ztest_run_test_suite(intel_adsp);
}
