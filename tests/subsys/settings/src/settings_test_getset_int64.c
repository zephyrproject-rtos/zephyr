/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"

ZTEST(settings_config, test_config_getset_int64)
{
	char name[80];
	char tmp[64];
	int rc;
	int64_t new_val64;

	new_val64 = 0x8012345678901234;
	strcpy(name, "myfoo/mybar64");
	rc = settings_runtime_set(name, &new_val64, sizeof(int64_t));
	zassert_true(rc == 0, "can't set value");
	zassert_true(test_set_called == 1, "the SET handler wasn't called");
	zassert_equal(val64, 0x8012345678901234,
		     "SET handler: was called with wrong parameters");
	ctest_clear_call_state();

	strcpy(name, "myfoo/mybar64");
	rc = settings_runtime_get(name, tmp, sizeof(tmp));
	zassert_equal(rc, sizeof(int64_t), "the key value should been available");
	zassert_true(test_get_called == 1, "the GET handler wasn't called");
	memcpy(&new_val64, tmp, sizeof(int64_t));
	zassert_equal(new_val64, 0x8012345678901234,
		      "unexpected value fetched %d", tmp);
	ctest_clear_call_state();

	new_val64 = 1;
	strcpy(name, "myfoo/mybar64");
	rc = settings_runtime_set(name, &new_val64, sizeof(int64_t));
	zassert_true(rc == 0, "can't set value");
	zassert_true(test_set_called == 1, "the SET handler wasn't called");
	zassert_equal(val64, 1,
		     "SET handler: was called with wrong parameters");
	ctest_clear_call_state();

	strcpy(name, "myfoo/mybar64");
	rc = settings_runtime_get(name, tmp, sizeof(tmp));
	zassert_equal(rc, sizeof(int64_t), "the key value should been available");
	zassert_true(test_get_called == 1, "the GET handler wasn't called");
	memcpy(&new_val64, tmp, sizeof(int64_t));
	zassert_equal(new_val64, 1,
		      "unexpected value fetched %d", tmp);
	ctest_clear_call_state();
}
