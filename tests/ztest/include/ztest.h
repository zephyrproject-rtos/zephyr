/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Zephyr testing suite
 */

/**
 * @brief Zephyr Tests
 * @defgroup all_tests Zephyr Tests
 * @{
 * @}
 */

#ifndef __ZTEST_H__
#define __ZTEST_H__

#ifdef __cplusplus
extern "C" {
#endif

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
/* FIXME: Properly integrate with Zephyr's arch specific code */
#define CONFIG_X86 1
#define PRINT printf
#endif /* !KERNEL */

#include <zephyr.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define  PRINT printf
#else /* !CONFIG_STDOUT_CONSOLE */
#include <misc/printk.h>
#define PRINT printk
#endif /* CONFIG_STDOUT_CONSOLE */

#include <ztest_assert.h>
#include <ztest_mock.h>
#include <ztest_test.h>
#include <tc_util.h>
#include <logging/sys_log.h>

void test_main(void);

#ifdef __cplusplus
}
#endif

#endif /* __ZTEST_H__ */
