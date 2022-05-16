/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_MODULE_H
#define TEST_MODULE_H

#include <zephyr/logging/log.h>

#ifdef __cplusplus
extern "C" {
#endif

void test_func(void);
void test_func2(void);

#ifdef __cplusplus
}
#endif

#define TEST_INLINE_DBG_MSG "inline debug"
#define TEST_INLINE_ERR_MSG "inline"

#define TEST_DBG_MSG "debug"
#define TEST_ERR_MSG "test"

static inline void test_inline_func(void)
{
	LOG_MODULE_DECLARE(test, CONFIG_SAMPLE_MODULE_LOG_LEVEL);

	LOG_DBG(TEST_INLINE_DBG_MSG);
	LOG_ERR(TEST_INLINE_ERR_MSG);
}

#endif /* TEST_MODULE_H */
