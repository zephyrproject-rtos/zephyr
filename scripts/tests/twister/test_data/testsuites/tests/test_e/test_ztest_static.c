/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

ZTEST_SUITE(feature7, NULL, NULL, NULL, NULL, NULL);

ZTEST(feature7, test_plain_case);
static ZTEST(feature7, test_static_case);
static ZTEST_F(feature7, test_static_fixture_case);

ztest_run_registered_test_suites(feature7);
