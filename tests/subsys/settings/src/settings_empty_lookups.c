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
	char tmp[64], *str;

	strcpy(name, "foo/bar");
	rc = settings_set_value(name, "tmp");
	zassert_true(rc != 0, "settings_set_value callback");

	strcpy(name, "foo/bar");
	str = settings_get_value(name, tmp, sizeof(tmp));
	zassert_true(str == NULL, "settings_get_value callback");
}
