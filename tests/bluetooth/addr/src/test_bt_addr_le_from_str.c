/* Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/ztest.h>

ZTEST_SUITE(bt_addr_le_from_str, NULL, NULL, NULL, NULL, NULL);

/* The example addresses used throughout these tests, in on-air (little-endian) order. */
static const bt_addr_t example = { { 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 } };
static const bt_addr_t example_alpha = { { 0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa } };

static ZTEST(bt_addr_le_from_str, test_public)
{
	bt_addr_le_t a;

	zassert_equal(bt_addr_le_from_str("P:11:22:33:44:55:66", &a), 0);
	zassert_equal(a.type, BT_ADDR_LE_PUBLIC);
	zassert_true(bt_addr_eq(&a.a, &example));
}

static ZTEST(bt_addr_le_from_str, test_random)
{
	bt_addr_le_t a;

	zassert_equal(bt_addr_le_from_str("R:11:22:33:44:55:66", &a), 0);
	zassert_equal(a.type, BT_ADDR_LE_RANDOM);
	zassert_true(bt_addr_eq(&a.a, &example));
}

static ZTEST(bt_addr_le_from_str, test_prefix_and_digits_are_case_insensitive)
{
	bt_addr_le_t a;

	zassert_equal(bt_addr_le_from_str("p:11:22:33:44:55:66", &a), 0);
	zassert_equal(a.type, BT_ADDR_LE_PUBLIC);
	zassert_true(bt_addr_eq(&a.a, &example));

	zassert_equal(bt_addr_le_from_str("r:aa:bb:cc:dd:ee:ff", &a), 0);
	zassert_equal(a.type, BT_ADDR_LE_RANDOM);
	zassert_true(bt_addr_eq(&a.a, &example_alpha));

	zassert_equal(bt_addr_le_from_str("R:AA:BB:CC:DD:EE:FF", &a), 0);
	zassert_equal(a.type, BT_ADDR_LE_RANDOM);
	zassert_true(bt_addr_eq(&a.a, &example_alpha));
}

static ZTEST(bt_addr_le_from_str, test_rejects_unknown_prefix)
{
	bt_addr_le_t a;

	zassert_equal(bt_addr_le_from_str("X:11:22:33:44:55:66", &a), -EINVAL);
	zassert_equal(bt_addr_le_from_str("1:11:22:33:44:55:66", &a), -EINVAL);
}

static ZTEST(bt_addr_le_from_str, test_rejects_missing_prefix)
{
	bt_addr_le_t a;

	/* The bare address without a type prefix is not accepted. */
	zassert_equal(bt_addr_le_from_str("11:22:33:44:55:66", &a), -EINVAL);
}

static ZTEST(bt_addr_le_from_str, test_rejects_missing_separator)
{
	bt_addr_le_t a;

	zassert_equal(bt_addr_le_from_str("P11:22:33:44:55:66", &a), -EINVAL);
	zassert_equal(bt_addr_le_from_str("R-11:22:33:44:55:66", &a), -EINVAL);
	zassert_equal(bt_addr_le_from_str("R 11:22:33:44:55:66", &a), -EINVAL);
}

static ZTEST(bt_addr_le_from_str, test_rejects_malformed_address)
{
	bt_addr_le_t a;

	zassert_equal(bt_addr_le_from_str("R:11:22:33:44:55", &a), -EINVAL);
	zassert_equal(bt_addr_le_from_str("R:11:22:33:44:55:66:77", &a), -EINVAL);
	zassert_equal(bt_addr_le_from_str("P:gg:22:33:44:55:66", &a), -EINVAL);
	zassert_equal(bt_addr_le_from_str("P:11-22-33-44-55-66", &a), -EINVAL);
}

static ZTEST(bt_addr_le_from_str, test_rejects_truncated_string)
{
	bt_addr_le_t a;

	zassert_equal(bt_addr_le_from_str("", &a), -EINVAL);
	zassert_equal(bt_addr_le_from_str("P", &a), -EINVAL);
	zassert_equal(bt_addr_le_from_str("P:", &a), -EINVAL);
}

static ZTEST(bt_addr_le_from_str, test_roundtrip_through_to_str)
{
	bt_addr_le_t src = {
		.type = BT_ADDR_LE_RANDOM,
		.a = { { 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff } },
	};
	bt_addr_le_t dst;
	char str[BT_ADDR_LE_STR_LEN];
	int written;

	written = bt_addr_le_to_str(&src, str, sizeof(str));
	zassert_equal(written, (int)strlen(str));
	zassert_equal(bt_addr_le_from_str(str, &dst), 0);
	zassert_true(bt_addr_le_eq(&src, &dst));
}
