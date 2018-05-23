/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"

void test_config_getset_int(void)
{
	char name[80];
	char tmp[64], *str;
	int rc;

	strcpy(name, "myfoo/mybar");
	rc = settings_set_value(name, "42");
	zassert_true(rc == 0, "can not set key value");
	zassert_true(test_set_called == 1, "the SET handler wasn't called");
	zassert_true(val8 == 42,
		     "SET handler: was called with wrong parameters");
	ctest_clear_call_state();

	strcpy(name, "myfoo/mybar");
	str = settings_get_value(name, tmp, sizeof(tmp));
	zassert_not_null(str, "the key value should been available");
	zassert_true(test_get_called == 1, "the GET handler wasn't called");
	zassert_true(!strcmp("42", tmp), "unexpected value fetched");
	ctest_clear_call_state();
}
