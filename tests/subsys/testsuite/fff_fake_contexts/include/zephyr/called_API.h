/*
 * Copyright (c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_SUBSYS_TESTSUITE_FFF_FAKE_CONTEXTS_INCLUDE_ZEPHYR_CALLED_API_H_
#define ZEPHYR_TESTS_SUBSYS_TESTSUITE_FFF_FAKE_CONTEXTS_INCLUDE_ZEPHYR_CALLED_API_H_

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A container struct for hidden instance data */
struct called_API_info;

/**
 * @brief Provide an instance handle to a session of the called_API.
 *
 * This API is defined for the code_under_test() example to call.
 * It represents a routine which provides call-unique data to the caller
 * as well as providing a return value.  This requires the _custom_fake
 * implementation to serve the data from a custom_fake context struture,
 * where each call needs to return a unique, configured value.
 *
 * @param instance_out Session instance handle for caller to use.
 *
 * @return zero(0) upon success, with *instance_out updated.
 * @return -EINVAL if invalid parameter(s)
 * @return -E2BIG  if more calls were made than expected.
 */
int called_API_open(const struct called_API_info **instance_out);

/**
 * @brief Return an instance handle to a session of the called_API.
 *
 * This API is defined for the code_under_test() example to call.
 * It represents a routine which requires specific data from the caller
 * as well as providing a return value.  This is defined to
 * have the code_under_test call multiple functions to illustrate
 * a way to verify a specific calling sequence was made
 *
 * @param instance Session instance handle provided by called_API_open
 *
 * @return zero(0) upon success, with instance invalidated.
 * @return -EINVAL if invalid parameter(s)
 * @return -E2BIG  if more calls were made than expected.
 */
int called_API_close(const struct called_API_info *instance);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TESTS_SUBSYS_TESTSUITE_FFF_FAKE_CONTEXTS_INCLUDE_ZEPHYR_CALLED_API_H_ */
