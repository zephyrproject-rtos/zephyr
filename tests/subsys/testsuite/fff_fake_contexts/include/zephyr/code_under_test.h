/*
 * Copyright (c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_SUBSYS_TESTSUITE_FFF_FAKE_CONTEXTS_INCLUDE_ZEPHYR_CODE_UNDER_TEST_H_
#define ZEPHYR_TESTS_SUBSYS_TESTSUITE_FFF_FAKE_CONTEXTS_INCLUDE_ZEPHYR_CODE_UNDER_TEST_H_

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opens and closes called_API session twice
 *
 * This routine loops twice on opening/closing a called_API session.
 * Failures cause early termination of the loop, and the error number
 * from the called routine is returned by this routine.  This routine
 * is designed for exercising custom_fake routines that need to use
 * call-specific data.
 *
 * @return zero(0) upon success
 * @return -EINVAL if invalid parameter(s)
 * @return -E2BIG  if more calls were made than expected.
 */
int code_under_test(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TESTS_SUBSYS_TESTSUITE_FFF_FAKE_CONTEXTS_INCLUDE_ZEPHYR_CODE_UNDER_TEST_H_ */
