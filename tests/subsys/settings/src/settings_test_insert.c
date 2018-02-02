/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"

void test_config_insert_x(int idx)
{
	int rc;

	rc = settings_register(&c_test_handlers[idx]);
	zassert_true(rc == 0, "settings_register fail");
}

void test_config_insert(void)
{
	test_config_insert_x(0);
}

void test_config_insert2(void)
{
	test_config_insert_x(1);
}

void test_config_insert3(void)
{
	test_config_insert_x(2);
}
