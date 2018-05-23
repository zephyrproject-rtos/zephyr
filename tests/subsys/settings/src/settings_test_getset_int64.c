/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"

void test_config_getset_int64(void)
{
	char name[80];
	char tmp[64], *str;
	int rc;
	s64_t cmp;

	strcpy(name, "myfoo/mybar64");
	rc = settings_set_value(name, "-9218247941279444428");
	zassert_true(rc == 0, "can't set value");
	zassert_true(test_set_called == 1, "the SET handler wasn't called");
	cmp = 0x8012345678901234;
	zassert_true(memcmp(&val64, &cmp, sizeof(val64)) == 0,
		     "SET handler: was called with wrong parameters");
	ctest_clear_call_state();

	strcpy(name, "myfoo/mybar64");
	str = settings_get_value(name, tmp, sizeof(tmp));
	zassert_not_null(str, "the key value should been available");
	zassert_true(test_get_called == 1, "the GET handler wasn't called");
	zassert_true(!strcmp("-9218247941279444428", tmp),
			     "unexpected value fetched %s\n", tmp);
	ctest_clear_call_state();

	strcpy(name, "myfoo/mybar64");
	rc = settings_set_value(name, "1");
	zassert_true(rc == 0, "can't set value");
	zassert_true(test_set_called == 1, "the SET handler wasn't called");
	zassert_true(val64 == 1,
		     "SET handler: was called with wrong parameters");
	ctest_clear_call_state();

	strcpy(name, "myfoo/mybar64");
	str = settings_get_value(name, tmp, sizeof(tmp));
	zassert_not_null(str, "the key value should been available");
	zassert_true(test_get_called == 1, "the GET handler wasn't called");
	zassert_true(!strcmp("1", tmp), "unexpected value fetched");
	ctest_clear_call_state();
}
