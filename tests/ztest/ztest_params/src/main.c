/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

/* initialize the example input data sets */
int int_value = 10;
int int_table[] = {0, 1, 2, 3};
char char_value = 'Z';
char char_table[] = {'H', 'e', 'l', 'l', 'o', ' ', 'Z', 'e', 'p', 'h', 'y', 'r'};
char string[] = "Hello Zephyr";

ZTEST_SUITE(ztest_params, NULL, NULL, NULL, NULL, NULL);

ZTEST_PARAM(ztest_params, test_int_value, &int_value)
{
	zassert_equal(*((int *)params), 10);
}

ZTEST_PARAM(ztest_params, test_int_table, int_table)
{
	for (int i = 0; i < 4; i++) {
		zassert_equal(((int *)params)[i], i);
	}
}

ZTEST_PARAM(ztest_params, test_char_value, &char_value)
{
	zassert_equal('Z', *((char *)(params)));
}

ZTEST_PARAM(ztest_params, test_char_table, char_table)
{
	zassert_equal('H', ((char *)params)[0]);
	zassert_equal('e', ((char *)params)[1]);
	zassert_equal('l', ((char *)params)[2]);
	zassert_equal('l', ((char *)params)[3]);
	zassert_equal('o', ((char *)params)[4]);
	zassert_equal(' ', ((char *)params)[5]);
	zassert_equal('Z', ((char *)params)[6]);
	zassert_equal('e', ((char *)params)[7]);
	zassert_equal('p', ((char *)params)[8]);
	zassert_equal('h', ((char *)params)[9]);
	zassert_equal('y', ((char *)params)[10]);
	zassert_equal('r', ((char *)params)[11]);
}

ZTEST_PARAM(ztest_params, test_string, string)
{
	zassert_equal(strcmp("Hello Zephyr", (char *)(params)), 0, "Strings are not equal");
}
