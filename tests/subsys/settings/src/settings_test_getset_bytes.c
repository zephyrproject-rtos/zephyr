/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"

void test_config_getset_bytes(void)
{
	char orig[32];
	char bytes[32];
	char str[48];
	char *ret;
	int j, i;
	int tmp;
	int rc;

	for (j = 1; j < sizeof(orig); j++) {
		for (i = 0; i < j; i++) {
			orig[i] = i + j + 1;
		}
		ret = settings_str_from_bytes(orig, j, str, sizeof(str));
		zassert_not_null(ret, "string base64 encodding");
		tmp = strlen(str);
		zassert_true(tmp < sizeof(str), "encoded string is to long");

		(void)memset(bytes, 0, sizeof(bytes));
		tmp = sizeof(bytes);

		tmp = sizeof(bytes);
		rc = settings_bytes_from_str(str, bytes, &tmp);
		zassert_true(rc == 0, "base64 to string decodding");
		zassert_true(tmp == j, "decoded string bad length");
		zassert_true(!memcmp(orig, bytes, j),
			     "decoded string not match to origin");
	}
}
