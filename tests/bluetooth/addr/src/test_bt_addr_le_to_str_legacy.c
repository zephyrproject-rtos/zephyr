/* Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/ztest.h>

ZTEST_SUITE(bt_addr_le_to_str_legacy, NULL, NULL, NULL, NULL, NULL);

static const bt_addr_le_t example_addr = {
	.type = 0,
	.a = { { 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 } },
};

static ZTEST(bt_addr_le_to_str_legacy, test_recommended_buffer_grows_for_legacy)
{
	zassert_true(BT_ADDR_LE_STR_LEN >= sizeof("11:22:33:44:55:66 (random-id)"));
}

static ZTEST(bt_addr_le_to_str_legacy, test_public_uses_parenthesised_type)
{
	bt_addr_le_t a = example_addr;
	char str[BT_ADDR_LE_STR_LEN];

	a.type = BT_ADDR_LE_PUBLIC;

	(void)bt_addr_le_to_str(&a, str, sizeof(str));
	zassert_str_equal(str, "11:22:33:44:55:66 (public)");
}

static ZTEST(bt_addr_le_to_str_legacy, test_random_uses_parenthesised_type)
{
	bt_addr_le_t a = example_addr;
	char str[BT_ADDR_LE_STR_LEN];

	a.type = BT_ADDR_LE_RANDOM;

	(void)bt_addr_le_to_str(&a, str, sizeof(str));
	zassert_str_equal(str, "11:22:33:44:55:66 (random)");
}

static ZTEST(bt_addr_le_to_str_legacy, test_public_id_keeps_distinct_label)
{
	bt_addr_le_t a = example_addr;
	char str[BT_ADDR_LE_STR_LEN];

	a.type = BT_ADDR_LE_PUBLIC_ID;

	(void)bt_addr_le_to_str(&a, str, sizeof(str));
	zassert_str_equal(str, "11:22:33:44:55:66 (public-id)");
}

static ZTEST(bt_addr_le_to_str_legacy, test_random_id_keeps_distinct_label)
{
	bt_addr_le_t a = example_addr;
	char str[BT_ADDR_LE_STR_LEN];

	a.type = BT_ADDR_LE_RANDOM_ID;

	(void)bt_addr_le_to_str(&a, str, sizeof(str));
	zassert_str_equal(str, "11:22:33:44:55:66 (random-id)");
}

static ZTEST(bt_addr_le_to_str_legacy, test_unknown_type_prints_hex)
{
	bt_addr_le_t a = example_addr;
	char str[BT_ADDR_LE_STR_LEN];

	a.type = 0x42;

	(void)bt_addr_le_to_str(&a, str, sizeof(str));
	zassert_str_equal(str, "11:22:33:44:55:66 (0x42)");
}
