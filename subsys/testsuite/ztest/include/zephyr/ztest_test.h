/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTSUITE_INCLUDE_ZTEST_TEST_H_
#define ZEPHYR_TESTSUITE_INCLUDE_ZTEST_TEST_H_

#ifdef CONFIG_ZTEST_NEW_API
#include <zephyr/ztest_test_new.h>
#else
#include <zephyr/ztest_test_deprecated.h>
#endif /* !CONFIG_ZTEST_NEW_API */

#ifdef __cplusplus
extern "C" {
#endif

__syscall void z_test_1cpu_start(void);
__syscall void z_test_1cpu_stop(void);

__syscall void sys_clock_tick_set(uint64_t tick);

#ifdef __cplusplus
}
#endif

#ifndef ZTEST_UNITTEST
#include <syscalls/ztest_test.h>
#endif

#endif /* ZEPHYR_TESTSUITE_INCLUDE_ZTEST_TEST_H_ */
