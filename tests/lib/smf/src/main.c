/*
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>

#include <smf.h>
#include "test_lib_smf.h"

void test_main(void)
{
	ztest_test_suite(smf_tests,
		ztest_unit_test(test_smf_transitions));

	ztest_run_test_suite(smf_tests);
}
