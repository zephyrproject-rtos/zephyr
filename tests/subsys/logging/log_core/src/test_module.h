/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_MODULE_H
#define TEST_MODULE_H

#include <logging/log.h>

void test_func(void);

static inline void test_inline_func(void)
{
	LOG_MODULE_DECLARE(test);

	LOG_ERR("inline");
}

#endif /* TEST_MODULE_H */
