/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"
#include <errno.h>

void test_config_getset_unknown(void)
{
	char name[80];
	char tmp[64], *str;
	int rc;

	strcpy(name, "foo/bar");
	rc = settings_set_value(name, "tmp");
	zassert_true(rc != 0, "set value should fail");
	zassert_true(ctest_get_call_state() == 0,
		     "a handler was called unexpectedly");

	strcpy(name, "foo/bar");
	str = settings_get_value(name, tmp, sizeof(tmp));
	zassert_true(str == NULL, "value should been unreachable");
	zassert_true(ctest_get_call_state() == 0,
		     "a handler was called unexpectedly");

	strcpy(name, "myfoo/bar");
	rc = settings_set_value(name, "tmp");
	zassert_true(rc == -ENOENT, "unexpected failure retval");
	zassert_true(test_set_called == 1,
		     "the GET handler wasn't called");
	ctest_clear_call_state();

	strcpy(name, "myfoo/bar");
	str = settings_get_value(name, tmp, sizeof(tmp));
	zassert_true(str == NULL, "value should been unreachable");
	zassert_true(test_get_called == 1,
		     "the SET handler wasn't called");
	ctest_clear_call_state();
}
