/*
 * Copyright (c) 2024 Keith Packard
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>
/**
 * @addtogroup kernel_common_tests
 * @{
 */

/**
 * @brief Test if constructors work
 *
 */

static int constructor_value;

#define CONSTRUCTOR_VALUE	1

void
__attribute__((__constructor__))
__constructor_init(void)
{
	constructor_value = CONSTRUCTOR_VALUE;
}

ZTEST(constructor, test_constructor)
{
	zassert_equal(constructor_value, CONSTRUCTOR_VALUE,
		      "constructor test failed: constructor not called");
}

extern void *common_setup(void);
ZTEST_SUITE(constructor, NULL, common_setup, NULL, NULL, NULL);

/**
 * @}
 */
