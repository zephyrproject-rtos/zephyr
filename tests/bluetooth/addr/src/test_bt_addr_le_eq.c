/* Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/addr.h>
#include <zephyr/ztest.h>

ZTEST_SUITE(bt_addr_le_eq, NULL, NULL, NULL, NULL, NULL);

ZTEST(bt_addr_le_eq, test_all_zero)
{
	bt_addr_le_t a = {.type = 0, .a = {{0, 0, 0, 0, 0, 0}}};
	bt_addr_le_t b = a;

	zassert_true(bt_addr_le_eq(&a, &b));
}

ZTEST(bt_addr_le_eq, test_type_not_zero)
{
	bt_addr_le_t a = {.type = 1, .a = {{1, 2, 3, 4, 5, 6}}};
	bt_addr_le_t b = a;

	zassert_true(bt_addr_le_eq(&a, &b));
}

ZTEST(bt_addr_le_eq, test_type_matters)
{
	bt_addr_le_t a = {.type = 0, .a = {{1, 2, 3, 4, 5, 6}}};
	bt_addr_le_t b = a;

	zassume_true(bt_addr_le_eq(&a, &b));
	a.type = 1;
	zassert_false(bt_addr_le_eq(&a, &b));
}

ZTEST(bt_addr_le_eq, test_address_matters_start)
{
	bt_addr_le_t a = {.type = 0, .a = {{1, 2, 3, 4, 5, 6}}};
	bt_addr_le_t b = a;

	zassume_true(bt_addr_le_eq(&a, &b));
	a.a.val[0] = 0;
	zassert_false(bt_addr_le_eq(&a, &b));
}

ZTEST(bt_addr_le_eq, test_address_matters_end)
{
	bt_addr_le_t a = {.type = 0, .a = {{1, 2, 3, 4, 5, 6}}};
	bt_addr_le_t b = a;

	zassume_true(bt_addr_le_eq(&a, &b));
	a.a.val[5] = 0;
	zassert_false(bt_addr_le_eq(&a, &b));
}

ZTEST(bt_addr_le_eq, test_only_type_and_address_matters)
{
	bt_addr_le_t a;
	bt_addr_le_t b;

	/* Make anything that is not the type and address unequal bytes. */
	memset(&a, 0xaa, sizeof(a));
	memset(&b, 0xbb, sizeof(b));
	a.type = 1;
	b.type = 1;
	memset(a.a.val, 1, sizeof(a.a.val));
	memset(b.a.val, 1, sizeof(b.a.val));

	zassert_true(bt_addr_le_eq(&a, &b));
}

ZTEST(bt_addr_le_eq, test_same_object)
{
	bt_addr_le_t a = {.type = 0, .a = {{1, 2, 3, 4, 5, 6}}};

	zassert_true(bt_addr_le_eq(&a, &a));
}
