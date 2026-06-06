/* Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/ztest.h>

ZTEST_SUITE(bt_addr_le_to_str, NULL, NULL, NULL, NULL, NULL);

static const bt_addr_le_t example_addr = {
	.type = 0,
	.a = { { 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 } },
};

ZTEST(bt_addr_le_to_str, test_public_uses_p_prefix)
{
	bt_addr_le_t a = example_addr;
	char str[BT_ADDR_LE_STR_LEN];

	a.type = BT_ADDR_LE_PUBLIC;

	zassert_equal(bt_addr_le_to_str(&a, str, sizeof(str)), strlen("P:11:22:33:44:55:66"));
	zassert_str_equal(str, "P:11:22:33:44:55:66");
}

ZTEST(bt_addr_le_to_str, test_random_uses_r_prefix)
{
	bt_addr_le_t a = example_addr;
	char str[BT_ADDR_LE_STR_LEN];

	a.type = BT_ADDR_LE_RANDOM;

	zassert_equal(bt_addr_le_to_str(&a, str, sizeof(str)), strlen("R:11:22:33:44:55:66"));
	zassert_str_equal(str, "R:11:22:33:44:55:66");
}

ZTEST(bt_addr_le_to_str, test_public_id_masks_to_public_prefix)
{
	/* BT_ADDR_LE_PUBLIC_ID (0x02) is the identity-address marker derived from a
	 * public on-air address; the base type is still public, so it must print as P:.
	 */
	bt_addr_le_t a = example_addr;
	char str[BT_ADDR_LE_STR_LEN];

	a.type = BT_ADDR_LE_PUBLIC_ID;

	zassert_equal(bt_addr_le_to_str(&a, str, sizeof(str)), strlen("P:11:22:33:44:55:66"));
	zassert_str_equal(str, "P:11:22:33:44:55:66");
}

ZTEST(bt_addr_le_to_str, test_random_id_masks_to_random_prefix)
{
	bt_addr_le_t a = example_addr;
	char str[BT_ADDR_LE_STR_LEN];

	a.type = BT_ADDR_LE_RANDOM_ID;

	zassert_equal(bt_addr_le_to_str(&a, str, sizeof(str)), strlen("R:11:22:33:44:55:66"));
	zassert_str_equal(str, "R:11:22:33:44:55:66");
}

ZTEST(bt_addr_le_to_str, test_hci_resolved_random_masks_to_random_prefix)
{
	/* HCI events use bit 1 of Address_Type to indicate the Controller resolved an
	 * RPA. bt_addr_le_to_str() must mask that bit away so the public API string
	 * format only reflects the base address type. The extra bit is logged
	 * separately at the call sites that handle raw HCI event addresses.
	 */
	bt_addr_le_t a = example_addr;
	char str[BT_ADDR_LE_STR_LEN];

	a.type = BT_ADDR_LE_RANDOM | 0x02U;

	(void)bt_addr_le_to_str(&a, str, sizeof(str));
	zassert_str_equal(str, "R:11:22:33:44:55:66");
}

ZTEST(bt_addr_le_to_str, test_hci_resolved_public_masks_to_public_prefix)
{
	bt_addr_le_t a = example_addr;
	char str[BT_ADDR_LE_STR_LEN];

	a.type = BT_ADDR_LE_PUBLIC | 0x02U;

	(void)bt_addr_le_to_str(&a, str, sizeof(str));
	zassert_str_equal(str, "P:11:22:33:44:55:66");
}

ZTEST(bt_addr_le_to_str, test_byte_order_is_big_endian_display)
{
	/* a.val is stored little-endian (on-air order) but the string is rendered
	 * with the most significant byte first.
	 */
	bt_addr_le_t a = {
		.type = BT_ADDR_LE_PUBLIC,
		.a = { { 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 } },
	};
	char str[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(&a, str, sizeof(str));
	zassert_str_equal(str, "P:01:02:03:04:05:06");
}

ZTEST(bt_addr_le_to_str, test_recommended_buffer_is_large_enough)
{
	bt_addr_le_t a = {
		.type = BT_ADDR_LE_RANDOM,
		.a = { { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } },
	};
	char str[BT_ADDR_LE_STR_LEN] = { 0 };
	int written = bt_addr_le_to_str(&a, str, sizeof(str));

	/* snprintk returns the number of bytes it would have written, excluding the
	 * trailing NUL. The recommended buffer must fit the full string + NUL.
	 */
	zassert_true(written > 0);
	zassert_true((size_t)written < sizeof(str),
		     "BT_ADDR_LE_STR_LEN (%d) too small to hold output (%d bytes)",
		     BT_ADDR_LE_STR_LEN, written);
}

ZTEST(bt_addr_le_to_str, test_anonymous_still_produces_valid_prefix)
{
	/* BT_ADDR_LE_ANONYMOUS (0xff) has bit 0 set, so it must mask to R: rather
	 * than producing an invalid prefix. This is a corner case but exercises
	 * the masking on an arbitrary high-bit value.
	 */
	bt_addr_le_t a = example_addr;
	char str[BT_ADDR_LE_STR_LEN];

	a.type = BT_ADDR_LE_ANONYMOUS;

	(void)bt_addr_le_to_str(&a, str, sizeof(str));
	zassert_true(str[0] == 'P' || str[0] == 'R', "got '%c'", str[0]);
	zassert_equal(str[1], ':');
}
