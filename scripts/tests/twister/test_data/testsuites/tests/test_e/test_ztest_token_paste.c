/*
 * Copyright 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

ZTEST_SUITE(feature6, NULL, NULL, NULL, NULL, NULL);

ZTEST(feature6, test_normal_case);

/*
 * Cases generated with the ## token-paste operator (as in
 * tests/drivers/dma/loop_transfer) cannot be resolved statically and must not
 * be registered as testcases; otherwise a phantom "feature6.dma" case is
 * created that never runs.
 */
#define DEFINE_LOOP_TESTS(idx, _)                                                                  \
	ZTEST(feature6, test_dma##idx##_m2m_loop);

LISTIFY(2, DEFINE_LOOP_TESTS, ())

ztest_run_registered_test_suites(feature6);
