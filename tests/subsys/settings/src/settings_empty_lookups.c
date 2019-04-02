/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"

void config_empty_lookups(void)
{
	int rc;
	char name[80];
	char tmp[64];

	strcpy(name, "foo/bar");
	rc = settings_runtime_set(name, "tmp", 4);
	zassert_true(rc != 0, "settings_runtime_set callback");

	strcpy(name, "foo/bar");
	rc = settings_runtime_get(name, tmp, sizeof(tmp));
	zassert_true(rc == -EINVAL, "settings_runtime_get callback");
}
