/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>

#include "../../../lib/utils/hex.c"

ZTEST(hex, test_char2hex_valid)
{
	uint8_t x;

	zassert_ok(char2hex('0', &x));
	zassert_equal(x, 0);
	zassert_ok(char2hex('9', &x));
	zassert_equal(x, 9);
	zassert_ok(char2hex('a', &x));
	zassert_equal(x, 10);
	zassert_ok(char2hex('f', &x));
	zassert_equal(x, 15);
	zassert_ok(char2hex('A', &x));
	zassert_equal(x, 10);
	zassert_ok(char2hex('F', &x));
	zassert_equal(x, 15);
}

ZTEST(hex, test_char2hex_invalid)
{
	uint8_t x;

	zassert_equal(char2hex('g', &x), -EINVAL);
	zassert_equal(char2hex('G', &x), -EINVAL);
	zassert_equal(char2hex('!', &x), -EINVAL);
}

ZTEST(hex, test_hex2char_valid)
{
	char c;

	zassert_ok(hex2char(0, &c));
	zassert_equal(c, '0');
	zassert_ok(hex2char(9, &c));
	zassert_equal(c, '9');
	zassert_ok(hex2char(10, &c));
	zassert_equal(c, 'a');
	zassert_ok(hex2char(15, &c));
	zassert_equal(c, 'f');
}

ZTEST(hex, test_hex2char_invalid)
{
	char c;

	zassert_equal(hex2char(16, &c), -EINVAL);
}

ZTEST(hex, test_bin2hex)
{
	uint8_t buf[] = {0x00, 0x10, 0xFF, 0x3A};
	char hexstr[9];
	size_t len;

	len = bin2hex(buf, sizeof(buf), hexstr, sizeof(hexstr));
	zassert_equal(len, 8);
	zassert_mem_equal(hexstr, "0010ff3a", 9);
}

ZTEST(hex, test_bin2hex_too_small)
{
	uint8_t buf[] = {0xAA};
	char hexstr[2];

	zassert_equal(bin2hex(buf, sizeof(buf), hexstr, sizeof(hexstr)), 0);
}

ZTEST(hex, test_hex2bin_even)
{
	const char *hexstr = "0010ff3a";
	uint8_t expected[] = {0x00, 0x10, 0xFF, 0x3A};
	uint8_t buf[4];
	size_t len;

	len = hex2bin(hexstr, strlen(hexstr), buf, sizeof(buf));
	zassert_equal(len, 4);
	zassert_mem_equal(buf, expected, sizeof(expected));
}

ZTEST(hex, test_hex2bin_odd)
{
	const char *hexstr = "abc";
	uint8_t expected[] = {0x0a, 0xbc};
	uint8_t buf[2];
	size_t len;

	len = hex2bin(hexstr, strlen(hexstr), buf, sizeof(buf));
	zassert_equal(len, 2);
	zassert_mem_equal(buf, expected, sizeof(expected));
}

ZTEST(hex, test_hex2bin_invalid)
{
	const char *hexstr1 = "g04";
	const char *hexstr2 = "01g4";
	const char *hexstr3 = "014g";
	uint8_t buf[2];

	zassert_equal(hex2bin(hexstr1, strlen(hexstr1), buf, sizeof(buf)), 0);
	zassert_equal(hex2bin(hexstr2, strlen(hexstr2), buf, sizeof(buf)), 0);
	zassert_equal(hex2bin(hexstr3, strlen(hexstr3), buf, sizeof(buf)), 0);
}

ZTEST(hex, test_hex2bin_buf_too_small)
{
	const char *hexstr = "abc";
	uint8_t buf[1];

	zassert_equal(hex2bin(hexstr, strlen(hexstr), buf, sizeof(buf)), 0);
}

ZTEST_SUITE(hex, NULL, NULL, NULL, NULL, NULL);
