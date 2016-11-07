/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 *
 * @brief Zephyr testing suite
 */

#ifndef __ZTEST_H__
#define __ZTEST_H__

/**
 * @defgroup ztest Zephyr testing suite
 */

#ifndef KERNEL
#define CONFIG_STDOUT_CONSOLE 1
#define CONFIG_ZTEST_ASSERT_VERBOSE 1
#define CONFIG_ZTEST_MOCKING
#define CONFIG_MICROKERNEL 1
#define CONFIG_NUM_COOP_PRIORITIES 16
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

#endif /* __ZTEST_H__ */
