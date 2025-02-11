/*
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

ZTEST_SUITE(feature5, NULL, NULL, NULL, NULL, NULL);

ZTEST(feature5, test_1a);
ZTEST(feature5, test_1b);

ztest_run_registered_test_suites(feature4);
