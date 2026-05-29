/*
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

ztest_register_test_suite(feature4, NULL,
			  ztest_unit_test(test_unit_1a),
			  ztest_unit_test(test_unit_1b));
