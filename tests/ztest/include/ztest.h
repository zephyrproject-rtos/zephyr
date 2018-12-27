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
#define CONFIG_SYS_CLOCK_TICKS_PER_SEC 100
#define CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC 10000000
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

#ifdef CONFIG_APP_SHARED_MEM
/* Application memory define */
APPMEM_DOMAIN_HEADER_DEFINE(ztest_dom0);
APPMEM_PARTITION_HEADER_DEFINE(ztest_part0);

#define APP_DATA_MEM _app_dmem(ztest_part0)
#define APP_BSS_MEM _app_bmem(ztest_part0)
#else
/* no shared memory is selected therefore macros are empty */
#define APP_DATA_MEM
#define APP_BSS_MEM
#endif

void test_main(void);

#ifdef __cplusplus
}
#endif

#endif /* __ZTEST_H__ */
