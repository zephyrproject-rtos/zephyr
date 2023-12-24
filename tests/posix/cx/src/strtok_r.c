/*
 * Copyright (c) 2020, Siddharth Chandrasekaran
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>

static void test_strtok_r_do(char *str, char *sep, int tlen, const char *const *toks, bool expect)
{
	int len = 0;
	char *state, *tok, buf[64 + 1] = {0};

	strncpy(buf, str, 64);

	tok = strtok_r(buf, sep, &state);
	while (tok && len < tlen) {
		if (strcmp(tok, toks[len]) != 0) {
			break;
		}
		tok = strtok_r(NULL, sep, &state);
		len++;
	}
	if (expect) {
		zassert_equal(len, tlen, "strtok_r error '%s' / '%s'", str, sep);
	} else {
		zassert_not_equal(len, tlen, "strtok_r error '%s' / '%s'", str, sep);
	}
}

ZTEST(posix_cx, test_strtok_r)
{
	static const char *const tc01[] = {"1", "2", "3", "4", "5"};

	test_strtok_r_do("1,2,3,4,5", ",", 5, tc01, true);
	test_strtok_r_do(",, 1 ,2  ,3   4,5  ", ", ", 5, tc01, true);
	test_strtok_r_do("1,,,2 3,,,4 5", ", ", 5, tc01, true);
	test_strtok_r_do("1,2 3,,,4 5  ", ", ", 5, tc01, true);
	test_strtok_r_do("0,1,,,2 3,,,4 5", ", ", 5, tc01, false);
	test_strtok_r_do("1,,,2 3,,,4 5", ",", 5, tc01, false);
	test_strtok_r_do("A,,,2,3,,,4 5", ",", 5, tc01, false);
	test_strtok_r_do("1,,,2,3,,,", ",", 5, tc01, false);
	test_strtok_r_do("1|2|3,4|5", "| ", 5, tc01, false);
}
