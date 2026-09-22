/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/addr.h>
#include <zephyr/ztest.h>

ZTEST_SUITE(bt_addr_from_str, NULL, NULL, NULL, NULL, NULL);

ZTEST(bt_addr_from_str, test_reject_empty_string)
{
	char *addr_str = "";
	bt_addr_t a;

	zassert_equal(bt_addr_from_str(addr_str, &a), -EINVAL);
}

ZTEST(bt_addr_from_str, test_reject_missing_octet)
{
	char *addr_str = "ab:ab:ab:ab:ab";
	bt_addr_t a;

	zassert_equal(bt_addr_from_str(addr_str, &a), -EINVAL);
}

ZTEST(bt_addr_from_str, test_reject_empty_octet)
{
	char *addr_str = "ab:ab:ab:ab::ab";
	bt_addr_t a;

	zassert_equal(bt_addr_from_str(addr_str, &a), -EINVAL);
}

ZTEST(bt_addr_from_str, test_reject_short_octet)
{
	char *addr_str = "ab:ab:ab:ab:b:ab";
	bt_addr_t a;

	zassert_equal(bt_addr_from_str(addr_str, &a), -EINVAL);
}

ZTEST(bt_addr_from_str, test_reject_trailing_colon)
{
	char *addr_str = "ab:ab:ab:ab:ab:ab:";
	bt_addr_t a;

	zassert_equal(bt_addr_from_str(addr_str, &a), -EINVAL);
}

ZTEST(bt_addr_from_str, test_reject_octet_colon_a)
{
	char *addr_str = "ab:ab:ab:ab:a::ab";
	bt_addr_t a;

	zassert_equal(bt_addr_from_str(addr_str, &a), -EINVAL);
}

ZTEST(bt_addr_from_str, test_reject_octet_colon_b)
{
	char *addr_str = "ab:ab:ab:ab::b:ab";
	bt_addr_t a;

	zassert_equal(bt_addr_from_str(addr_str, &a), -EINVAL);
}

ZTEST(bt_addr_from_str, test_reject_octet_space_a)
{
	char *addr_str = "ab:ab:ab:ab: b:ab";
	bt_addr_t a;

	zassert_equal(bt_addr_from_str(addr_str, &a), -EINVAL);
}

ZTEST(bt_addr_from_str, test_reject_octet_space_b)
{
	char *addr_str = "ab:ab:ab:ab:a :ab";
	bt_addr_t a;

	zassert_equal(bt_addr_from_str(addr_str, &a), -EINVAL);
}

ZTEST(bt_addr_from_str, test_reject_extra_space_before)
{
	char *addr_str = " 00:00:00:00:00:00";
	bt_addr_t a;

	zassert_equal(bt_addr_from_str(addr_str, &a), -EINVAL);
}

ZTEST(bt_addr_from_str, test_reject_extra_space_after)
{
	char *addr_str = "00:00:00:00:00:00 ";
	bt_addr_t a;

	zassert_equal(bt_addr_from_str(addr_str, &a), -EINVAL);
}

ZTEST(bt_addr_from_str, test_reject_replace_space_first)
{
	char *addr_str = " 0:00:00:00:00:00";
	bt_addr_t a;

	zassert_equal(bt_addr_from_str(addr_str, &a), -EINVAL);
}

ZTEST(bt_addr_from_str, test_reject_replace_colon_first)
{
	char *addr_str = ":0:00:00:00:00:00";
	bt_addr_t a;

	zassert_equal(bt_addr_from_str(addr_str, &a), -EINVAL);
}

ZTEST(bt_addr_from_str, test_reject_non_hex)
{
	char *addr_str = "00:00:00:00:g0:00";
	bt_addr_t a;

	zassert_equal(bt_addr_from_str(addr_str, &a), -EINVAL);
}

ZTEST(bt_addr_from_str, test_reject_bad_colon)
{
	char *addr_str = "00.00:00:00:00:00";
	bt_addr_t a;

	zassert_equal(bt_addr_from_str(addr_str, &a), -EINVAL);
}

ZTEST(bt_addr_from_str, test_order)
{
	char *addr_str = "01:02:03:04:05:06";
	bt_addr_t a;
	bt_addr_t b = {{6, 5, 4, 3, 2, 1}};

	zassert_equal(bt_addr_from_str(addr_str, &a), 0);
	zassert_true(bt_addr_eq(&a, &b));
}

ZTEST(bt_addr_from_str, test_hex_case_equal)
{
	char *addr_str_a = "ab:cd:ef:00:00:00";
	char *addr_str_b = "AB:CD:EF:00:00:00";
	bt_addr_t a;
	bt_addr_t b;

	zassert_equal(bt_addr_from_str(addr_str_a, &a), 0);
	zassert_equal(bt_addr_from_str(addr_str_b, &b), 0);
	zassert_true(bt_addr_eq(&a, &b));
}

ZTEST(bt_addr_from_str, test_hex_case_not_equal)
{
	char *addr_str_a = "aa:aa:aa:00:00:00";
	char *addr_str_b = "bb:bb:bb:00:00:00";
	bt_addr_t a;
	bt_addr_t b;

	zassert_equal(bt_addr_from_str(addr_str_a, &a), 0);
	zassert_equal(bt_addr_from_str(addr_str_b, &b), 0);
	zassert_false(bt_addr_eq(&a, &b));
}
