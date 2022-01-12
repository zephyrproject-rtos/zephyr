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

/**
 * @brief Zephyr Tests (ZTest)
 * @defgroup all_tests Zephyr Tests
 * @{
 * @}
 */

#ifndef ZEPHYR_TESTSUITE_INCLUDE_ZTEST_H_
#define ZEPHYR_TESTSUITE_INCLUDE_ZTEST_H_

/**
 * @defgroup ztest Zephyr testing suite
 */

#if !defined(CONFIG_ZTEST) && !defined(ZTEST_UNITTEST)
#error "You need to add CONFIG_ZTEST to your config file."
#endif

#ifndef KERNEL
#define CONFIG_STDOUT_CONSOLE 1
#define CONFIG_ZTEST_ASSERT_VERBOSE 1
#define CONFIG_ZTEST_MOCKING
#define CONFIG_NUM_COOP_PRIORITIES 16
#define CONFIG_COOP_ENABLED 1
#define CONFIG_PREEMPT_ENABLED 1
#define CONFIG_MP_NUM_CPUS 1
#define CONFIG_SYS_CLOCK_TICKS_PER_SEC 100
#define CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC 10000000
#define CONFIG_SYS_CLOCK_MAX_TIMEOUT_DAYS 365
#define ARCH_STACK_PTR_ALIGN 8
/* FIXME: Properly integrate with Zephyr's arch specific code */
#define CONFIG_X86 1
#define CONFIG_PRINTK 1
#ifdef __cplusplus
extern "C" {
#endif
struct esf;
typedef struct esf z_arch_esf_t;
#ifdef __cplusplus
}
#endif
#endif /* KERNEL */

#include <sys/printk.h>
#define PRINT printk

#include <zephyr.h>

#include <ztest_assert.h>
#include <ztest_mock.h>
#include <ztest_test.h>
#include <tc_util.h>

#ifdef __cplusplus
extern "C" {
#endif

void test_main(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TESTSUITE_INCLUDE_ZTEST_H_ */
