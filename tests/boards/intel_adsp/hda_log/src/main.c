/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/zephyr.h>
#include <ztest.h>
#include <stdlib.h>
#include "tests.h"

void test_main(void)
{
	ztest_test_suite(intel_adsp_hda,
			 ztest_unit_test(test_hda_logger)
			 );

	ztest_run_test_suite(intel_adsp_hda);
}
