/* Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/ztest.h>

ZTEST_SUITE(bt_addr_le_from_str, NULL, NULL, NULL, NULL, NULL);

/* The example address used throughout these tests, in on-air (little-endian) order. */
static const bt_addr_t example = { { 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 } };

static ZTEST(bt_addr_le_from_str, test_new_format_public)
{
	bt_addr_le_t a;

	zassert_equal(bt_addr_le_from_str("P:11:22:33:44:55:66", "", &a), 0);
	zassert_equal(a.type, BT_ADDR_LE_PUBLIC);
	zassert_true(bt_addr_eq(&a.a, &example));
}

static ZTEST(bt_addr_le_from_str, test_new_format_random)
{
	bt_addr_le_t a;

	zassert_equal(bt_addr_le_from_str("R:11:22:33:44:55:66", "", &a), 0);
	zassert_equal(a.type, BT_ADDR_LE_RANDOM);
	zassert_true(bt_addr_eq(&a.a, &example));
}

static ZTEST(bt_addr_le_from_str, test_new_format_ignores_type_argument)
{
	/* The dedicated type-arg is intentionally meaningless when a prefix is
	 * present so that single-token shell input works unchanged.
	 */
	bt_addr_le_t a;

	zassert_equal(bt_addr_le_from_str("P:11:22:33:44:55:66", "random", &a), 0);
	zassert_equal(a.type, BT_ADDR_LE_PUBLIC);
}

static ZTEST(bt_addr_le_from_str, test_new_format_rejects_bogus_prefix)
{
	bt_addr_le_t a;

	/* "X:" is neither public nor random. Since str[0] is not P/p/R/r, the
	 * implementation falls through to the legacy parser, which then rejects
	 * the leading "X:" because it isn't a hex byte.
	 */
	zassert_equal(bt_addr_le_from_str("X:11:22:33:44:55:66", "random", &a), -EINVAL);
}

static ZTEST(bt_addr_le_from_str, test_new_format_rejects_malformed_address)
{
	bt_addr_le_t a;

	zassert_equal(bt_addr_le_from_str("R:11:22:33:44:55", "", &a), -EINVAL);
	zassert_equal(bt_addr_le_from_str("P:gg:22:33:44:55:66", "", &a), -EINVAL);
}

static ZTEST(bt_addr_le_from_str, test_legacy_format_public)
{
	bt_addr_le_t a;

	zassert_equal(bt_addr_le_from_str("11:22:33:44:55:66", "public", &a), 0);
	zassert_equal(a.type, BT_ADDR_LE_PUBLIC);
	zassert_true(bt_addr_eq(&a.a, &example));
}

static ZTEST(bt_addr_le_from_str, test_legacy_format_random)
{
	bt_addr_le_t a;

	zassert_equal(bt_addr_le_from_str("11:22:33:44:55:66", "random", &a), 0);
	zassert_equal(a.type, BT_ADDR_LE_RANDOM);
	zassert_true(bt_addr_eq(&a.a, &example));
}

static ZTEST(bt_addr_le_from_str, test_legacy_format_parenthesised_type)
{
	bt_addr_le_t a;

	/* The legacy parser also accepts the bracketed form produced by older
	 * versions of bt_addr_le_to_str().
	 */
	zassert_equal(bt_addr_le_from_str("11:22:33:44:55:66", "(public)", &a), 0);
	zassert_equal(a.type, BT_ADDR_LE_PUBLIC);

	zassert_equal(bt_addr_le_from_str("11:22:33:44:55:66", "(random)", &a), 0);
	zassert_equal(a.type, BT_ADDR_LE_RANDOM);
}

static ZTEST(bt_addr_le_from_str, test_legacy_format_public_id_and_random_id)
{
	bt_addr_le_t a;

	zassert_equal(bt_addr_le_from_str("11:22:33:44:55:66", "public-id", &a), 0);
	zassert_equal(a.type, BT_ADDR_LE_PUBLIC_ID);

	zassert_equal(bt_addr_le_from_str("11:22:33:44:55:66", "random-id", &a), 0);
	zassert_equal(a.type, BT_ADDR_LE_RANDOM_ID);
}

static ZTEST(bt_addr_le_from_str, test_legacy_format_rejects_unknown_type)
{
	bt_addr_le_t a;

	zassert_equal(bt_addr_le_from_str("11:22:33:44:55:66", "weird", &a), -EINVAL);
}

static ZTEST(bt_addr_le_from_str, test_roundtrip_through_to_str)
{
	bt_addr_le_t src = {
		.type = BT_ADDR_LE_RANDOM,
		.a = { { 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff } },
	};
	bt_addr_le_t dst;
	char str[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(&src, str, sizeof(str));
	zassert_equal(bt_addr_le_from_str(str, "", &dst), 0);
	zassert_true(bt_addr_le_eq(&src, &dst));
}
