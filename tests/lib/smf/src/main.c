/*
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>

#include <zephyr/smf.h>
#include "test_lib_smf.h"

void test_main(void)
{
	if (IS_ENABLED(CONFIG_SMF_ANCESTOR_SUPPORT)) {
		ztest_test_suite(smf_tests,
			ztest_unit_test(test_smf_hierarchical),
			ztest_unit_test(test_smf_hierarchical_5_ancestors));
		ztest_run_test_suite(smf_tests);
	} else {
		ztest_test_suite(smf_tests,
			ztest_unit_test(test_smf_flat));
		ztest_run_test_suite(smf_tests);
	}
}
