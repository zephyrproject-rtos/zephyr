/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"
#include <errno.h>

ZTEST(settings_config, test_config_getset_unknown)
{
	char name[80];
	char tmp[64];
	int rc;

	strcpy(name, "foo/bar");
	rc = settings_runtime_set(name, "tmp", 4);
	zassert_true(rc != 0, "set value should fail");
	zassert_true(ctest_get_call_state() == 0,
		     "a handler was called unexpectedly");

	strcpy(name, "foo/bar");
	rc = settings_runtime_get(name, tmp, sizeof(tmp));
	zassert_true(rc == -EINVAL, "value should been unreachable");
	zassert_true(ctest_get_call_state() == 0,
		     "a handler was called unexpectedly");

	strcpy(name, "myfoo/bar");
	rc = settings_runtime_set(name, "tmp", 4);
	zassert_true(rc == -ENOENT, "unexpected failure retval\n");
	zassert_true(test_set_called == 1,
		     "the GET handler wasn't called");
	ctest_clear_call_state();

	strcpy(name, "myfoo/bar");
	rc = settings_runtime_get(name, tmp, sizeof(tmp));
	zassert_true(rc == -ENOENT, "value should been unreachable\n");
	zassert_true(test_get_called == 1,
		     "the SET handler wasn't called");
	ctest_clear_call_state();
}
