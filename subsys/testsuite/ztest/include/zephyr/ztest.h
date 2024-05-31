/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Zephyr Testsuite
 */

#ifndef ZEPHYR_TESTSUITE_INCLUDE_ZTEST_H_
#define ZEPHYR_TESTSUITE_INCLUDE_ZTEST_H_

/**
 * @defgroup ztest Zephyr Testing Framework (ZTest)
 * @ingroup testing
 */

#if !defined(CONFIG_ZTEST) && !defined(ZTEST_UNITTEST)
#error "You need to add CONFIG_ZTEST to your config file."
#endif

#ifndef KERNEL
#define ARCH_STACK_PTR_ALIGN 8
/* FIXME: Properly integrate with Zephyr's arch specific code */
#ifdef __cplusplus
extern "C" {
#endif
struct arch_esf;
#ifdef __cplusplus
}
#endif
#endif /* KERNEL */

#include <zephyr/sys/printk.h>
#define PRINT printk

#include <zephyr/kernel.h>

#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_mock.h>
#include <zephyr/ztest_test.h>
#include <zephyr/tc_util.h>

#ifdef __cplusplus
extern "C" {
#endif

void test_main(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TESTSUITE_INCLUDE_ZTEST_H_ */
