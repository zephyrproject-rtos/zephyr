/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"

ZTEST(settings_config, test_config_getset_int)
{
	char name[80];
	char tmp[64];
	int rc;
	uint8_t small_value;

	small_value = 42U;
	strcpy(name, "myfoo/mybar");
	rc = settings_runtime_set(name, &small_value, sizeof(small_value));
	zassert_true(rc == 0, "can not set key value");
	zassert_true(test_set_called == 1, "the SET handler wasn't called");
	zassert_true(val8 == 42,
		     "SET handler: was called with wrong parameters");
	ctest_clear_call_state();

	strcpy(name, "myfoo/mybar");
	rc = settings_runtime_get(name, tmp, sizeof(tmp));
	zassert_true(rc == 1, "the key value should been available");
	zassert_true(test_get_called == 1, "the GET handler wasn't called");
	zassert_equal(42, tmp[0], "unexpected value fetched");
	ctest_clear_call_state();
}
