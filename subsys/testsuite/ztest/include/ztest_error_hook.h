/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZTEST_FATAL_HOOK_H_
#define ZEPHYR_INCLUDE_ZTEST_FATAL_HOOK_H_

#include <zephyr.h>


#if defined(CONFIG_ZTEST_FATAL_HOOK)
/**
 * @brief Set the flag that treat fatal error happened as expected
 *
 * @details This is used for negative test cases which triggers a fatal
 * error. Set the param true will still pass the test case when expected
 * fatal error happened. For normal test case, set it false makes it back
 * to normal behavior.
 *
 * @param valid flag indicate fault is expected
 */
__syscall void ztest_set_fault_valid(bool valid);

/* @brief A hook after fatal error handler
 *
 * @details This is a test case hook that can run code from test case, in
 * order to deal with some special case when catching the expected fatal
 * error.
 *
 * Usage: Define your own hook function in your test case code, and do what
 * you want to do after fatal error handler.
 *
 * By default, it will do nothing before leaving error handler.
 */
void ztest_post_fatal_error_hook(unsigned int reason,
		const z_arch_esf_t *pEsf);

#endif


#if defined(CONFIG_ZTEST_ASSERT_HOOK)
/**
 * @brief Set the flag that treat assert fail happened as expected
 *
 * @details This is used for negative test cases which triggers a assert
 * fail. Set the param true will still pass the test case when expected
 * assert fail happened. For normal test case, set it false make it back
 * to normal behavior.
 *
 * @param valid flag indicate assert is expected
 */
__syscall void ztest_set_assert_valid(bool valid);

/* @brief A hook after assert fault handler
 *
 * @details This is a test case hook that can run code from test case, in
 * order to deal with some special case when catching the expected assert
 * failed.
 *
 * Usage: Define your own hook function in your test case code, and do what
 * you want to do after assert handler.
 *
 * By default, it will abort the thread which assert failed.
 */
void ztest_post_assert_fail_hook(void);

#endif

#if defined(CONFIG_ZTEST_FATAL_HOOK)
#include <syscalls/ztest_error_hook.h>
#endif

#endif /* ZEPHYR_INCLUDE_ZTEST_FATAL_HOOK_H_ */
