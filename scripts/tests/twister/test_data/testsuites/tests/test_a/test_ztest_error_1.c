/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

ztest_test_suite(feature3,
				ztest_unit_test(test_unit_1a),
#ifdef CONFIG_WHATEVER
				ztest_unit_test(test_unit_1b),
#endif
				ztest_unit_test(test_Unit_1c)
				);
				ztest_run_test_suite(feature3);
