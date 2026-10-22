/* Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/addr.h>
#include <zephyr/ztest.h>

#include <host/addr_internal.h>

ZTEST_SUITE(bt_addr_le_is_resolved, NULL, NULL, NULL, NULL, NULL);

static const bt_addr_t example = { { 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 } };

static ZTEST(bt_addr_le_is_resolved, test_identity_types_are_resolved)
{
	bt_addr_le_t a = { .a = example };

	a.type = BT_ADDR_LE_PUBLIC_ID;
	zassert_true(bt_addr_le_is_resolved(&a));

	a.type = BT_ADDR_LE_RANDOM_ID;
	zassert_true(bt_addr_le_is_resolved(&a));
}

static ZTEST(bt_addr_le_is_resolved, test_plain_types_are_not_resolved)
{
	bt_addr_le_t a = { .a = example };

	a.type = BT_ADDR_LE_PUBLIC;
	zassert_false(bt_addr_le_is_resolved(&a));

	a.type = BT_ADDR_LE_RANDOM;
	zassert_false(bt_addr_le_is_resolved(&a));
}

static ZTEST(bt_addr_le_is_resolved, test_sentinels_are_not_resolved)
{
	/* The LE Extended Advertising Report sentinels have bit 1 set as well,
	 * but neither is a Controller-resolved identity address.
	 */
	bt_addr_le_t a = { .a = example };

	a.type = BT_ADDR_LE_UNRESOLVED;
	zassert_false(bt_addr_le_is_resolved(&a));

	a.type = BT_ADDR_LE_ANONYMOUS;
	zassert_false(bt_addr_le_is_resolved(&a));
}

static ZTEST(bt_addr_le_is_resolved, test_copy_resolved_yields_base_type)
{
	bt_addr_le_t src = { .a = example };
	bt_addr_le_t dst;

	src.type = BT_ADDR_LE_PUBLIC_ID;
	bt_addr_le_copy_resolved(&dst, &src);
	zassert_equal(dst.type, BT_ADDR_LE_PUBLIC);
	zassert_true(bt_addr_eq(&dst.a, &example));

	src.type = BT_ADDR_LE_RANDOM_ID;
	bt_addr_le_copy_resolved(&dst, &src);
	zassert_equal(dst.type, BT_ADDR_LE_RANDOM);
	zassert_true(bt_addr_eq(&dst.a, &example));
}
