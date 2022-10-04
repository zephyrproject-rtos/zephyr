/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"

void test_config_commit(void)
{
	char name[80];
	int rc;

	strcpy(name, "bar");
	rc = settings_runtime_commit(name);
	zassert_true(rc, "commit-nonexisting-tree call should succeed");
	zassert_true(ctest_get_call_state() == 0,
		     "a handler was called unexpectedly");

	rc = settings_commit();
	zassert_true(rc == 0, "commit-All call should succeed");
	zassert_true(test_commit_called == 1,
		     "the COMMIT handler wasn't called");
	ctest_clear_call_state();

	strcpy(name, "myfoo");
	rc = settings_runtime_commit(name);
	zassert_true(rc == 0, "commit-a-tree call should succeed");
	zassert_true(test_commit_called == 1,
		     "the COMMIT handler wasn't called");
	ctest_clear_call_state();
}
